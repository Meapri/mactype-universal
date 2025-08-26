MacType
========================
[日本語](./README_ja-JP.md)

**Windows-only** better font rendering solution.

> **⚠️ Important**: MacType is Windows-only software that enhances font rendering by hooking Windows GDI and DirectWrite APIs.

Latest build
------------------

[Download](https://github.com/snowie2000/mactype/releases/latest)

Official site
------------------

MacType official site:

http://www.mactype.net (An archived version is restored)

What's new?
------------------

- Win11 compatible
- CET compatible
- Updated FreeType
- Support for color fonts :sunglasses:
- New installer
- Lots of bug fixes
- Updates for multi-monitor support
- Tray app can intercept explorer in Service Mode now
- Tweaks for diacritics
- Updates to EasyHook
- Lower CPU in Tray Mode
- Better DirectWrite support thanks to [しらいと](http://silight.hatenablog.jp)
- Separate DirectWrite parameter adjustment
- Traditional Chinese localization greatly improved thanks to GT Wang
- English localization improved
- Added Korea localization, thanks to 조현희
- MultiLang system improved

Donation
------------------

MacType now accepts donations. 

Please visit http://www.mactype.net and keep an eye on the bottom right corner :heart:

Thank you for your support! Your donations will keep the server running, keep me updating, and buy more coffees :coffee:

Known issues
---------------

- Please backup your profiles before upgrading!

- Only Chinese simplified/Traditional and English are fully localized, some options may missing in MacType Tuner due to the strings missing in the language file. You can help with translations!

- If you want to use MacType-patch together with MacType official release, remember to add DirectWrite=0 to your profile or you will have mysterious problems

- If you're running 64 bit Windows, antimalware/antivirus software may conflict with MacType, because it sees MacType trying to modify running software. One possible workaround is to try running in Service Mode (recommended), or add HookChildProcesses=0 to your profile. See https://github.com/snowie2000/mactype/wiki/HookChildProcesses for an explanation

- Office 2013 does not use DirectWrite or GDI (it uses its own custom rendering), so Office 2013 doesn't work with MacType. If this bothers you you can use Office 2010 which uses GDI or Office 2016+ which uses DirectWrite.

- WPS has a built in defense that **UNLOADS** MacType automatically. The latest version has a workaround [here](https://github.com/snowie2000/mactype/wiki/WPS) thanks to wmjordan.

How to get registry mode back
-------------

It is no longer possible to enable registry mode via the wizard in Windows 10. 

We have a detailed guide on how you can enable the registry mode manually in [wiki](https://github.com/snowie2000/mactype/wiki/Enable-registry-mode-manually), get your screwdrivers ready before you head over to it.

How to build
-------------

### 자동 빌드 (GitHub Actions)

이 리포지토리는 GitHub Actions를 통한 자동 빌드를 지원합니다:

[![Build Status](https://github.com/snowie2000/mactype/workflows/Build%20MacType/badge.svg)](https://github.com/snowie2000/mactype/actions)

- **CI 빌드**: 코드 푸시 시 자동으로 Windows x86/x64 버전 빌드
- **자동 릴리스**: 태그 생성 시 자동으로 바이너리를 릴리스에 첨부
- **종속성 자동 관리**: FreeType, Detours, IniParser 등 모든 Windows 종속성 자동 빌드

자세한 내용은 [빌드 설명서](.github/BUILD_INSTRUCTIONS.md)를 참조하세요.

### 수동 빌드

빌드 환경 체크:
```powershell
.\scripts\check-build-env.ps1
```

종속성 빌드:
```powershell
.\scripts\build-dependencies.ps1 -Platform x86
.\scripts\build-dependencies.ps1 -Platform x64
```

MacType 빌드:
```powershell
msbuild gdipp.sln -p:Configuration=Release -p:Platform=Win32  # x86
msbuild gdipp.sln -p:Configuration=Release -p:Platform=x64    # x64
```

상세한 빌드 방법은 [빌드 문서](doc/HOWTOBUILD.md)를 확인하세요.

## Architecture

MacType Universal follows a modern modular architecture:

```
src/
├── core/           # FreeType rendering engine
│   ├── ft.cpp      # FreeType integration with modern C++
│   ├── fteng.cpp   # Font engine with enhanced features
│   ├── override.cpp # Main rendering override logic
│   └── ...
├── hooking/        # Windows API hooking system
│   ├── hook.cpp    # Detours-based API interception
│   ├── hooklist.h  # Hook function definitions
│   └── ...
├── settings/       # Configuration management
│   ├── settings.cpp # JSON-based settings with modern parsing
│   └── settings.h  # Configuration interfaces
├── cache/          # Memory and performance optimization
│   ├── cache.cpp   # SIMD-optimized bitmap caching
│   └── cache.h     # Memory pool and cache management
├── platform/       # Windows-specific implementations
│   ├── dll.cpp     # DLL loading and management
│   ├── directwrite.cpp # DirectWrite integration
│   └── ...
└── utils/          # Shared utilities
    ├── common.cpp  # Common helper functions
    ├── EventLogging.cpp # Modern event logging
    └── ...

include/
└── modern_cpp.h    # C++17/20 utilities and patterns

tests/              # Comprehensive test suite
├── test_main.cpp   # Google Test integration
└── test_*.cpp      # Unit tests for each module
```

### Key Modern Features

- **Memory Management**: RAII patterns, smart pointers, custom memory pools
- **Error Handling**: Exception-safe operations, typed error codes
- **Performance**: SIMD optimization, thread-safe memory pools
- **Type Safety**: Modern C++ templates, constexpr, string_view
- **Testing**: Comprehensive unit tests with Google Test/Mock

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

### Development Requirements

- C++20 compatible compiler (MSVC 2022+)
- CMake 3.20+
- vcpkg for dependency management
- Google Test for testing

### Code Style

- Use modern C++17/20 features where appropriate
- Follow RAII principles for resource management
- Use smart pointers instead of raw pointers
- Include comprehensive error handling
- Add unit tests for new functionality

