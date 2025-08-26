#include "override.h"
#include "modern_cpp.h"  // For performance optimizations

// SIMD headers for optimization
#include <immintrin.h>  // AVX2 intrinsics
#include <algorithm>    // std::max, std::min

// Modern constants
namespace cache_constants {
    constexpr int BITMAP_REDUCE_COUNTER = 256;  // Default was 1024, optimized for performance
    constexpr size_t MAX_CACHE_SIZE = 50 * 1024 * 1024;  // 50MB cache limit
    constexpr size_t MEMORY_POOL_BLOCK_SIZE = 64 * 1024;  // 64KB blocks for bitmap data
}


HDC CBitmapCache::CreateDC(HDC dc)
{
	if(!m_hdc) {
		m_hdc = CreateCompatibleDC(dc);
		m_exthdc = dc;
	}
	return m_hdc;
}

HBITMAP CBitmapCache::CreateDIB(int width, int height, BYTE** lplpPixels)
{
	SIZE& dibSize = m_dibSize;
	width  = (width + 3) & ~3;  // Align to 4-byte boundary

	// Check if existing bitmap is still suitable
	if (dibSize.cx >= width && dibSize.cy >= height) {
		if (++m_counter < cache_constants::BITMAP_REDUCE_COUNTER) {
			*lplpPixels = m_lpPixels;
			return m_hbmp;
		}
		//カウンタ超過
		//ただしサイズが全く同じなら再生成しない
		if (dibSize.cx == width && dibSize.cy == height) {
			m_counter   = 0;
			*lplpPixels = m_lpPixels;
			return m_hbmp;
		}
	} else {
		// Expand to larger size for better reuse
		if (dibSize.cx > width) {
			width  = dibSize.cx;
		}
		if (dibSize.cy > height) {
			height = dibSize.cy;
		}
	}

	// Modern memory management with optimized bitmap creation
	const size_t bitmapSize = static_cast<size_t>(width) * static_cast<size_t>(height) * 4; // 32-bit RGBA

	// Check memory pool limits
	if (total_allocated_memory_ + bitmapSize > cache_constants::MAX_CACHE_SIZE) {
		// Memory limit exceeded, clean up old resources
		if (m_hbmp) {
			DeleteBitmap(m_hbmp);
			memory_pool_.deallocate(m_lpPixels, static_cast<size_t>(dibSize.cx) * dibSize.cy * 4);
			total_allocated_memory_ -= static_cast<size_t>(dibSize.cx) * dibSize.cy * 4;
		}
	}

	// Allocate from memory pool
	BYTE* pixels = static_cast<BYTE*>(memory_pool_.allocate(bitmapSize));
	if (!pixels) {
		return nullptr;  // Memory allocation failed
	}

	// Create DIB section with pre-allocated memory using RAII
	auto cleanup_pixels = [&](BYTE* p) {
		if (p) memory_pool_.deallocate(p, bitmapSize);
	};
	std::unique_ptr<BYTE, decltype(cleanup_pixels)> pixel_guard(pixels, cleanup_pixels);

	BITMAPINFOHEADER bmiHeader = { sizeof(BITMAPINFOHEADER), width, -height, 1, 32, BI_RGB };

	// Create compatible DC for the bitmap
	error_handling::UniqueHDC dc_guard(CreateDC(m_exthdc), DeleteDC);
	if (!dc_guard) {
		return nullptr;
	}

	HBITMAP hbmpNew = CreateDIBSection(dc_guard.get(), (BITMAPINFO*)&bmiHeader, DIB_RGB_COLORS, (LPVOID*)&pixels, NULL, 0);
	if (!hbmpNew) {
		return nullptr; // pixel_guard will automatically clean up
	}

	// Success - create RAII wrapper for the bitmap
	pixel_guard.release(); // Don't clean up pixels, they're now owned by the bitmap

	// Success - update cache
	if (m_hbmp) {
		DeleteBitmap(m_hbmp);
		// Deallocate previous memory
		size_t prevSize = static_cast<size_t>(dibSize.cx) * dibSize.cy * 4;
		memory_pool_.deallocate(m_lpPixels, prevSize);
		total_allocated_memory_ -= prevSize;
	}

	m_hbmp = hbmpNew;
	dibSize.cx = width;
	dibSize.cy = height;
	m_lpPixels = pixels;
	m_counter = 0;
	total_allocated_memory_ += bitmapSize;

	*lplpPixels = m_lpPixels;
	return m_hbmp;
}

void CBitmapCache::FillSolidRect(COLORREF rgb, const RECT* lprc)
{
	// RAII-based brush management
	if (!m_brush || rgb != m_bkColor) {
		if (m_brush) {
			DeleteObject(m_brush);
		}
		m_brush = CreateSolidBrush(rgb);
		if (!m_brush) {
			// Log error but don't crash
			debug_utils::logDebugInfo("Failed to create solid brush");
			return;
		}
		m_bkColor = rgb;
	}

	// Use GDI for small rectangles (more efficient)
	if (lprc && (lprc->right - lprc->left) * (lprc->bottom - lprc->top) < 1000) {
		FillRect(m_hdc, lprc, m_brush);
		return;
	}

	// For larger rectangles, use direct pixel manipulation with SIMD
	if (m_lpPixels && lprc && m_dibSize.cx > 0 && m_dibSize.cy > 0) {
		const int rectWidth = lprc->right - lprc->left;
		const int rectHeight = lprc->bottom - lprc->top;

		// Clamp rectangle to bitmap bounds
		const int startX = std::max(0, lprc->left);
		const int startY = std::max(0, lprc->top);
		const int endX = std::min(m_dibSize.cx, lprc->right);
		const int endY = std::min(m_dibSize.cy, lprc->bottom);

		// Convert RGB to DIB format (BGR)
		const DWORD dibColor = RGB(GetBValue(rgb), GetGValue(rgb), GetRValue(rgb));

		// Fill rectangle with SIMD optimization
		for (int y = startY; y < endY; ++y) {
			DWORD* pixelPtr = reinterpret_cast<DWORD*>(m_lpPixels) + y * m_dibSize.cx + startX;
			const size_t pixelsToFill = static_cast<size_t>(endX - startX);

			// Use SIMD for large fills
			if (pixelsToFill >= 8) {
				// Fill 8 pixels at a time with SIMD
				const __m256i colorVec = _mm256_set1_epi32(static_cast<int>(dibColor));
				__m256i* vectorPtr = reinterpret_cast<__m256i*>(pixelPtr);

				size_t vectorCount = pixelsToFill / 8;
				for (size_t i = 0; i < vectorCount; ++i) {
					_mm256_storeu_si256(vectorPtr++, colorVec);
				}

				// Handle remaining pixels
				size_t remainingPixels = pixelsToFill % 8;
				pixelPtr += vectorCount * 8;

				for (size_t i = 0; i < remainingPixels; ++i) {
					*pixelPtr++ = dibColor;
				}
			} else {
				// Small fill - use simple loop
				for (size_t i = 0; i < pixelsToFill; ++i) {
					*pixelPtr++ = dibColor;
				}
			}
		}
	} else {
		// Fallback to GDI
		FillRect(m_hdc, lprc, m_brush);
	}
}

//水平線を引く
//(X1,Y1)           (X2,Y1)
//   +-----------------+   ^
//   |       rgb       |   | width
//   +-----------------+   v
void CBitmapCache::DrawHorizontalLine(int X1, int Y1, int X2, COLORREF rgb, int width)
{
	if (!m_dibSize.cx || !m_dibSize.cy) {
		return;
	}

	if (X1 > X2) {
		const int xx = X1;
		X1 = X2;
		X2 = xx;
	}

	//クリッピング
	const int xSize = m_dibSize.cx;
	const int ySize = m_dibSize.cy;
	X1 = Bound(X1, 0, xSize);
	X2 = Bound(X2, 0, xSize);
	Y1 = Bound(Y1, 0, ySize);
	width = Max(width, 1);
	const int Y2 = Bound(Y1 + width, 0, ySize);

	rgb = RGB2DIB(rgb);

	DWORD* lpPixels = (DWORD*)m_lpPixels + (Y1 * xSize + X1);
	const int Xd = X2 - X1;
	const int Yd = Y2 - Y1;
/*	for (int yy=Y1; yy<Y2; yy++) {
		for (int xx=X1; xx<X2; xx++) {
			_SetPixelV(xx, yy, rgb);
		}
	}

	for (int yy=Y1; yy<Y2; yy++, lpPixels += xSize) {
		__asm {
			mov edi, dword ptr [lpPixels]
			mov ecx, dword ptr [Xd]
			mov eax, dword ptr [rgb]
			cld
			rep stosd
		}
	}*/

/*#ifdef _M_IX86
	//無意味にアセンブリ化
	__asm {
		mov ebx, dword ptr [Yd]
		mov edx, dword ptr [lpPixels]
		mov esi, dword ptr [xSize]
		cld
L1:
		mov edi, edx
		mov ecx, dword ptr [Xd]
		mov eax, dword ptr [rgb]
		rep stosd
		lea edx, dword ptr [edx+esi*4]
		dec ebx
		jnz L1
	}
#else*/	//对于64位系统，使用C语言
	for (int yy=Y1; yy<Y2; yy++) {
		for (int xx=X1; xx<X2; xx++) {
			*( (DWORD*)m_lpPixels + (yy * xSize + xx) ) = rgb;
		}
	}
//#endif

}
