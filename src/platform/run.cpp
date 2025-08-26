// dll injection
// MacType SDK 호환성 헤더 (최신 Windows SDK와의 충돌 방지)
#include "sdk_compat.h"

#define UNICODE  1
#define _UNICODE 1

#include <Windows.h>
#include <ShellApi.h>
#include <ComDef.h>
#include <ShlObj.h>
#include <ShLwApi.h>
#include <tchar.h>
#include "array.h"
#include <strsafe.h>

// _vsnwprintf用
#include <wchar.h>		
#include <stdarg.h>
#include <string_view>
#include <array>
#include <algorithm>

#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <malloc.h>
#include <crtdbg.h>

// 위험한 for 매크로 제거 - 현대적 범위 기반 for 루프 사용 권장
// #define for if(0);else for  // <- 제거됨

// C++17 std::size 사용 권장
#ifndef _countof
    #if __cplusplus >= 201703L
        #include <iterator>
        #define _countof(array) std::size(array)
    #else
        #define _countof(array) (sizeof(array) / sizeof((array)[0]))
    #endif
#endif

#pragma comment(linker, "/subsystem:windows,5.0")
#pragma comment(lib, "Kernel32.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "ShLwApi.lib")
#pragma comment(lib, "Ole32.lib")

#define IDS_USAGE		101
#define IDS_DLL			102
#define IDC_EXEC		103

// 현대적 에러 메시지 표시 함수
static void ShowMessage(std::string_view msg) noexcept {
    MessageBoxA(nullptr, msg.data(), "MacType ERROR", MB_OK | MB_ICONSTOP);
}

static void ShowErrorMessage(UINT id, DWORD code) noexcept
{
    constexpr size_t BUFFER_SIZE = 512;
    constexpr size_t FORMAT_SIZE = 128;
    
    std::array<char, BUFFER_SIZE> buffer{};
    std::array<char, FORMAT_SIZE> format{};
    
    if (LoadStringA(GetModuleHandleA(nullptr), id, format.data(), FORMAT_SIZE) > 0) {
        // 현대적 문자열 포맷팅 (안전한 버전)
        if (sprintf_s(buffer.data(), BUFFER_SIZE, format.data(), code) > 0) {
            ShowMessage(std::string_view{ buffer.data() });
        } else {
            ShowMessage("MacType: Unknown error occurred");
        }
    } else {
        ShowMessage("MacType: Failed to load error message");
    }
}

// 런타임 함수로 현대화 (GetLastError는 constexpr이 아님)
[[nodiscard]] inline HRESULT HresultFromLastError() noexcept
{
    const DWORD error_code = GetLastError();
    return HRESULT_FROM_WIN32(error_code);
}

// 레거시 호환성을 위한 함수들
static void showmsg(LPCSTR msg) {
    ShowMessage(msg ? std::string_view{msg} : std::string_view{"Unknown error"});
}

static void errmsg(UINT id, DWORD code) {
    ShowErrorMessage(id, code);
}


#include "detours.h"

// 아키텍처별 라이브러리 및 DLL 이름 설정
#if defined(_M_ARM64)
    #pragma comment (lib, "detours_arm64.lib")
    const auto MacTypeDll = L"MacType_ARM64.dll";
    const auto MacTypeDllA = "MacType_ARM64.dll";
#elif defined(_M_X64)
	// vcpkg는 detours를 detours.lib로 제공합니다 (아키텍처 접미사 없음)
	#pragma comment (lib, "detours.lib")
    const auto MacTypeDll = L"MacType64.dll";
    const auto MacTypeDllA = "MacType64.dll";
#elif defined(_M_IX86)
    #pragma comment (lib, "detours.lib")
    const auto MacTypeDll = L"MacType.dll";
    const auto MacTypeDllA = "MacType.dll";
#else
    #error "Unsupported architecture"
#endif


HINSTANCE hinstDLL;

#include <stddef.h>
#define GetDLLInstance()	(hinstDLL)

#define _GDIPP_EXE
#define _GDIPP_RUN_CPP
//#include "supinfo.h"

//#define OLD_PSDK

#ifdef OLD_PSDK
extern "C" {
	HRESULT WINAPI _SHILCreateFromPath(LPCWSTR pszPath, LPITEMIDLIST* ppidl, DWORD* rgflnOut)
	{
		if (!pszPath || !ppidl) {
			return E_INVALIDARG;
		}

		LPSHELLFOLDER psf;
		HRESULT hr = ::SHGetDesktopFolder(&psf);
		if (hr != NOERROR) {
			return hr;
		}

		ULONG chEaten;
		LPOLESTR lpszDisplayName = ::StrDupW(pszPath);
		hr = psf->ParseDisplayName(NULL, NULL, lpszDisplayName, &chEaten, ppidl, rgflnOut);
		::LocalFree(lpszDisplayName);
		psf->Release();
		return hr;
	}

	void WINAPI _SHFree(void* pv)
	{
		if (!pv) {
			return;
		}

		LPMALLOC pMalloc = NULL;
		if (::SHGetMalloc(&pMalloc) == NOERROR) {
			pMalloc->Free(pv);
			pMalloc->Release();
		}
	}
}
#else
#define _SHILCreateFromPath	SHILCreateFromPath
#define _SHFree				SHFree
#endif


bool isX64PE(const TCHAR* file_path) {
	HANDLE hFile = CreateFile(file_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		showmsg("Error opening file");
		return false;
	}

	IMAGE_DOS_HEADER dosHeader;
	DWORD bytesRead;
	if (!ReadFile(hFile, &dosHeader, sizeof(IMAGE_DOS_HEADER), &bytesRead, NULL)) {
		showmsg("Error reading file");
		CloseHandle(hFile);
		return false;
	}

	// Check if it's a PE file
	if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE) {
		showmsg("Not a PE file");
		CloseHandle(hFile);
		return false;
	}

	IMAGE_NT_HEADERS ntHeaders;
	// Seek to the PE header offset
	SetFilePointer(hFile, dosHeader.e_lfanew, NULL, FILE_BEGIN);
	if (!ReadFile(hFile, &ntHeaders, sizeof(IMAGE_NT_HEADERS), &bytesRead, NULL)) {
		showmsg("Error reading PE header");
		CloseHandle(hFile);
		return false;
	}

	if (ntHeaders.FileHeader.Machine == IMAGE_FILE_MACHINE_I386) {
		CloseHandle(hFile);
		return false;
	}
	else if (ntHeaders.FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64) {
		CloseHandle(hFile);
		return true;
	}
	else {
		CloseHandle(hFile);
		return false;
	}
}


// １つ目の引数だけファイルとして扱い、実行する。
//
// コマンドは こんな感じで連結されます。
//  exe linkpath linkarg cmdarg2 cmdarg3 cmdarg4 ...
//
static HRESULT HookAndExecute(int show)
{
	int     argc = 0;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (!argv) {
		return HresultFromLastError();
	}
	if (argc <= 1) {
		char buffer[256];
		LoadStringA(GetModuleHandleA(NULL), IDS_USAGE, buffer, 256);
		MessageBoxA(NULL,
			buffer
			, "MacType", MB_OK | MB_ICONINFORMATION);
		LocalFree(argv);
		return S_OK;
	}


	int i;
	size_t length = 1;
	for (i = 1; i < argc; i++) {
		length += wcslen(argv[i]) + 3;
	}

	LPWSTR cmdline = (WCHAR*)calloc(sizeof(WCHAR), length);
	if (!cmdline) {
		LocalFree(argv);
		return E_OUTOFMEMORY;
	}

	LPWSTR p = cmdline;
	*p = L'\0';
	for (i = 1; i < argc; i++) {
		const bool dq = !!wcschr(argv[i], L' ');
		if (dq) {
			*p++ = '"';
			length--;
		}
		StringCchCopyExW(p, length, argv[i], &p, &length, STRSAFE_NO_TRUNCATION);
		if (dq) {
			*p++ = '"';
			length--;
		}
		*p++ = L' ';
		length--;
	}

	*CharPrevW(cmdline, p) = L'\0';

// Modern approach: mt64agnt handles all architectures
// mt64agnt automatically detects process architecture and injects appropriate DLL
#ifdef _M_IX86
// x86 Windows에서는 mt64agnt를 실행할 수 없음 (32비트 OS)
// 32비트 프로세스만 직접 처리
return S_OK;  // 기존 로직으로 진행
#else
// x64/ARM64 Windows에서는 mt64agnt 사용
ShellExecute(NULL, NULL, L"mt64agnt.exe", cmdline, NULL, SW_SHOW);
return S_OK;
#endif

	WCHAR file[MAX_PATH], dir[MAX_PATH];
	GetCurrentDirectoryW(_countof(dir), dir);
	StringCchCopyW(file, _countof(file), argv[1]);
	if (PathIsRelativeW(file)) {
		PathCombineW(file, dir, file);
	}
	else {
		WCHAR gdippDir[MAX_PATH];
		GetModuleFileNameW(NULL, gdippDir, _countof(gdippDir));
		PathRemoveFileSpec(gdippDir);

		// カレントディレクトリがgdi++.exeの置かれているディレクトリと同じだったら、
		// 起動しようとしているEXEのフルパスから抜き出したディレクトリ名をカレント
		// ディレクトリとして起動する。(カレントディレクトリがEXEと同じ場所である
		// 前提で作られているアプリ対策)
		if (wcscmp(dir, gdippDir) == 0) {
			StringCchCopyW(dir, _countof(dir), argv[1]);
			PathRemoveFileSpec(dir);
		}
	}

#ifdef _DEBUG
	if ((GetAsyncKeyState(VK_CONTROL) & 0x8000)
		&& MessageBoxW(NULL, cmdline, NULL, MB_YESNO) != IDYES) {
		free(cmdline);
		return NOERROR;
	}
#endif


	PROCESS_INFORMATION processInfo;
	STARTUPINFO startupInfo = { 0 };
	startupInfo.cb = sizeof(startupInfo);

	// get current directory and append mactype dll 
	char path[MAX_PATH] = { 0 };
	if (GetModuleFileNameA(NULL, path, _countof(path))) {
		PathRemoveFileSpecA(path);
		strcat(path, "\\");
	}
	strcat(path, MacTypeDllA);

	auto ret = DetourCreateProcessWithDllEx(NULL, cmdline, NULL, NULL, false, 0, NULL, dir, &startupInfo, &processInfo, path, NULL);

	free(cmdline);
	LocalFree(argv);
	argv = NULL;
	return ret ? S_OK : E_ACCESSDENIED;
}

int WINAPI wWinMain(HINSTANCE ins, HINSTANCE prev, LPWSTR cmd, int show)
{
    _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
    OleInitialize(NULL);

    // Optional: IFEO register/unregister minimal handler (admin required)
    if (cmd) {
        if (wcsstr(cmd, L"/register") != nullptr) {
            HKEY hKey;
            if (RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\notepad.exe",
                0, nullptr, 0, KEY_ALL_ACCESS, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
                wchar_t marker[] = L"MacType";
                RegSetValueExW(hKey, L"VerifierDlls", 0, REG_SZ, (const BYTE*)marker, sizeof(marker));
                RegCloseKey(hKey);
            }
            OleUninitialize();
            return 0;
        }
        if (wcsstr(cmd, L"/unregister") != nullptr) {
            RegDeleteTreeW(HKEY_LOCAL_MACHINE,
                L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\notepad.exe");
            OleUninitialize();
            return 0;
        }
    }

	WCHAR path[MAX_PATH];
	if (GetModuleFileNameW(NULL, path, _countof(path))) {
		PathRemoveFileSpec(path);
		wcscat(path, L"\\");
		wcscat(path, MacTypeDll);
		//DONT_RESOLVE_DLL_REFERENCESを指定すると依存関係の解決や
		//DllMainの呼び出しが行われない
		hinstDLL = LoadLibraryExW(path, NULL, DONT_RESOLVE_DLL_REFERENCES);
	}
	if (!hinstDLL) {
		errmsg(IDS_DLL, HresultFromLastError());
	}
	else {
		PathRemoveFileSpecW(path);
		SetCurrentDirectoryW(path);

		HRESULT hr = HookAndExecute(show);
		if (hr != S_OK) {
			errmsg(IDC_EXEC, hr);
		}
	}

    OleUninitialize();
	return 0;
}

//EOF
