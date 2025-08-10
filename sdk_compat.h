#pragma once
/*
 * MacType SDK 호환성 헤더
 * 최신 Windows SDK와의 충돌을 방지하고 호환성을 보장합니다.
 */

// Windows SDK 버전 관련 충돌 방지
#ifndef _SDK_COMPAT_H_
#define _SDK_COMPAT_H_

// 1. 기본 전처리기 정의들
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif

// 2. Windows 버전 호환성 정의
#ifdef _WIN64
    #if !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0601)
        #undef _WIN32_WINNT
        #define _WIN32_WINNT 0x0A00  // Windows 10 (64비트는 현대적 버전 사용)
    #endif
    
    #if !defined(WINVER) || (WINVER < 0x0601)
        #undef WINVER
        #define WINVER 0x0A00
    #endif
#else
    #if !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0601)
        #undef _WIN32_WINNT
        #define _WIN32_WINNT 0x0601  // Windows 7 (32비트는 호환성 고려)
    #endif
    
    #if !defined(WINVER) || (WINVER < 0x0601)
        #undef WINVER
        #define WINVER 0x0601
    #endif
#endif

// 3. NTDDI 버전 안전 설정
#ifndef NTDDI_VERSION
    #ifdef _WIN64
        #define NTDDI_VERSION NTDDI_WIN10  // Windows 10
    #else
        #define NTDDI_VERSION NTDDI_WIN7   // Windows 7
    #endif
#endif

// 4. Windows SDK atomic 함수 충돌 방지
// 최신 SDK의 winnt.h에서 정의되는 atomic 함수들과의 이름 충돌 방지
#ifndef _WINNT_H_ATOMIC_CONFLICT_PREVENTION
#define _WINNT_H_ATOMIC_CONFLICT_PREVENTION

// winnt.h의 interlocked 내장 함수 비활성화 (충돌 방지)
#define _WINNT_H_DISABLE_INTERLOCKED_INTRINSICS

// 문제가 되는 atomic 함수들의 매크로 재정의 방지
#ifdef ReadAcquire
#undef ReadAcquire
#endif

#ifdef ReadNoFence  
#undef ReadNoFence
#endif

#ifdef ReadRaw
#undef ReadRaw
#endif

#ifdef WriteRelease
#undef WriteRelease
#endif

#ifdef WriteNoFence
#undef WriteNoFence
#endif

#ifdef WriteRaw
#undef WriteRaw
#endif

#endif // _WINNT_H_ATOMIC_CONFLICT_PREVENTION

// 5. 컴파일러별 호환성 정의
#if defined(_MSC_VER) && (_MSC_VER >= 1900)  // Visual Studio 2015+
    // 최신 컴파일러에서의 경고 억제
    #pragma warning(push)
    #pragma warning(disable: 4996)  // deprecated function warnings
    #pragma warning(disable: 4005)  // macro redefinition warnings
    #pragma warning(disable: 4267)  // size_t conversion warnings
    #pragma warning(disable: 4311)  // pointer truncation warnings
    #pragma warning(disable: 4302)  // type cast truncation warnings
#endif

// 6. 표준 라이브러리 호환성
#ifndef _ALLOW_RTCc_IN_STL
#define _ALLOW_RTCc_IN_STL
#endif

#ifndef _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#endif

#ifndef _SILENCE_CXX20_CODECVT_FACETS_DEPRECATION_WARNING
#define _SILENCE_CXX20_CODECVT_FACETS_DEPRECATION_WARNING
#endif

// 7. 매크로 충돌 방지
#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

// 8. 타입 호환성 정의
#ifndef LONG_PTR_DEFINED
#define LONG_PTR_DEFINED
#ifdef _WIN64
typedef __int64 LONG_PTR;
typedef unsigned __int64 ULONG_PTR;
#else
typedef long LONG_PTR;
typedef unsigned long ULONG_PTR;
#endif
#endif

// 9. 함수 호환성 선언
#ifndef FORCEINLINE
#if (_MSC_VER >= 1200)
#define FORCEINLINE __forceinline
#else
#define FORCEINLINE __inline
#endif
#endif

#ifndef NOP_FUNCTION
#if (_MSC_VER >= 1210)
#define NOP_FUNCTION __noop
#else
#define NOP_FUNCTION (void)0
#endif
#endif

// 10. DirectWrite 및 D2D 호환성
#ifndef D2D1_ALPHA_MODE_IGNORE
#define D2D1_ALPHA_MODE_IGNORE D2D1_ALPHA_MODE_IGNORE
#endif

#endif // _SDK_COMPAT_H_
