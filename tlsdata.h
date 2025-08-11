#pragma once

#include "modern_cpp.h"
#include "array.h"
#include <memory>
#include <vector>
#include <mutex>

// 현대적 Thread Local Storage 클래스
template <typename T>
class ModernTlsData
{
private:
    static constexpr DWORD INVALID_TLS_VALUE = 0xffffffff;
    
    DWORD tls_index_;
    mutable std::mutex array_mutex_;
    mutable std::vector<std::unique_ptr<T>> managed_objects_;

public:
    ModernTlsData() : tls_index_(INVALID_TLS_VALUE) {}
    
    ~ModernTlsData() noexcept
    {
        ProcessTerm();
    }

    // 복사/이동 불가 (리소스 관리 클래스)
    ModernTlsData(const ModernTlsData&) = delete;
    ModernTlsData& operator=(const ModernTlsData&) = delete;
    ModernTlsData(ModernTlsData&&) = delete;
    ModernTlsData& operator=(ModernTlsData&&) = delete;

    // 현대적 포인터 반환 (nullopt 대신 nullptr 사용)
    [[nodiscard]] T* GetPtr() const noexcept
    {
        if (tls_index_ == INVALID_TLS_VALUE) {
            return nullptr;
        }

        if (auto* pT = static_cast<T*>(::TlsGetValue(tls_index_))) {
            return pT;
        }

        // 새 객체 생성 (스마트 포인터 사용)
        auto new_object = std::make_unique<T>();
        T* raw_ptr = new_object.get();

        if (!::TlsSetValue(tls_index_, raw_ptr)) {
            return nullptr;
        }

        // 스레드 안전한 배열 관리
        {
            std::lock_guard<std::mutex> lock(array_mutex_);
            managed_objects_.emplace_back(std::move(new_object));
        }

        return raw_ptr;
    }

    // 옵셔널 기반 안전한 접근
    [[nodiscard]] std::optional<std::reference_wrapper<T>> TryGet() const noexcept
    {
        if (auto* ptr = GetPtr()) {
            return std::ref(*ptr);
        }
        return std::nullopt;
    }
    [[nodiscard]] bool ProcessInit() noexcept
    {
        tls_index_ = ::TlsAlloc();
        if (tls_index_ == INVALID_TLS_VALUE) {
            return false;
        }
        return true;
    }

    void ProcessTerm() noexcept
    {
        if (tls_index_ == INVALID_TLS_VALUE) {
            return;
        }

        ThreadTerm(); // 현재 스레드 정리
        ::TlsFree(tls_index_);
        tls_index_ = INVALID_TLS_VALUE;

        // 스마트 포인터를 사용하므로 자동으로 메모리 해제됨
        {
            std::lock_guard<std::mutex> lock(array_mutex_);
            managed_objects_.clear();
        }
    }

    void ThreadTerm() noexcept
    {
        if (tls_index_ == INVALID_TLS_VALUE) {
            return;
        }

        auto* pT = static_cast<T*>(::TlsGetValue(tls_index_));
        if (pT) {
            // 해당 포인터를 managed_objects_에서 제거
            {
                std::lock_guard<std::mutex> lock(array_mutex_);
                managed_objects_.erase(
                    std::remove_if(managed_objects_.begin(), managed_objects_.end(),
                        [pT](const std::unique_ptr<T>& ptr) { 
                            return ptr.get() == pT; 
                        }),
                    managed_objects_.end());
            }
            ::TlsSetValue(tls_index_, nullptr);
        }
    }
};

// 레거시 호환성을 위한 typedef (기존 코드와의 호환성)
template <typename T>
using CTlsData = ModernTlsData<T>;
