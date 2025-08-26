/*
 * MacType Agent Logger
 *
 * Structured logging for the modern agent
 * - Asynchronous logging
 * - Multiple log levels
 * - File and ETW logging
 */

#pragma once

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>
#include <memory>
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>

namespace MacType::Agent {

    // Log levels
    enum class LogLevel {
        Trace,
        Debug,
        Info,
        Warning,
        Error,
        Critical,
        Off
    };

    // Log entry structure
    struct LogEntry {
        winrt::Windows::Foundation::DateTime timestamp;
        LogLevel level;
        std::string component;
        std::string message;
        std::string category;
        uint32_t processId;
        uint32_t threadId;
        std::vector<std::pair<std::string, std::string>> metadata;

        LogEntry(LogLevel level, std::string_view component, std::string_view message);
        LogEntry(LogLevel level, std::string_view component, std::string_view message,
                std::string_view category);

        std::string ToString() const;
        std::string ToJson() const;
    };

    // Logger configuration
    struct LoggerConfig {
        std::wstring logFilePath = L"MacType.Agent.log";
        LogLevel consoleLevel = LogLevel::Info;
        LogLevel fileLevel = LogLevel::Debug;
        LogLevel etwLevel = LogLevel::Warning;
        uint32_t maxFileSizeMB = 10;
        uint32_t maxBackupFiles = 5;
        bool enableConsoleLogging = true;
        bool enableFileLogging = true;
        bool enableETWLogging = true;
        bool enableAsyncLogging = true;
        uint32_t flushIntervalMs = 1000; // 1 second
    };

    // Main logger class
    class AgentLogger {
    private:
        // Configuration
        LoggerConfig m_config;

        // File logging
        winrt::Windows::Storage::StorageFile m_logFile{ nullptr };
        winrt::Windows::Storage::Streams::DataWriter m_fileWriter{ nullptr };
        uint64_t m_currentFileSize = 0;

        // Async logging
        std::queue<std::unique_ptr<LogEntry>> m_logQueue;
        std::mutex m_queueMutex;
        std::atomic<bool> m_isRunning = false;
        winrt::Windows::Foundation::IAsyncAction m_loggingAction{ nullptr };
        winrt::Windows::System::Threading::ThreadPoolTimer m_flushTimer{ nullptr };

        // ETW provider (if available)
        void* m_etwProvider = nullptr;

        // Performance tracking
        uint64_t m_totalLogsProcessed = 0;
        uint64_t m_droppedLogsCount = 0;

    public:
        AgentLogger();
        ~AgentLogger();

        // Prevent copying
        AgentLogger(const AgentLogger&) = delete;
        AgentLogger& operator=(const AgentLogger&) = delete;

        // Initialization
        winrt::Windows::Foundation::IAsyncAction InitializeAsync();
        winrt::Windows::Foundation::IAsyncAction ShutdownAsync();

        // Logging methods
        winrt::Windows::Foundation::IAsyncAction LogAsync(LogLevel level, std::string_view component,
                                                         std::string_view message);
        winrt::Windows::Foundation::IAsyncAction LogAsync(LogLevel level, std::string_view component,
                                                         std::string_view message, std::string_view category);
        winrt::Windows::Foundation::IAsyncAction LogWithMetadataAsync(LogLevel level, std::string_view component,
                                                                     std::string_view message,
                                                                     const std::vector<std::pair<std::string, std::string>>& metadata);

        // Convenience methods
        winrt::Windows::Foundation::IAsyncAction TraceAsync(std::string_view component, std::string_view message);
        winrt::Windows::Foundation::IAsyncAction DebugAsync(std::string_view component, std::string_view message);
        winrt::Windows::Foundation::IAsyncAction InfoAsync(std::string_view component, std::string_view message);
        winrt::Windows::Foundation::IAsyncAction WarningAsync(std::string_view component, std::string_view message);
        winrt::Windows::Foundation::IAsyncAction ErrorAsync(std::string_view component, std::string_view message);
        winrt::Windows::Foundation::IAsyncAction CriticalAsync(std::string_view component, std::string_view message);

        // Configuration
        void SetConfig(const LoggerConfig& config);
        const LoggerConfig& GetConfig() const noexcept { return m_config; }

        // Status and statistics
        winrt::Windows::Foundation::IAsyncAction GetStatisticsAsync();
        bool IsQueueEmpty() const noexcept;
        size_t GetQueueSize() const noexcept;

        // File management
        winrt::Windows::Foundation::IAsyncAction RotateLogFileAsync();
        winrt::Windows::Foundation::IAsyncAction FlushAsync();

    private:
        // Core logging implementation
        winrt::Windows::Foundation::IAsyncAction ProcessLogQueueAsync();
        winrt::Windows::Foundation::IAsyncAction WriteToConsoleAsync(const LogEntry& entry);
        winrt::Windows::Foundation::IAsyncAction WriteToFileAsync(const LogEntry& entry);
        winrt::Windows::Foundation::IAsyncAction WriteToETWAsync(const LogEntry& entry);

        // File operations
        winrt::Windows::Foundation::IAsyncAction OpenLogFileAsync();
        winrt::Windows::Foundation::IAsyncAction CloseLogFileAsync();
        winrt::Windows::Foundation::IAsyncAction CheckFileRotationAsync();
        winrt::Windows::Foundation::IAsyncAction BackupLogFileAsync();

        // ETW operations
        winrt::Windows::Foundation::IAsyncAction InitializeETWAsync();
        winrt::Windows::Foundation::IAsyncAction ShutdownETWAsync();

        // Utility methods
        bool ShouldLog(LogLevel level, LogLevel configuredLevel) const noexcept;
        std::string FormatMessage(const LogEntry& entry) const;
        std::string GetCurrentTimestampString() const;
        std::string LogLevelToString(LogLevel level) const;
        LogLevel StringToLogLevel(const std::string& levelStr) const;

        // Queue management
        void EnqueueLogEntry(std::unique_ptr<LogEntry> entry);
        std::unique_ptr<LogEntry> DequeueLogEntry();
        winrt::Windows::Foundation::IAsyncAction ProcessQueueItemAsync(std::unique_ptr<LogEntry> entry);

        // Timer management
        winrt::Windows::Foundation::IAsyncAction StartFlushTimerAsync();
        winrt::Windows::Foundation::IAsyncAction StopFlushTimerAsync();
        winrt::Windows::Foundation::IAsyncAction FlushTimerCallbackAsync();
    };

    // Logger factory
    class LoggerFactory {
    public:
        static std::unique_ptr<AgentLogger> CreateLogger(const LoggerConfig& config = LoggerConfig());
        static std::unique_ptr<AgentLogger> CreateConsoleLogger();
        static std::unique_ptr<AgentLogger> CreateFileLogger(const std::wstring& filePath);
        static std::unique_ptr<AgentLogger> CreateETWLogger();

    private:
        static LoggerConfig CreateDefaultConfig();
        static winrt::Windows::Foundation::IAsyncAction ValidateConfigAsync(const LoggerConfig& config);
    };

    // Global logger instance
    extern std::unique_ptr<AgentLogger> g_logger;

    // Global logging macros for convenience
    #define LOG_TRACE(component, message) \
        if (g_logger) co_await g_logger->TraceAsync(component, message);

    #define LOG_DEBUG(component, message) \
        if (g_logger) co_await g_logger->DebugAsync(component, message);

    #define LOG_INFO(component, message) \
        if (g_logger) co_await g_logger->InfoAsync(component, message);

    #define LOG_WARNING(component, message) \
        if (g_logger) co_await g_logger->WarningAsync(component, message);

    #define LOG_ERROR(component, message) \
        if (g_logger) co_await g_logger->ErrorAsync(component, message);

    #define LOG_CRITICAL(component, message) \
        if (g_logger) co_await g_logger->CriticalAsync(component, message);

} // namespace MacType::Agent
