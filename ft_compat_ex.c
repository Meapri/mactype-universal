#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>

// FreeType 커스텀 확장 심벌의 약식 래퍼/스텁
// 최신 FreeType에서 심벌이 사라졌거나 패치 적용 실패 시 빌드-링크를 유지하기 위해 제공
// 실제 구현은 FreeType 포크에 포함되며, 여기서는 하위호환을 위한 래퍼만 제공합니다.

#ifdef __cplusplus
extern "C" {
#endif

FT_Error FT_Glyph_To_BitmapEx(
    FT_Glyph*       the_glyph,
    FT_Render_Mode  render_mode,
    FT_Vector*      origin,
    FT_Bool         destroy,
    FT_Bool         loadcolor,
    FT_UInt         glyphindex,
    FT_Face         face)
{
    // 기본 FreeType API로 대체 경로: loadcolor/glyphindex/face는 무시
    // 필요 시 향후 포크 버전의 구현과 API를 동기화하세요
    (void)loadcolor; (void)glyphindex; (void)face;
    return FT_Glyph_To_Bitmap(the_glyph, render_mode, origin, destroy);
}

#ifdef __cplusplus
}
#endif


