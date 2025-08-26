/*
 * MacType Agent Logger Implementation
 *
 * Asynchronous structured logging with multiple targets
 * File logging, console logging, and ETW support
 */

#include "logger.h"
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.System.Threading.h>
#include <sstream>
#include <iomanip>
#include <filesystem>

namespace MacType::Agent {

    // Global logger instance
    std::unique_ptr<AgentLogger> g_logger;

    // LogEntry implementation
    LogEntry::LogEntry(LogLevel level, std::string_view component, std::string_view message)
        : level(level), component(component), message(message), category(""),
          processId(GetCurrentProcessId()), threadId(GetCurrentThreadId()) {
        timestamp = winrt::clock::now();
    }

    LogEntry::LogEntry(LogLevel level, std::string_view component, std::string_view message,
                      std::string_view category)
        : level(level), component(component), message(message), category(category),
          processId(GetCurrentProcessId()), threadId(GetCurrentThreadId()) {
        timestamp = winrt::clock::now();
    }

    std::string LogEntry::ToString() const {
        std::stringstream ss;
        ss << "[" << LogLevelToString() << "] "
           << "[" << component << "] "
           << "[" << std::put_time(std::localtime(&timestamp), "%Y-%m-%d %H:%M:%S") << "] "
           << message;

        if (!category.empty()) {
            ss << " [" << category << "]";
        }

        return ss.str();
    }

    std::string LogEntry::ToJson() const {
        std::stringstream ss;
        ss << "{"
           << "\"timestamp\":\"" << std::put_time(std::localtime(&timestamp), "%Y-%m-%dT%H:%M:%S") << "\","
           << "\"level\":\"" << LogLevelToString() << "\","
           << "\"component\":\"" << component << "\","
           << "\"message\":\"" << message << "\","
           << "\"category\":\"" << category << "\","
           << "\"processId\":" << processId << ","
           << "\"threadId\":" << threadId
           << "}";

        return ss.str();
    }

    std::string LogEntry::LogLevelToString() const {
        switch (level) {
            case LogLevel::Trace: return "TRACE";
            case LogLevel::Debug: return "DEBUG";
            case LogLevel::Info: return "INFO";
            case LogLevel::Warning: return "WARNING";
            case LogLevel::Error: return "ERROR";
            case LogLevel::Critical: return "CRITICAL";
            case LogLevel::Off: return "OFF";
            default: return "UNKNOWN";
        }
    }

    // AgentLogger implementation
    AgentLogger::AgentLogger() :
        m_config(LoggerConfig{}),
        m_isRunning(false),
        m_totalLogsProcessed(0),
        m_droppedLogsCount(0) {
        // Initialize with default config
        m_config = LoggerConfig{};
    }

    AgentLogger::~AgentLogger() {
        // Ensure proper shutdown
        if (m_isRunning) {
            ShutdownAsync().get();
        }
    }

    winrt::Windows::Foundation::IAsyncAction AgentLogger::InitializeAsync() {
        if (m_isRunning) {
            co_return; // Already initialized
        }

        try {
            // Initialize hash provider for checksums
            m_hashProvider = winrt::Windows::Security::Cryptography::Core::HashAlgorithmProvider::OpenAlgorithm(
                winrt::Windows::Security::Cryptography::Core::HashAlgorithmNames::Sha256());

            // Initialize logging targets
            if (m_config.enableFileLogging) {
                co_await OpenLogFileAsync();
            }

            if (m_config.enableETWLogging) {
                co_await InitializeETWAsync();
            }

            // Start processing queue
            m_isRunning = true;
            co_await StartFlushTimerAsync();

            // Initial log entry
            co_await InfoAsync("Logger", "Logger initialized successfully");

        } catch (winrt::hresult_error const& ex) {
            throw winrt::hresult_error(E_FAIL, L"Failed to initialize logger: " + ex.message());
        }
    }

    winrt::Windows::Foundation::IAsyncAction AgentLogger::ShutdownAsync() {
        if (!m_isRunning) {
            co_return;
        }

        m_isRunning = false;

        try {
            // Stop timer
            co_await StopFlushTimerAsync();

            // Process remaining queue items
            co_await ProcessLogQueueAsync();

            // Close resources
            if (m_config.enableFileLogging) {
                co_await CloseLogFileAsync();
            }

            if (m_config.enableETWLogging) {
                co_await ShutdownETWAsync();
            }

            // Final log (this won't be processed since we're shutting down)
            // But it's good practice to have it

        } catch (winrt::hresult_error const& ex) {
            // Log to console if possible
            if (m_config.enableConsoleLogging) {
                std::wcout << L"Error during logger shutdown: " << ex.message().c_str() << std::endl;
            }
        }
    }

    winrt::Windows::Foundation::IAsyncAction AgentLogger::LogAsync(LogLevel level, std::string_view component,
                                                                  std::string_view message) {
        co_await LogAsync(level, component, message, "");
    }

    winrt::Windows::Foundation::IAsyncAction AgentLogger::LogAsync(LogLevel level, std::string_view component,
                                                                  std::string_view message, std::string_view category) {
        if (!m_isRunning || !ShouldLog(level, GetConfiguredLevel())) {
            co_return;
        }

        auto entry = std::make_unique<LogEntry>(level, component, message, category);
        EnqueueLogEntry(std::move(entry));
        co_return;
    }

    winrt::Windows::Foundation::IAsyncAction AgentLogger::LogWithMetadataAsync(LogLevel level, std::string_view component,
                                                                             std::string_view message,
                                                                             const std::vector<std::pair<std::string, std::string>>& metadata) {
        if (!m_isRunning || !ShouldLog(level, GetConfiguredLevel())) {
            co_return;
        }

        auto entry = std::make_unique<LogEntry>(level, component, message);
        entry->metadata = metadata;

        // Add metadata to message
        std::string enhancedMessage = std::string(message);
        enhancedMessage += " [";
        for (const auto& [key, value] : metadata) {
            enhancedMessage += key + "=" + value + ",";
        }
        if (!metadata.empty()) {
            enhancedMessage.back() = ']'; // Replace last comma with closing bracket
        } else {
            enhancedMessage += "]";
        }

        entry->message = enhancedMessage;
        EnqueueLogEntry(std::move(entry));
        co_return;
    }

    winrt::Windows::Foundation::IAsyncAction AgentLogger::TraceAsync(std::string_view component, std::string_view message) {
        co_await LogAsync(LogLevel::Trace, component, message);
    }

    winrt::Windows::Foundation::IAsyncAction AgentLogger::DebugAsync(std::string_view component, std::string_view message) {
        co_await LogAsync(LogLevel::Debug, component, message);
    }

    winrt::Windows::Foundation::IAsyncAction AgentLogger::InfoAsync(std::string_view component, std::string_view message) {
        co_await LogAsync(LogLevel::Info, component, message);
    }

    winrt::Windows::Foundation::IAsyncAction AgentLogger::WarningAsync(std::string_view component, std::string_view message) {
        co_await LogAsync(LogLevel::Warning, component, message);
    }

    winrt::Windows::Foundation::IAsyncAction AgentLogger::ErrorAsync(std::string_view component, std::string_view message) {
        co_await LogAsync(LogLevel::Error, component, message);
    }

    winrt::Windows::Foundation::IAsyncAction AgentLogger::CriticalAsync(std::string_view component, std::string_view message) {
        co_await LogAsync(LogLevel::Critical, component, message);
    }

    void AgentLogger::SetConfig(const LoggerConfig& config) {
        m_config = config;
    }

    winrt::Windows::Foundation::IAsyncAction AgentLogger::GetStatisticsAsync() {
        // This would return detailed statistics about logging performance
        // For now, just basic info
        if (m_config.enableConsoleLogging) {
            std::cout << "Logger Statistics:" << std::endl;
            std::cout << "Total logs processed: " << m_totalLogsProcessed << std::endl;
            std::cout << "Dropped logs: " << m_droppedLogsCount << std::endl;
            std::cout << "Queue size: " << m_logQueue.size() << std::endl;
        }

        co_return;
    }

    bool AgentLogger::IsQueueEmpty() const noexcept {
        std::lock_guard lock(m_queueMutex);
        return m_logQueue.empty();
    }

    size_t AgentLogger::GetQueueSize() const noexcept {
        std::lock_guard lock(m_queueMutex);
        return m_logQueue.size();
    }

    winrt::Windows::Foundation::IAsyncAction AgentLogger::RotateLogFileAsync() {
        if (!m_config.enableFileLogging) {
            co_return;
        }

        try {
            co_await CloseLogFileAsync();
            co_await CheckFileRotationAsync();
            co_await OpenLogFileAsync();

        } catch (winrt::hresult_error const& ex) {
            // Log to console if file logging failed
            if (m_config.enableConsoleLogging) {
                std::wcout << L"Failed to rotate log file: " << ex.message().c_str() << std::endl;
            }
        }
    }

    winrt::Windows::Foundation::IAsyncAction AgentLogger::FlushAsync() {
        co_await ProcessLogQueueAsync();
    }

    // Private implementation methods
    winrt::Windows::Foundation::IAsyncAction AgentLogger::ProcessLogQueueAsync() {
        // Process all pending log entries
        while (!IsQueueEmpty()) {
            auto entry = DequeueLogEntry();
            if (entry) {
                co_await ProcessQueueItemAsync(std::move(entry));
            }
        }
    }

    winrt::Windows::Foundation::IAsyncAction AgentLogger::WriteToConsoleAsync(const LogEntry& entry) {
        if (!m_config.enableConsoleLogging) {
            co_return;
        }

        std::wcout << winrt::to_hstring(entry.ToString()).c_str() << std::endl;
        co_return;
    }

    winrt::Windows::Foundation::IAsyncAction AgentLogger::WriteToFileAsync(const LogEntry& entry) {
        if (!m_config.enableFileLogging || !m_fileWriter) {
            co_return;
        }

        try {
            std::string logLine = entry.ToString() + "\n";
            auto data = winrt::to_hstring(logLine);

            co_await m_fileWriter.WriteStringAsync(data);
            co_await m_fileWriter.StoreAsync();

        } catch (winrt::hresult_error const& ex) {
            // If file logging fails, we might want to disable it
            m_config.enableFileLogging = false;
        }
    }

    winrt::Windows::Foundation::IAsyncAction AgentLogger::WriteToETWAsync(const LogEntry& entry) {
        if (!m_config.enableETWLogging || !m_etwProvider) {
            co_return;
        }

        // TODO: Implement ETW logging
        // This would involve Windows Event Tracing for Windows (ETW)
        // For now, just return
        co_return;
    }

    winrt::Windows::Foundation::IAsyncAction AgentLogger::OpenLogFileAsync() {
        try {
            auto folder = winrt::Windows::Storage::ApplicationData::Current().LocalFolder();
            m_logFile = co_await folder.CreateFileAsync(
                winrt::to_hstring(m_config.logFilePath),
                winrt::Windows::Storage::CreationCollisionOption::ReplaceExisting);

            m_fileWriter = winrt::Windows::Storage::Streams::DataWriter(
                co_await m_logFile.OpenStreamForWriteAsync());

            // Write header
            auto header = winrt::to_hstring("MacType Agent Log - Started at " + GetCurrentTimestampString() + "\n");
            co_await m_fileWriter.WriteStringAsync(header);
            co_await m_fileWriter.Store();

        } catch (winrt::hresult_error const& ex) {
            m_config.enableFileLogging = false;
            throw winrt::hresult_error(E_FAIL, L"Failed to open log file: " + ex.message());
        }
    }

    winrt::Windows::Foundation::IAsyncAction AgentLogger::CloseLogFileAsync() {
        if (m_fileWriter) {
            co_await m_fileWriter.FlushAsync();
            m_fileWriter = nullptr;
        }
        m_logFile = nullptr;
    }

    winrt::Windows::Foundation::IAsyncAction AgentLogger::CheckFileRotationAsync() {
        if (!m_logFile) {
            co_return;
        }

        try {
            auto properties = co_await m_logFile.GetBasicPropertiesAsync();
            uint64_t currentSize = properties.Size();

            if (currentSize >= m_config.maxFileSizeMB * 1024 * 1024) {
                co_await BackupLogFileAsync();
            }

        } catch (winrt::hresult_error const& ex) {
            // Continue without rotation if it fails
        }
    }

    winrt::Windows::Foundation::IAsyncAction AgentLogger::BackupLogFileAsync() {
        if (!m_logFile) {
            co_return;
        }

        try {
            auto folder = co_await m_logFile.GetParentAsync();
            if (!folder) {
                co_return;
            }

            // Create backup filename with timestamp
            auto timestamp = winrt::clock::now();
            std::wstringstream backupName;
            backupName << m_config.logFilePath << L"." << std::chrono::duration_cast<std::chrono::seconds>(
                timestamp.time_since_epoch()).count() << L".bak";

            co_await m_logFile.RenameAsync(winrt::to_hstring(backupName.str()));

            // Clean up old backup files
            auto files = co_await folder.GetFilesAsync();
            std::vector<winrt::Windows::Storage::StorageFile> backupFiles;

            for (const auto& file : files) {
                auto name = winrt::to_string(file.Name());
                if (name.find(m_config.logFilePath + ".") == 0 && name.find(".bak") != std::string::npos) {
                    backupFiles.push_back(file);
                }
            }

            // Keep only the most recent backup files
            if (backupFiles.size() > m_config.maxBackupFiles) {
                std::sort(backupFiles.begin(), backupFiles.end(),
                    [](const winrt::Windows::Storage::StorageFile& a, const winrt::Windows::Storage::StorageFile& b) {
                        return a.DateCreated() < b.DateCreated();
                    });

                for (size_t i = 0; i < backupFiles.size() - m_config.maxBackupFiles; ++i) {
                    co_await backupFiles[i].DeleteAsync();
                }
            }

        } catch (winrt::hresult_error const& ex) {
            // Continue without backup if it fails
        }
    }

    winrt::Windows::Foundation::IAsyncAction AgentLogger::InitializeETWAsync() {
        // TODO: Initialize ETW provider
        // This would involve registering with Windows Event Tracing
        co_return;
    }

    winrt::Windows::Foundation::IAsyncAction AgentLogger::ShutdownETWAsync() {
        // TODO: Shutdown ETW provider
        m_etwProvider = nullptr;
        co_return;
    }

    bool AgentLogger::ShouldLog(LogLevel level, LogLevel configuredLevel) const noexcept {
        // Don't log if level is Off
        if (level == LogLevel::Off || configuredLevel == LogLevel::Off) {
            return false;
        }

        // Log if the message level is at or above the configured level
        return static_cast<int>(level) >= static_cast<int>(configuredLevel);
    }

    std::string AgentLogger::FormatMessage(const LogEntry& entry) const {
        return entry.ToString();
    }

    std::string AgentLogger::GetCurrentTimestampString() const {
        auto now = winrt::clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        return std::string(std::ctime(&time_t));
    }

    LogLevel AgentLogger::GetConfiguredLevel() const noexcept {
        // Return the most verbose level from enabled targets
        LogLevel level = LogLevel::Off;

        if (m_config.enableConsoleLogging) {
            level = std::min(level, m_config.consoleLevel);
        }
        if (m_config.enableFileLogging) {
            level = std::min(level, m_config.fileLevel);
        }
        if (m_config.enableETWLogging) {
            level = std::min(level, m_config.etwLevel);
        }

        return level;
    }

    void AgentLogger::EnqueueLogEntry(std::unique_ptr<LogEntry> entry) {
        std::lock_guard lock(m_queueMutex);

        // Check queue size limit (simple implementation)
        if (m_logQueue.size() >= 10000) { // Arbitrary limit
            ++m_droppedLogsCount;
            return; // Drop the log entry
        }

        m_logQueue.push(std::move(entry));
    }

    std::unique_ptr<LogEntry> AgentLogger::DequeueLogEntry() {
        std::lock_guard lock(m_queueMutex);

        if (m_logQueue.empty()) {
            return nullptr;
        }

        auto entry = std::move(m_logQueue.front());
        m_logQueue.pop();
        return entry;
    }

    winrt::Windows::Foundation::IAsyncAction AgentLogger::ProcessQueueItemAsync(std::unique_ptr<LogEntry> entry) {
        if (!entry) {
            co_return;
        }

        ++m_totalLogsProcessed;

        // Write to all enabled targets
        auto consoleTask = WriteToConsoleAsync(*entry);
        auto fileTask = WriteToFileAsync(*entry);
        auto etwTask = WriteToETWAsync(*entry);

        // Wait for all to complete
        co_await consoleTask;
        co_await fileTask;
        co_await etwTask;
    }

    winrt::Windows::Foundation::IAsyncAction AgentLogger::StartFlushTimerAsync() {
        if (!m_config.enableAsyncLogging) {
            co_return;
        }

        auto timerDelegate = [weak_this = std::weak_ptr<AgentLogger>(shared_from_this())]
            (winrt::Windows::System::Threading::ThreadPoolTimer const&) -> winrt::Windows::Foundation::IAsyncAction {
            if (auto strong_this = weak_this.lock()) {
                co_await strong_this->FlushTimerCallbackAsync();
            }
        };

        // Create periodic timer for flushing logs
        m_flushTimer = winrt::Windows::System::Threading::ThreadPoolTimer::CreatePeriodicTimer(
            winrt::Windows::Foundation::TimeSpan{ static_cast<int64_t>(m_config.flushIntervalMs) * 10000 }, // Convert to 100ns units
            timerDelegate
        );
    }

    winrt::Windows::Foundation::IAsyncAction AgentLogger::StopFlushTimerAsync() {
        if (m_flushTimer) {
            m_flushTimer.Cancel();
            m_flushTimer = nullptr;
        }
    }

    winrt::Windows::Foundation::IAsyncAction AgentLogger::FlushTimerCallbackAsync() {
        co_await ProcessLogQueueAsync();
    }

    // LoggerFactory implementation
    std::unique_ptr<AgentLogger> LoggerFactory::CreateLogger(const LoggerConfig& config) {
        auto logger = std::make_unique<AgentLogger>();
        logger->SetConfig(config);
        return logger;
    }

    std::unique_ptr<AgentLogger> LoggerFactory::CreateConsoleLogger() {
        LoggerConfig config = CreateDefaultConfig();
        config.enableFileLogging = false;
        config.enableETWLogging = false;
        config.enableConsoleLogging = true;
        return CreateLogger(config);
    }

    std::unique_ptr<AgentLogger> LoggerFactory::CreateFileLogger(const std::wstring& filePath) {
        LoggerConfig config = CreateDefaultConfig();
        config.enableConsoleLogging = false;
        config.enableETWLogging = false;
        config.enableFileLogging = true;
        config.logFilePath = filePath;
        return CreateLogger(config);
    }

    std::unique_ptr<AgentLogger> LoggerFactory::CreateETWLogger() {
        LoggerConfig config = CreateDefaultConfig();
        config.enableConsoleLogging = false;
        config.enableFileLogging = false;
        config.enableETWLogging = true;
        return CreateLogger(config);
    }

    LoggerConfig LoggerFactory::CreateDefaultConfig() {
        LoggerConfig config;
        config.enableConsoleLogging = true;
        config.enableFileLogging = true;
        config.enableETWLogging = false;
        config.enableAsyncLogging = true;
        config.flushIntervalMs = 1000;
        config.maxFileSizeMB = 10;
        config.maxBackupFiles = 5;
        return config;
    }

    winrt::Windows::Foundation::IAsyncAction LoggerFactory::ValidateConfigAsync(const LoggerConfig& config) {
        // Basic validation
        if (config.logFilePath.empty() && config.enableFileLogging) {
            throw winrt::hresult_error(E_INVALIDARG, L"Log file path cannot be empty when file logging is enabled");
        }

        if (config.maxFileSizeMB == 0 && config.enableFileLogging) {
            throw winrt::hresult_error(E_INVALIDARG, L"Max file size must be greater than 0 when file logging is enabled");
        }

        co_return;
    }

} // namespace MacType::Agent
