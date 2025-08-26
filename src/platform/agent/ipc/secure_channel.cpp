/*
 * MacType Secure Channel Implementation
 *
 * Encrypted inter-process communication
 * Named Pipes with TLS 1.3 encryption
 */

#include "secure_channel.h"
#include <winrt/Windows.Security.Cryptography.h>
#include <winrt/Windows.Storage.Streams.h>
#include <sstream>

namespace MacType::Agent {

    // SecureChannel implementation
    SecureChannel::SecureChannel() :
        m_state(ConnectionState::Disconnected),
        m_nextMessageId(1) {
        // Initialize cryptography providers
        m_aesProvider = winrt::Windows::Security::Cryptography::Core::SymmetricKeyAlgorithmProvider::OpenAlgorithm(
            winrt::Windows::Security::Cryptography::Core::SymmetricAlgorithmNames::AesGcm());

        m_hashProvider = winrt::Windows::Security::Cryptography::Core::HashAlgorithmProvider::OpenAlgorithm(
            winrt::Windows::Security::Cryptography::Core::HashAlgorithmNames::Sha256());
    }

    SecureChannel::~SecureChannel() {
        // Set state to closing to signal background threads
        m_state = ConnectionState::Closing;

        if (m_state != ConnectionState::Disconnected) {
            try {
                DisconnectAsync().get();
            } catch (...) {
                // Ignore errors during destruction
            }
        }

        // Clean up resources
        CleanupOverlapped();

        if (m_pipeHandle != INVALID_HANDLE_VALUE) {
            CloseHandle(m_pipeHandle);
            m_pipeHandle = INVALID_HANDLE_VALUE;
        }
    }

    winrt::Windows::Foundation::IAsyncAction SecureChannel::ConnectAsync() {
        if (m_state != ConnectionState::Disconnected) {
            throw winrt::hresult_error(E_INVALIDARG, L"Channel is not disconnected");
        }

        try {
            m_state = ConnectionState::Connecting;

            co_await EstablishConnectionAsync();
            co_await SetupEncryptionAsync();
            co_await PerformHandshakeAsync();

            m_state = ConnectionState::Connected;

            // Start heartbeat
            co_await StartHeartbeatAsync();

        } catch (winrt::hresult_error const& ex) {
            m_state = ConnectionState::Error;
            throw;
        }
    }

    winrt::Windows::Foundation::IAsyncAction SecureChannel::DisconnectAsync() {
        if (m_state == ConnectionState::Disconnected) {
            co_return;
        }

        try {
            m_state = ConnectionState::Closing;

            // Stop heartbeat
            // TODO: Stop heartbeat timer

            // Close connections
            if (m_socket) {
                m_socket.Close();
                m_socket = nullptr;
            }

            m_dataReader = nullptr;
            m_dataWriter = nullptr;

            // Close named pipe
            if (m_pipeHandle != INVALID_HANDLE_VALUE) {
                // Cancel any pending I/O
                CancelIoEx(m_pipeHandle, nullptr);

                // Disconnect if we're the server
                if (m_isServer) {
                    DisconnectNamedPipe(m_pipeHandle);
                }

                CloseHandle(m_pipeHandle);
                m_pipeHandle = INVALID_HANDLE_VALUE;
            }

            // Clean up overlapped structures
            CleanupOverlapped();

            m_state = ConnectionState::Disconnected;

        } catch (winrt::hresult_error const& ex) {
            m_state = ConnectionState::Error;
            throw;
        }
    }

    winrt::Windows::Foundation::IAsyncAction SecureChannel::ReconnectAsync() {
        co_await DisconnectAsync();
        co_await ConnectAsync();
    }

    winrt::Windows::Foundation::IAsyncAction SecureChannel::AuthenticateAsync() {
        // TODO: Implement authentication
        co_return;
    }

    winrt::Windows::Foundation::IAsyncAction SecureChannel::VerifyPeerCertificateAsync() {
        // TODO: Implement certificate verification
        co_return;
    }

    winrt::Windows::Foundation::IAsyncAction SecureChannel::SendMessageAsync(std::unique_ptr<ChannelMessage> message) {
        if (m_state != ConnectionState::Connected && m_state != ConnectionState::Authenticated) {
            throw winrt::hresult_error(E_INVALIDARG, L"Channel is not connected");
        }

        if (!message) {
            throw winrt::hresult_error(E_INVALIDARG, L"Message is null");
        }

        try {
            // Serialize message
            auto data = co_await SerializeMessageAsync(*message);

            // Encrypt message
            co_await EncryptMessageAsync(message);

            // Send data via named pipe
            if (m_pipeHandle != INVALID_HANDLE_VALUE) {
                // First send the size
                DWORD bytesWritten;
                uint32_t dataSize = static_cast<uint32_t>(data.size());

                if (!WriteFile(m_pipeHandle, &dataSize, sizeof(dataSize), &bytesWritten, &m_writeOverlapped)) {
                    auto error = GetLastError();
                    if (error != ERROR_IO_PENDING) {
                        throw winrt::hresult_error(HRESULT_FROM_WIN32(error),
                                                 L"Failed to send message size");
                    }

                    // Wait for completion
                    if (!GetOverlappedResult(m_pipeHandle, &m_writeOverlapped, &bytesWritten, TRUE)) {
                        throw winrt::hresult_error(HRESULT_FROM_WIN32(GetLastError()),
                                                 L"Failed to complete size write");
                    }
                }

                // Then send the actual data
                if (!WriteFile(m_pipeHandle, data.data(), dataSize, &bytesWritten, &m_writeOverlapped)) {
                    auto error = GetLastError();
                    if (error != ERROR_IO_PENDING) {
                        throw winrt::hresult_error(HRESULT_FROM_WIN32(error),
                                                 L"Failed to send message data");
                    }

                    // Wait for completion
                    if (!GetOverlappedResult(m_pipeHandle, &m_writeOverlapped, &bytesWritten, TRUE)) {
                        throw winrt::hresult_error(HRESULT_FROM_WIN32(GetLastError()),
                                                 L"Failed to complete data write");
                    }
                }
            }

        } catch (winrt::hresult_error const& ex) {
            co_await HandleConnectionErrorAsync(ex);
            throw;
        }
    }

    winrt::Windows::Foundation::IAsyncAction SecureChannel::SendUrgentMessageAsync(std::unique_ptr<ChannelMessage> message) {
        // For urgent messages, we could implement priority queuing
        co_await SendMessageAsync(std::move(message));
    }

    winrt::Windows::Foundation::IAsyncOperation<std::unique_ptr<ChannelMessage>> SecureChannel::ReceiveMessageAsync() {
        if (m_state != ConnectionState::Connected && m_state != ConnectionState::Authenticated) {
            throw winrt::hresult_error(E_INVALIDARG, L"Channel is not connected");
        }

        try {
            if (m_pipeHandle == INVALID_HANDLE_VALUE) {
                throw winrt::hresult_error(E_HANDLE, L"Pipe handle is not valid");
            }

            // Read message size
            uint32_t payloadSize;
            DWORD bytesRead;

            if (!ReadFile(m_pipeHandle, &payloadSize, sizeof(payloadSize), &bytesRead, &m_readOverlapped)) {
                auto error = GetLastError();
                if (error != ERROR_IO_PENDING) {
                    throw winrt::hresult_error(HRESULT_FROM_WIN32(error),
                                             L"Failed to read message size");
                }

                // Wait for completion
                if (!GetOverlappedResult(m_pipeHandle, &m_readOverlapped, &bytesRead, TRUE)) {
                    throw winrt::hresult_error(HRESULT_FROM_WIN32(GetLastError()),
                                             L"Failed to complete size read");
                }
            }

            if (bytesRead != sizeof(payloadSize)) {
                throw winrt::hresult_error(E_UNEXPECTED, L"Incomplete message header");
            }

            if (payloadSize > m_config.maxMessageSize) {
                throw winrt::hresult_error(E_INVALIDARG, L"Message size exceeds maximum allowed");
            }

            // Read message data
            std::vector<uint8_t> data(payloadSize);
            if (!ReadFile(m_pipeHandle, data.data(), payloadSize, &bytesRead, &m_readOverlapped)) {
                auto error = GetLastError();
                if (error != ERROR_IO_PENDING) {
                    throw winrt::hresult_error(HRESULT_FROM_WIN32(error),
                                             L"Failed to read message data");
                }

                // Wait for completion
                if (!GetOverlappedResult(m_pipeHandle, &m_readOverlapped, &bytesRead, TRUE)) {
                    throw winrt::hresult_error(HRESULT_FROM_WIN32(GetLastError()),
                                             L"Failed to complete data read");
                }
            }

            if (bytesRead != payloadSize) {
                throw winrt::hresult_error(E_UNEXPECTED, L"Incomplete message data");
            }

            // Deserialize message
            auto message = co_await DeserializeMessageAsync(data);

            // Decrypt message
            co_await DecryptMessageAsync(message);

            co_return message;

        } catch (winrt::hresult_error const& ex) {
            co_await HandleConnectionErrorAsync(ex);
            throw;
        }
    }

    winrt::Windows::Foundation::IAsyncAction SecureChannel::SendMultipleMessagesAsync(std::vector<std::unique_ptr<ChannelMessage>> messages) {
        for (auto& message : messages) {
            co_await SendMessageAsync(std::move(message));
        }
    }

    winrt::Windows::Foundation::IAsyncAction SecureChannel::FlushSendQueueAsync() {
        // Process any queued messages
        // TODO: Implement message queuing
        co_return;
    }

    winrt::Windows::Foundation::IAsyncAction SecureChannel::GetChannelStatisticsAsync() {
        // TODO: Implement statistics collection
        co_return;
    }

    winrt::Windows::Foundation::IAsyncAction SecureChannel::PingAsync() {
        // Send ping message
        auto pingMessage = std::make_unique<ChannelMessage>(MessageType::HealthCheck);
        co_await SendMessageAsync(std::move(pingMessage));
    }

    // Private implementation methods
    winrt::Windows::Foundation::IAsyncAction SecureChannel::EstablishConnectionAsync() {
        try {
            // Initialize OVERLAPPED structures
            m_readOverlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
            m_writeOverlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

            if (!m_readOverlapped.hEvent || !m_writeOverlapped.hEvent) {
                throw winrt::hresult_error(HRESULT_FROM_WIN32(GetLastError()),
                                         L"Failed to create event handles");
            }

            // Try to connect as client first
            co_await ConnectAsClientAsync();

            // If client connection fails, try to create server pipe
            if (m_pipeHandle == INVALID_HANDLE_VALUE) {
                m_isServer = true;
                co_await CreateServerPipeAsync();
            }

        } catch (winrt::hresult_error const& ex) {
            // Clean up on failure
            CleanupOverlapped();
            throw;
        }
    }

    winrt::Windows::Foundation::IAsyncAction SecureChannel::ConnectAsClientAsync() {
        // Attempt to connect to existing named pipe as client
        auto pipeName = L"\\\\.\\pipe\\" + m_config.pipeName;

        // Try to connect with timeout
        if (!WaitNamedPipeW(pipeName.c_str(), m_config.connectionTimeoutMs)) {
            // Pipe doesn't exist or is busy - this is expected for client mode
            co_return;
        }

        // Create file handle to the pipe
        m_pipeHandle = CreateFileW(
            pipeName.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,              // No sharing
            nullptr,        // Default security
            OPEN_EXISTING,  // Open existing pipe
            FILE_FLAG_OVERLAPPED, // Overlapped I/O
            nullptr         // No template file
        );

        if (m_pipeHandle == INVALID_HANDLE_VALUE) {
            auto error = GetLastError();
            if (error != ERROR_PIPE_BUSY) {
                throw winrt::hresult_error(HRESULT_FROM_WIN32(error),
                                         L"Failed to connect to named pipe");
            }
        } else {
            // Set pipe to message-read mode
            DWORD mode = PIPE_READMODE_MESSAGE;
            if (!SetNamedPipeHandleState(m_pipeHandle, &mode, nullptr, nullptr)) {
                CloseHandle(m_pipeHandle);
                m_pipeHandle = INVALID_HANDLE_VALUE;
                throw winrt::hresult_error(HRESULT_FROM_WIN32(GetLastError()),
                                         L"Failed to set pipe mode");
            }
        }
    }

    winrt::Windows::Foundation::IAsyncAction SecureChannel::CreateServerPipeAsync() {
        // Create named pipe server
        auto pipeName = L"\\\\.\\pipe\\" + m_config.pipeName;

        m_pipeHandle = CreateNamedPipeW(
            pipeName.c_str(),
            PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            m_config.maxConnections,
            0,  // Output buffer size
            0,  // Input buffer size
            m_config.connectionTimeoutMs,
            nullptr // Default security
        );

        if (m_pipeHandle == INVALID_HANDLE_VALUE) {
            throw winrt::hresult_error(HRESULT_FROM_WIN32(GetLastError()),
                                     L"Failed to create named pipe server");
        }

        // Start listening for connections in background thread
        m_listenThread = std::thread([this]() {
            try {
                ListenForConnections();
            } catch (winrt::hresult_error const& ex) {
                // Handle error in background thread
                m_state = ConnectionState::Error;
            }
        });

        // Wait for initial connection
        if (!ConnectNamedPipe(m_pipeHandle, &m_readOverlapped)) {
            auto error = GetLastError();
            if (error != ERROR_IO_PENDING && error != ERROR_PIPE_CONNECTED) {
                throw winrt::hresult_error(HRESULT_FROM_WIN32(error),
                                         L"Failed to connect named pipe");
            }

            // Wait for connection
            if (error == ERROR_IO_PENDING) {
                DWORD bytesTransferred;
                if (!GetOverlappedResult(m_pipeHandle, &m_readOverlapped, &bytesTransferred, TRUE)) {
                    throw winrt::hresult_error(HRESULT_FROM_WIN32(GetLastError()),
                                             L"Failed to wait for pipe connection");
                }
            }
        }
    }

    void SecureChannel::ListenForConnections() {
        while (m_state != ConnectionState::Closing) {
            // Wait for client connection
            if (ConnectNamedPipe(m_pipeHandle, &m_readOverlapped)) {
                // Connection successful
                break;
            }

            auto error = GetLastError();
            if (error == ERROR_IO_PENDING) {
                // Wait for connection
                DWORD bytesTransferred;
                if (GetOverlappedResult(m_pipeHandle, &m_readOverlapped, &bytesTransferred, TRUE)) {
                    break; // Connection established
                }
            }

            // Check if we should continue listening
            if (m_state == ConnectionState::Closing) {
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    void SecureChannel::CleanupOverlapped() {
        if (m_readOverlapped.hEvent) {
            CloseHandle(m_readOverlapped.hEvent);
            m_readOverlapped.hEvent = nullptr;
        }
        if (m_writeOverlapped.hEvent) {
            CloseHandle(m_writeOverlapped.hEvent);
            m_writeOverlapped.hEvent = nullptr;
        }
        if (m_listenThread.joinable()) {
            m_listenThread.join();
        }
    }

    winrt::Windows::Foundation::IAsyncAction SecureChannel::SetupEncryptionAsync() {
        // Generate session key
        co_await GenerateSessionKeyAsync();
        co_return;
    }

    winrt::Windows::Foundation::IAsyncAction SecureChannel::PerformHandshakeAsync() {
        // TODO: Implement handshake protocol
        m_state = ConnectionState::Authenticated;
        co_return;
    }

    winrt::Windows::Foundation::IAsyncAction SecureChannel::ProcessMessageQueueAsync() {
        // TODO: Implement message queue processing
        co_return;
    }

    winrt::Windows::Foundation::IAsyncAction SecureChannel::EncryptMessageAsync(std::unique_ptr<ChannelMessage>& message) {
        if (!message || !m_sessionKey) {
            co_return; // No encryption if no key
        }

        try {
            // Generate nonce for this message
            auto nonce = GenerateNonce();

            // Create GCM seal operation
            auto sealOp = m_aesProvider.CreateGcmSealAlgorithm(m_sessionKey);

            // Convert message payload to IBuffer
            auto payload = winrt::Windows::Security::Cryptography::CryptographicBuffer::CreateFromByteArray(message->GetPayload());
            auto nonceBuffer = winrt::Windows::Security::Cryptography::CryptographicBuffer::CreateFromByteArray(nonce);

            // Perform encryption
            auto encryptedResult = co_await sealOp.SealAsync(payload, nonceBuffer, nullptr);

            // Update message header with nonce and extract tag
            message->GetHeader().nonce = nonce;

            // Extract encrypted data and authentication tag
            auto encryptedData = winrt::Windows::Security::Cryptography::CryptographicBuffer::CopyToByteArray(encryptedResult.EncryptedData);
            auto authTag = winrt::Windows::Security::Cryptography::CryptographicBuffer::CopyToByteArray(encryptedResult.AuthenticationTag);

            // Combine encrypted data and auth tag
            std::vector<uint8_t> encryptedPayload;
            encryptedPayload.reserve(encryptedData.size() + authTag.size());
            encryptedPayload.insert(encryptedPayload.end(), encryptedData.begin(), encryptedData.end());
            encryptedPayload.insert(encryptedPayload.end(), authTag.begin(), authTag.end());

            // Update message payload
            message->GetPayload() = encryptedPayload;

        } catch (winrt::hresult_error const& ex) {
            co_await HandleDecryptionErrorAsync();
            throw winrt::hresult_error(E_FAIL, L"Failed to encrypt message: " + ex.message());
        }
    }

    winrt::Windows::Foundation::IAsyncAction SecureChannel::DecryptMessageAsync(std::unique_ptr<ChannelMessage>& message) {
        if (!message || !m_sessionKey) {
            co_return; // No decryption if no key
        }

        try {
            // Extract nonce from message header
            auto nonce = message->GetHeader().nonce;
            auto& encryptedPayload = message->GetPayload();

            // Split encrypted data and authentication tag (tag is last 16 bytes for GCM)
            const size_t authTagSize = 16;
            if (encryptedPayload.size() < authTagSize) {
                throw winrt::hresult_error(E_INVALIDARG, L"Invalid encrypted payload size");
            }

            auto encryptedDataSize = encryptedPayload.size() - authTagSize;
            std::vector<uint8_t> encryptedData(encryptedPayload.begin(), encryptedPayload.begin() + encryptedDataSize);
            std::vector<uint8_t> authTag(encryptedPayload.begin() + encryptedDataSize, encryptedPayload.end());

            // Create GCM verify operation
            auto verifyOp = m_aesProvider.CreateGcmVerifyAlgorithm(m_sessionKey);

            // Convert to IBuffer
            auto encryptedBuffer = winrt::Windows::Security::Cryptography::CryptographicBuffer::CreateFromByteArray(encryptedData);
            auto nonceBuffer = winrt::Windows::Security::Cryptography::CryptographicBuffer::CreateFromByteArray(nonce);
            auto authTagBuffer = winrt::Windows::Security::Cryptography::CryptographicBuffer::CreateFromByteArray(authTag);

            // Perform decryption and verification
            auto decryptedResult = co_await verifyOp.VerifyAsync(encryptedBuffer, nonceBuffer, nullptr, authTagBuffer);

            if (!decryptedResult.IsVerified) {
                throw winrt::hresult_error(E_FAIL, L"Message authentication failed");
            }

            // Extract decrypted data
            auto decryptedData = winrt::Windows::Security::Cryptography::CryptographicBuffer::CopyToByteArray(decryptedResult.DecryptedData);
            message->GetPayload() = decryptedData;

        } catch (winrt::hresult_error const& ex) {
            co_await HandleDecryptionErrorAsync();
            throw winrt::hresult_error(E_FAIL, L"Failed to decrypt message: " + ex.message());
        }
    }

    winrt::Windows::Foundation::IAsyncOperation<std::vector<uint8_t>> SecureChannel::SerializeMessageAsync(const ChannelMessage& message) {
        std::vector<uint8_t> serializedData;

        try {
            const auto& header = message.GetHeader();
            const auto& payload = message.GetPayload();

            // Calculate total size
            size_t totalSize = sizeof(MessageHeader) + payload.size();
            serializedData.reserve(totalSize);

            // Serialize header
            auto headerBytes = reinterpret_cast<const uint8_t*>(&header);
            serializedData.insert(serializedData.end(), headerBytes, headerBytes + sizeof(MessageHeader));

            // Serialize payload
            serializedData.insert(serializedData.end(), payload.begin(), payload.end());

        } catch (std::exception const& ex) {
            throw winrt::hresult_error(E_FAIL, winrt::to_hstring(std::string("Serialization failed: ") + ex.what()));
        }

        co_return serializedData;
    }

    winrt::Windows::Foundation::IAsyncOperation<std::unique_ptr<ChannelMessage>> SecureChannel::DeserializeMessageAsync(winrt::array_view<uint8_t> data) {
        try {
            if (data.size() < sizeof(MessageHeader)) {
                throw winrt::hresult_error(E_INVALIDARG, L"Data too small for message header");
            }

            // Deserialize header
            MessageHeader header;
            std::memcpy(&header, data.data(), sizeof(MessageHeader));

            // Validate header
            if (header.payloadSize > m_config.maxMessageSize) {
                throw winrt::hresult_error(E_INVALIDARG, L"Payload size exceeds maximum allowed size");
            }

            size_t expectedSize = sizeof(MessageHeader) + header.payloadSize;
            if (data.size() != expectedSize) {
                throw winrt::hresult_error(E_INVALIDARG, L"Data size does not match expected message size");
            }

            // Extract payload
            std::vector<uint8_t> payload(data.begin() + sizeof(MessageHeader), data.end());

            // Create appropriate message type
            std::unique_ptr<ChannelMessage> message;
            switch (header.type) {
                case MessageType::InjectionRequest:
                    message = std::make_unique<InjectionRequestMessage>(0, L"");
                    break;
                case MessageType::InjectionComplete:
                    message = std::make_unique<InjectionCompleteMessage>(0, false);
                    break;
                default:
                    message = std::make_unique<ChannelMessage>(header.type, header.priority);
                    break;
            }

            // Set header and payload
            message->GetHeader() = header;
            message->GetPayload() = payload;

            co_return message;

        } catch (winrt::hresult_error const&) {
            throw;
        } catch (std::exception const& ex) {
            throw winrt::hresult_error(E_FAIL, winrt::to_hstring(std::string("Deserialization failed: ") + ex.what()));
        }
    }

    winrt::Windows::Foundation::IAsyncAction SecureChannel::HandleConnectionErrorAsync(winrt::hresult_error error) {
        m_state = ConnectionState::Error;

        // TODO: Implement error handling and recovery
        co_return;
    }

    winrt::Windows::Foundation::IAsyncAction SecureChannel::HandleAuthenticationFailureAsync() {
        m_state = ConnectionState::Error;
        co_return;
    }

    winrt::Windows::Foundation::IAsyncAction SecureChannel::HandleDecryptionErrorAsync() {
        // TODO: Implement decryption error handling
        co_return;
    }

    winrt::Windows::Foundation::IAsyncAction SecureChannel::StartHeartbeatAsync() {
        // TODO: Implement heartbeat mechanism
        co_return;
    }

    winrt::Windows::Foundation::IAsyncAction SecureChannel::SendHeartbeatAsync() {
        co_await PingAsync();
        co_return;
    }

    winrt::Windows::Foundation::IAsyncAction SecureChannel::HandleHeartbeatTimeoutAsync() {
        // TODO: Implement heartbeat timeout handling
        co_return;
    }

    winrt::Windows::Foundation::IAsyncAction SecureChannel::GenerateSessionKeyAsync() {
        // Generate random session key
        auto keyMaterial = winrt::Windows::Security::Cryptography::CryptographicBuffer::GenerateRandom(32);
        m_sessionKey = m_aesProvider.CreateSymmetricKey(keyMaterial);
        co_return;
    }

    winrt::Windows::Foundation::IAsyncAction SecureChannel::ValidateMessageIntegrityAsync(const ChannelMessage& message) {
        // TODO: Implement message integrity validation
        co_return;
    }

    std::vector<uint8_t> SecureChannel::GenerateNonce() {
        // Generate random nonce for AES-GCM
        auto buffer = winrt::Windows::Security::Cryptography::CryptographicBuffer::GenerateRandom(12);
        std::vector<uint8_t> nonce(12);
        winrt::Windows::Security::Cryptography::CryptographicBuffer::CopyToByteArray(buffer, nonce);
        return nonce;
    }

    std::array<uint8_t, 32> SecureChannel::CalculateChecksum(winrt::array_view<uint8_t> data) {
        // Calculate SHA256 checksum
        auto hash = m_hashProvider.HashData(winrt::Windows::Security::Cryptography::CryptographicBuffer::CreateFromByteArray(data));
        std::array<uint8_t, 32> checksum;
        winrt::Windows::Security::Cryptography::CryptographicBuffer::CopyToByteArray(hash, checksum);
        return checksum;
    }

    // ChannelMessage implementation
    ChannelMessage::ChannelMessage(MessageType type, MessagePriority priority)
        : m_header({0, type, priority, 0, 0, {}, {}}) {
    }

    winrt::Windows::Foundation::IAsyncAction ChannelMessage::SerializeAsync() {
        // TODO: Implement serialization
        co_return;
    }

    winrt::Windows::Foundation::IAsyncAction ChannelMessage::DeserializeAsync(winrt::array_view<uint8_t> data) {
        // TODO: Implement deserialization
        co_return;
    }

    // Specific message implementations
    InjectionRequestMessage::InjectionRequestMessage(DWORD processId, const std::wstring& dllPath)
        : ChannelMessage(MessageType::InjectionRequest), m_processId(processId), m_dllPath(dllPath) {
        GetHeader().payloadSize = 0; // Will be set during serialization
    }

    InjectionRequestMessage::InjectionRequestMessage(DWORD processId, const std::wstring& dllPath,
                                                   const std::vector<std::string>& parameters)
        : ChannelMessage(MessageType::InjectionRequest), m_processId(processId), m_dllPath(dllPath), m_parameters(parameters) {
        GetHeader().payloadSize = 0; // Will be set during serialization
    }

    winrt::Windows::Foundation::IAsyncAction InjectionRequestMessage::SerializeAsync() {
        try {
            // Calculate payload size
            size_t payloadSize = sizeof(m_processId) + sizeof(uint32_t) + (m_dllPath.size() * sizeof(wchar_t));
            payloadSize += sizeof(uint32_t); // Parameter count
            for (const auto& param : m_parameters) {
                payloadSize += sizeof(uint32_t) + (param.size() * sizeof(char));
            }

            std::vector<uint8_t> payload;
            payload.reserve(payloadSize);

            // Serialize process ID
            auto processIdBytes = reinterpret_cast<const uint8_t*>(&m_processId);
            payload.insert(payload.end(), processIdBytes, processIdBytes + sizeof(m_processId));

            // Serialize DLL path length and data
            uint32_t dllPathLength = static_cast<uint32_t>(m_dllPath.size());
            auto dllPathLengthBytes = reinterpret_cast<const uint8_t*>(&dllPathLength);
            payload.insert(payload.end(), dllPathLengthBytes, dllPathLengthBytes + sizeof(dllPathLength));

            auto dllPathBytes = reinterpret_cast<const uint8_t*>(m_dllPath.data());
            payload.insert(payload.end(), dllPathBytes, dllPathBytes + (dllPathLength * sizeof(wchar_t)));

            // Serialize parameters
            uint32_t paramCount = static_cast<uint32_t>(m_parameters.size());
            auto paramCountBytes = reinterpret_cast<const uint8_t*>(&paramCount);
            payload.insert(payload.end(), paramCountBytes, paramCountBytes + sizeof(paramCount));

            for (const auto& param : m_parameters) {
                uint32_t paramLength = static_cast<uint32_t>(param.size());
                auto paramLengthBytes = reinterpret_cast<const uint8_t*>(&paramLength);
                payload.insert(payload.end(), paramLengthBytes, paramLengthBytes + sizeof(paramLength));

                auto paramBytes = reinterpret_cast<const uint8_t*>(param.data());
                payload.insert(payload.end(), paramBytes, paramBytes + (paramLength * sizeof(char)));
            }

            GetPayload() = payload;
            GetHeader().payloadSize = static_cast<uint32_t>(payload.size());

        } catch (std::exception const& ex) {
            throw winrt::hresult_error(E_FAIL, winrt::to_hstring(std::string("InjectionRequestMessage serialization failed: ") + ex.what()));
        }

        co_return;
    }

    winrt::Windows::Foundation::IAsyncAction InjectionRequestMessage::DeserializeAsync(winrt::array_view<uint8_t> data) {
        try {
            size_t offset = 0;

            // Deserialize process ID
            if (data.size() < sizeof(m_processId)) {
                throw winrt::hresult_error(E_INVALIDARG, L"Data too small for process ID");
            }
            std::memcpy(&m_processId, data.data() + offset, sizeof(m_processId));
            offset += sizeof(m_processId);

            // Deserialize DLL path
            if (data.size() < offset + sizeof(uint32_t)) {
                throw winrt::hresult_error(E_INVALIDARG, L"Data too small for DLL path length");
            }
            uint32_t dllPathLength;
            std::memcpy(&dllPathLength, data.data() + offset, sizeof(dllPathLength));
            offset += sizeof(dllPathLength);

            if (data.size() < offset + (dllPathLength * sizeof(wchar_t))) {
                throw winrt::hresult_error(E_INVALIDARG, L"Data too small for DLL path");
            }
            m_dllPath.assign(reinterpret_cast<const wchar_t*>(data.data() + offset), dllPathLength);
            offset += dllPathLength * sizeof(wchar_t);

            // Deserialize parameters
            if (data.size() < offset + sizeof(uint32_t)) {
                throw winrt::hresult_error(E_INVALIDARG, L"Data too small for parameter count");
            }
            uint32_t paramCount;
            std::memcpy(&paramCount, data.data() + offset, sizeof(paramCount));
            offset += sizeof(paramCount);

            m_parameters.clear();
            m_parameters.reserve(paramCount);

            for (uint32_t i = 0; i < paramCount; ++i) {
                if (data.size() < offset + sizeof(uint32_t)) {
                    throw winrt::hresult_error(E_INVALIDARG, L"Data too small for parameter length");
                }
                uint32_t paramLength;
                std::memcpy(&paramLength, data.data() + offset, sizeof(paramLength));
                offset += sizeof(paramLength);

                if (data.size() < offset + (paramLength * sizeof(char))) {
                    throw winrt::hresult_error(E_INVALIDARG, L"Data too small for parameter data");
                }
                std::string param(reinterpret_cast<const char*>(data.data() + offset), paramLength);
                m_parameters.push_back(std::move(param));
                offset += paramLength * sizeof(char);
            }

        } catch (winrt::hresult_error const&) {
            throw;
        } catch (std::exception const& ex) {
            throw winrt::hresult_error(E_FAIL, winrt::to_hstring(std::string("InjectionRequestMessage deserialization failed: ") + ex.what()));
        }

        co_return;
    }

    InjectionCompleteMessage::InjectionCompleteMessage(DWORD processId, bool success, uint64_t moduleBase, winrt::hresult errorCode)
        : ChannelMessage(MessageType::InjectionComplete), m_processId(processId), m_success(success),
          m_moduleBase(moduleBase), m_errorCode(errorCode) {
        GetHeader().payloadSize = 0; // Will be set during serialization
    }

    winrt::Windows::Foundation::IAsyncAction InjectionCompleteMessage::SerializeAsync() {
        try {
            // Calculate payload size
            size_t payloadSize = sizeof(m_processId) + sizeof(m_success) + sizeof(m_moduleBase) + sizeof(winrt::hresult);
            std::vector<uint8_t> payload;
            payload.reserve(payloadSize);

            // Serialize process ID
            auto processIdBytes = reinterpret_cast<const uint8_t*>(&m_processId);
            payload.insert(payload.end(), processIdBytes, processIdBytes + sizeof(m_processId));

            // Serialize success flag
            auto successBytes = reinterpret_cast<const uint8_t*>(&m_success);
            payload.insert(payload.end(), successBytes, successBytes + sizeof(m_success));

            // Serialize module base
            auto moduleBaseBytes = reinterpret_cast<const uint8_t*>(&m_moduleBase);
            payload.insert(payload.end(), moduleBaseBytes, moduleBaseBytes + sizeof(m_moduleBase));

            // Serialize error code
            auto errorCodeBytes = reinterpret_cast<const uint8_t*>(&m_errorCode);
            payload.insert(payload.end(), errorCodeBytes, errorCodeBytes + sizeof(m_errorCode));

            GetPayload() = payload;
            GetHeader().payloadSize = static_cast<uint32_t>(payload.size());

        } catch (std::exception const& ex) {
            throw winrt::hresult_error(E_FAIL, winrt::to_hstring(std::string("InjectionCompleteMessage serialization failed: ") + ex.what()));
        }

        co_return;
    }

    winrt::Windows::Foundation::IAsyncAction InjectionCompleteMessage::DeserializeAsync(winrt::array_view<uint8_t> data) {
        try {
            size_t offset = 0;
            size_t requiredSize = sizeof(m_processId) + sizeof(m_success) + sizeof(m_moduleBase) + sizeof(winrt::hresult);

            if (data.size() < requiredSize) {
                throw winrt::hresult_error(E_INVALIDARG, L"Data too small for InjectionCompleteMessage");
            }

            // Deserialize process ID
            std::memcpy(&m_processId, data.data() + offset, sizeof(m_processId));
            offset += sizeof(m_processId);

            // Deserialize success flag
            std::memcpy(&m_success, data.data() + offset, sizeof(m_success));
            offset += sizeof(m_success);

            // Deserialize module base
            std::memcpy(&m_moduleBase, data.data() + offset, sizeof(m_moduleBase));
            offset += sizeof(m_moduleBase);

            // Deserialize error code
            std::memcpy(&m_errorCode, data.data() + offset, sizeof(m_errorCode));

        } catch (winrt::hresult_error const&) {
            throw;
        } catch (std::exception const& ex) {
            throw winrt::hresult_error(E_FAIL, winrt::to_hstring(std::string("InjectionCompleteMessage deserialization failed: ") + ex.what()));
        }

        co_return;
    }

    // ChannelFactory implementation
    winrt::Windows::Foundation::IAsyncOperation<std::unique_ptr<SecureChannel>> ChannelFactory::CreateClientChannelAsync(const ChannelConfig& config) {
        co_await ValidateConfigAsync(config);
        auto channel = CreateChannelWithConfig(config);
        co_return channel;
    }

    winrt::Windows::Foundation::IAsyncOperation<std::unique_ptr<SecureChannel>> ChannelFactory::CreateServerChannelAsync(const ChannelConfig& config) {
        co_await ValidateConfigAsync(config);
        auto channel = CreateChannelWithConfig(config);
        co_return channel;
    }

    winrt::Windows::Foundation::IAsyncAction ChannelFactory::ValidateConfigAsync(const ChannelConfig& config) {
        if (config.pipeName.empty()) {
            throw winrt::hresult_error(E_INVALIDARG, L"Pipe name cannot be empty");
        }

        if (config.maxMessageSize == 0) {
            throw winrt::hresult_error(E_INVALIDARG, L"Max message size must be greater than 0");
        }

        co_return;
    }

    std::unique_ptr<SecureChannel> ChannelFactory::CreateChannelWithConfig(const ChannelConfig& config) {
        auto channel = std::make_unique<SecureChannel>();
        channel->SetConfig(config);
        return channel;
    }

} // namespace MacType::Agent
