/*
 * MacType Secure Channel
 *
 * Encrypted inter-process communication
 * - Named Pipes with TLS 1.3 encryption
 * - Process authentication
 * - Secure data serialization
 */

#pragma once

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Security.Cryptography.h>
#include <winrt/Windows.Security.Cryptography.Core.h>
#include <memory>
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>

// Windows API for Named Pipes
#include <windows.h>
#include <lmcons.h>

namespace MacType::Agent {

    // Forward declarations
    class ChannelMessage;

    // Message types
    enum class MessageType {
        InjectionRequest,
        InjectionComplete,
        ConfigurationUpdate,
        HealthCheck,
        DiagnosticsRequest,
        DiagnosticsResponse,
        ErrorReport,
        ShutdownRequest,
        Custom
    };

    // Message priority
    enum class MessagePriority {
        Low,
        Normal,
        High,
        Critical
    };

    // Secure message header
    struct MessageHeader {
        uint32_t messageId;
        MessageType type;
        MessagePriority priority;
        uint32_t payloadSize;
        uint64_t timestamp;
        std::array<uint8_t, 32> checksum; // SHA256 checksum
        std::array<uint8_t, 16> nonce;    // AES-GCM nonce
    };

    // Channel configuration
    struct ChannelConfig {
        std::wstring pipeName = L"\\\\.\\pipe\\MacType.Agent.Channel";
        uint32_t maxConnections = 10;
        uint32_t connectionTimeoutMs = 5000;
        uint32_t maxMessageSize = 1024 * 1024; // 1MB
        bool enableEncryption = true;
        bool enableCompression = true;
        uint32_t heartbeatIntervalMs = 30000; // 30 seconds
    };

    // Connection state
    enum class ConnectionState {
        Disconnected,
        Connecting,
        Connected,
        Authenticating,
        Authenticated,
        Error,
        Closing
    };

    // Secure channel main class
    class SecureChannel {
    private:
        // Windows Runtime components
        winrt::Windows::Storage::Streams::DataReader m_dataReader{ nullptr };
        winrt::Windows::Storage::Streams::DataWriter m_dataWriter{ nullptr };
        winrt::Windows::Storage::Streams::StreamSocket m_socket{ nullptr };

        // Named Pipe components
        HANDLE m_pipeHandle{ INVALID_HANDLE_VALUE };
        OVERLAPPED m_readOverlapped{ 0 };
        OVERLAPPED m_writeOverlapped{ 0 };
        std::atomic<bool> m_isServer{ false };
        std::thread m_listenThread;

        // Cryptography components
        winrt::Windows::Security::Cryptography::Core::SymmetricKeyAlgorithmProvider m_aesProvider{ nullptr };
        winrt::Windows::Security::Cryptography::Core::HashAlgorithmProvider m_hashProvider{ nullptr };
        winrt::Windows::Security::Cryptography::Core::CryptographicKey m_sessionKey{ nullptr };

        // Channel state
        ConnectionState m_state = ConnectionState::Disconnected;
        ChannelConfig m_config;

        // Message handling
        std::queue<std::unique_ptr<ChannelMessage>> m_sendQueue;
        std::mutex m_queueMutex;
        uint32_t m_nextMessageId = 1;

        // Authentication
        std::vector<uint8_t> m_clientCertificate;
        winrt::Windows::Foundation::DateTime m_lastHeartbeat;

        // Event handlers
        winrt::event_token m_connectionLostToken;
        winrt::event_token m_messageReceivedToken;

    public:
        SecureChannel();
        ~SecureChannel();

        // Prevent copying
        SecureChannel(const SecureChannel&) = delete;
        SecureChannel& operator=(const SecureChannel&) = delete;

        // Connection management
        winrt::Windows::Foundation::IAsyncAction ConnectAsync();
        winrt::Windows::Foundation::IAsyncAction DisconnectAsync();
        winrt::Windows::Foundation::IAsyncAction ReconnectAsync();

        // Authentication
        winrt::Windows::Foundation::IAsyncAction AuthenticateAsync();
        winrt::Windows::Foundation::IAsyncAction VerifyPeerCertificateAsync();

        // Message operations
        winrt::Windows::Foundation::IAsyncAction SendMessageAsync(std::unique_ptr<ChannelMessage> message);
        winrt::Windows::Foundation::IAsyncAction SendUrgentMessageAsync(std::unique_ptr<ChannelMessage> message);
        winrt::Windows::Foundation::IAsyncOperation<std::unique_ptr<ChannelMessage>> ReceiveMessageAsync();

        // Bulk operations
        winrt::Windows::Foundation::IAsyncAction SendMultipleMessagesAsync(std::vector<std::unique_ptr<ChannelMessage>> messages);
        winrt::Windows::Foundation::IAsyncAction FlushSendQueueAsync();

        // Status and diagnostics
        ConnectionState GetState() const noexcept { return m_state; }
        winrt::Windows::Foundation::IAsyncAction GetChannelStatisticsAsync();
        winrt::Windows::Foundation::IAsyncAction PingAsync();

        // Configuration
        void SetConfig(const ChannelConfig& config) { m_config = config; }
        const ChannelConfig& GetConfig() const noexcept { return m_config; }

    private:
        // Connection helpers
        winrt::Windows::Foundation::IAsyncAction EstablishConnectionAsync();
        winrt::Windows::Foundation::IAsyncAction SetupEncryptionAsync();
        winrt::Windows::Foundation::IAsyncAction PerformHandshakeAsync();

        // Message processing
        winrt::Windows::Foundation::IAsyncAction ProcessMessageQueueAsync();
        winrt::Windows::Foundation::IAsyncAction EncryptMessageAsync(std::unique_ptr<ChannelMessage>& message);
        winrt::Windows::Foundation::IAsyncAction DecryptMessageAsync(std::unique_ptr<ChannelMessage>& message);

        // Serialization
        winrt::Windows::Foundation::IAsyncOperation<std::vector<uint8_t>> SerializeMessageAsync(const ChannelMessage& message);
        winrt::Windows::Foundation::IAsyncOperation<std::unique_ptr<ChannelMessage>> DeserializeMessageAsync(winrt::array_view<uint8_t> data);

        // Error handling and recovery
        winrt::Windows::Foundation::IAsyncAction HandleConnectionErrorAsync(winrt::hresult error);
        winrt::Windows::Foundation::IAsyncAction HandleAuthenticationFailureAsync();
        winrt::Windows::Foundation::IAsyncAction HandleDecryptionErrorAsync();

        // Heartbeat and keepalive
        winrt::Windows::Foundation::IAsyncAction StartHeartbeatAsync();
        winrt::Windows::Foundation::IAsyncAction SendHeartbeatAsync();
        winrt::Windows::Foundation::IAsyncAction HandleHeartbeatTimeoutAsync();

        // Utility methods
        winrt::Windows::Foundation::IAsyncAction GenerateSessionKeyAsync();
        winrt::Windows::Foundation::IAsyncAction ValidateMessageIntegrityAsync(const ChannelMessage& message);
        std::vector<uint8_t> GenerateNonce();
        std::array<uint8_t, 32> CalculateChecksum(winrt::array_view<uint8_t> data);
    };

    // Message base class
    class ChannelMessage {
    protected:
        MessageHeader m_header;
        std::vector<uint8_t> m_payload;

    public:
        ChannelMessage(MessageType type, MessagePriority priority = MessagePriority::Normal);
        virtual ~ChannelMessage() = default;

        // Header access
        const MessageHeader& GetHeader() const noexcept { return m_header; }
        MessageHeader& GetHeader() noexcept { return m_header; }

        // Payload access
        const std::vector<uint8_t>& GetPayload() const noexcept { return m_payload; }
        std::vector<uint8_t>& GetPayload() noexcept { return m_payload; }

        // Type-specific methods
        virtual winrt::Windows::Foundation::IAsyncAction SerializeAsync() = 0;
        virtual winrt::Windows::Foundation::IAsyncAction DeserializeAsync(winrt::array_view<uint8_t> data) = 0;

        // Utility methods
        virtual std::string GetTypeName() const = 0;
        virtual bool IsValid() const noexcept { return !m_payload.empty(); }
    };

    // Specific message types
    class InjectionRequestMessage : public ChannelMessage {
    private:
        DWORD m_processId;
        std::wstring m_dllPath;
        std::vector<std::string> m_parameters;

    public:
        InjectionRequestMessage(DWORD processId, const std::wstring& dllPath);
        InjectionRequestMessage(DWORD processId, const std::wstring& dllPath, const std::vector<std::string>& parameters);

        // Accessors
        DWORD GetProcessId() const noexcept { return m_processId; }
        const std::wstring& GetDllPath() const noexcept { return m_dllPath; }
        const std::vector<std::string>& GetParameters() const noexcept { return m_parameters; }

        // Overrides
        winrt::Windows::Foundation::IAsyncAction SerializeAsync() override;
        winrt::Windows::Foundation::IAsyncAction DeserializeAsync(winrt::array_view<uint8_t> data) override;
        std::string GetTypeName() const override { return "InjectionRequest"; }
    };

    class InjectionCompleteMessage : public ChannelMessage {
    private:
        DWORD m_processId;
        bool m_success;
        uint64_t m_moduleBase;
        winrt::hresult m_errorCode;

    public:
        InjectionCompleteMessage(DWORD processId, bool success, uint64_t moduleBase = 0, winrt::hresult errorCode = S_OK);

        // Accessors
        DWORD GetProcessId() const noexcept { return m_processId; }
        bool GetSuccess() const noexcept { return m_success; }
        uint64_t GetModuleBase() const noexcept { return m_moduleBase; }
        winrt::hresult GetErrorCode() const noexcept { return m_errorCode; }

        // Overrides
        winrt::Windows::Foundation::IAsyncAction SerializeAsync() override;
        winrt::Windows::Foundation::IAsyncAction DeserializeAsync(winrt::array_view<uint8_t> data) override;
        std::string GetTypeName() const override { return "InjectionComplete"; }
    };

    // Channel factory for creating channels
    class SecureChannelFactory {
    public:
        static winrt::Windows::Foundation::IAsyncOperation<std::unique_ptr<SecureChannel>> CreateClientChannelAsync(const ChannelConfig& config);
        static winrt::Windows::Foundation::IAsyncOperation<std::unique_ptr<SecureChannel>> CreateServerChannelAsync(const ChannelConfig& config);

    private:
        static winrt::Windows::Foundation::IAsyncAction ValidateConfigAsync(const ChannelConfig& config);
        static std::unique_ptr<SecureChannel> CreateChannelWithConfig(const ChannelConfig& config);
    };

} // namespace MacType::Agent
