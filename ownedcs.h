#pragma once

#include "modern_cpp.h"
#include <windows.h>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <thread>
#include <condition_variable>

// 레거시 구조체 (하위 호환성을 위해 유지)
typedef struct _OWNED_CRITIAL_SECTION 
{
    int nOwner, nRequests, nRecursiveCount;
    HANDLE hEvent;
    CRITICAL_SECTION threadLock;
} OWNED_CRITIAL_SECTION, *POWNED_CRITIAL_SECTION;

// 레거시 함수들 (하위 호환성을 위해 유지)
void WINAPI InitializeOwnedCritialSection(POWNED_CRITIAL_SECTION cs);
void WINAPI DeleteOwnedCritialSection(POWNED_CRITIAL_SECTION cs);
void WINAPI EnterOwnedCritialSection(POWNED_CRITIAL_SECTION cs, WORD Owner);
void WINAPI LeaveOwnedCritialSection(POWNED_CRITIAL_SECTION cs, WORD Owner);

// 현대적 소유권 기반 뮤텍스 클래스
class OwnedMutex
{
private:
    mutable std::recursive_mutex mutex_;
    std::atomic<std::thread::id> owner_id_{ std::thread::id{} };
    std::atomic<int> recursion_count_{ 0 };
    std::atomic<int> request_count_{ 0 };

public:
    OwnedMutex() = default;
    ~OwnedMutex() = default;

    // 복사/이동 불가
    OwnedMutex(const OwnedMutex&) = delete;
    OwnedMutex& operator=(const OwnedMutex&) = delete;
    OwnedMutex(OwnedMutex&&) = delete;
    OwnedMutex& operator=(OwnedMutex&&) = delete;

    void lock() noexcept
    {
        const auto current_thread = std::this_thread::get_id();
        
        ++request_count_;
        mutex_.lock();
        
        owner_id_.store(current_thread);
        ++recursion_count_;
    }

    void unlock() noexcept
    {
        --recursion_count_;
        if (recursion_count_ == 0) {
            owner_id_.store(std::thread::id{});
        }
        
        mutex_.unlock();
        --request_count_;
    }

    [[nodiscard]] bool try_lock() noexcept
    {
        const auto current_thread = std::this_thread::get_id();
        
        if (mutex_.try_lock()) {
            ++request_count_;
            owner_id_.store(current_thread);
            ++recursion_count_;
            return true;
        }
        return false;
    }

    [[nodiscard]] std::thread::id get_owner() const noexcept
    {
        return owner_id_.load();
    }

    [[nodiscard]] int get_recursion_count() const noexcept
    {
        return recursion_count_.load();
    }

    [[nodiscard]] int get_request_count() const noexcept
    {
        return request_count_.load();
    }

    [[nodiscard]] bool is_owned_by_current_thread() const noexcept
    {
        return owner_id_.load() == std::this_thread::get_id();
    }
};

// RAII 기반 소유권 락 가드
class OwnedLockGuard
{
private:
    OwnedMutex* mutex_;

public:
    explicit OwnedLockGuard(OwnedMutex& mtx) noexcept : mutex_(&mtx)
    {
        mutex_->lock();
    }

    ~OwnedLockGuard() noexcept
    {
        if (mutex_) {
            mutex_->unlock();
        }
    }

    // 복사/이동 불가
    OwnedLockGuard(const OwnedLockGuard&) = delete;
    OwnedLockGuard& operator=(const OwnedLockGuard&) = delete;
    OwnedLockGuard(OwnedLockGuard&&) = delete;
    OwnedLockGuard& operator=(OwnedLockGuard&&) = delete;
};

// 현대적 크리티컬 섹션 관리 클래스
namespace mactype {
    
    enum class CriticalSectionType : int {
        FONTCACHE = 0,
        DCRELATION = 1,
        FREETYPE = 2,
        BITMAP = 3,
        LOGGING = 4,
        MAX_COUNT = 20
    };

    class CriticalSectionManager
    {
    private:
        static constexpr size_t MAX_SECTIONS = static_cast<size_t>(CriticalSectionType::MAX_COUNT);
        std::array<std::unique_ptr<OwnedMutex>, MAX_SECTIONS> sections_;
        std::once_flag initialized_;

        void InitializeInternal() noexcept
        {
            for (auto& section : sections_) {
                section = std::make_unique<OwnedMutex>();
            }
        }

    public:
        static CriticalSectionManager& Instance() noexcept
        {
            static CriticalSectionManager instance;
            return instance;
        }

        OwnedMutex& GetSection(CriticalSectionType type) noexcept
        {
            std::call_once(initialized_, [this] { InitializeInternal(); });
            
            const auto index = static_cast<size_t>(type);
            ASSERT(index < MAX_SECTIONS);
            return *sections_[index];
        }

        // RAII 기반 섹션 락
        [[nodiscard]] auto LockSection(CriticalSectionType type) noexcept
        {
            return OwnedLockGuard{ GetSection(type) };
        }
    };

    // 편의 매크로 (기존 코드 호환성)
    #define LOCK_SECTION(type) auto _lock = mactype::CriticalSectionManager::Instance().LockSection(mactype::CriticalSectionType::type)

} // namespace mactype
