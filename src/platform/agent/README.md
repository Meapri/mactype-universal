# MacType Modern Agent (mt64agnt 2.0)

## Overview

The MacType Modern Agent is a complete rewrite of the legacy mt64agnt.exe, designed for modern Windows development using WinUI 3 and Windows App SDK. This agent provides secure cross-architecture communication and DLL injection capabilities without relying on the outdated wow64ext library.

## Key Features

### 🔒 Security First
- **Encrypted Communication**: All inter-process communication is encrypted using AES-256-GCM
- **Process Verification**: Comprehensive process integrity and signature verification
- **Secure Injection**: Memory-safe DLL injection with rollback capabilities
- **Access Control**: Granular permission management for injection operations

### 🏗️ Modern Architecture
- **WinUI 3 Native**: Built with Windows App SDK for optimal Windows 11 integration
- **Asynchronous Operations**: Fully async/await pattern for responsive performance
- **Modular Design**: Clean separation of concerns with independent components
- **RAII Resource Management**: Safe resource handling with automatic cleanup

### 🚀 Performance & Reliability
- **wow64ext Free**: Direct Windows API usage for better performance
- **Memory Efficient**: Optimized memory usage with smart pooling
- **Auto Recovery**: Automatic failure detection and recovery
- **Comprehensive Logging**: Structured logging with ETW support

### 🔧 Cross-Architecture Support
- **x64 Native**: Optimized for 64-bit Windows
- **ARM64 Ready**: ARM64 architecture support for future Windows devices
- **Backward Compatible**: Maintains compatibility with existing MacType ecosystem

## Architecture

```
src/platform/agent/
├── core/                    # Core agent functionality
│   ├── agent_controller.h   # Main agent lifecycle management
│   ├── process_bridge.h     # Cross-architecture process communication
│   └── ...
├── ipc/                     # Inter-process communication
│   ├── secure_channel.h     # Encrypted named pipe communication
│   └── ...
├── injection/               # DLL injection engine
│   ├── modern_injector.h    # Safe injection without wow64ext
│   └── ...
├── security/                # Security layer
│   ├── process_verifier.h   # Process integrity verification
│   ├── encryption.h         # AES encryption/decryption
│   └── ...
├── diagnostics/             # Monitoring and diagnostics
│   ├── logger.h             # Structured logging system
│   └── ...
├── arch/                    # Architecture-specific code
│   ├── x64_support.h        # x64 optimizations
│   └── arm64_support.h      # ARM64 support
└── utils/                   # Utility functions
    ├── winrt_helper.h       # WinRT/Windows API interoperability
    └── ...
```

## Components

### 1. Agent Controller (`agent_controller`)
The central orchestrator that manages the entire agent lifecycle:
- Service initialization and shutdown
- Component coordination
- Health monitoring and recovery
- Configuration management

### 2. Process Bridge (`process_bridge`)
Handles cross-architecture process communication:
- Secure process attachment/detachment
- Memory operations across architectures
- Thread context manipulation
- Architecture detection and adaptation

### 3. Secure Channel (`secure_channel`)
Provides encrypted inter-process communication:
- Named pipe with TLS 1.3 encryption
- Message queuing and prioritization
- Authentication and authorization
- Heartbeat and connection monitoring

### 4. Modern Injector (`modern_injector`)
Performs safe DLL injection without wow64ext.

## Architecture Evolution: From Legacy to Modern

### Legacy MacType Structure (wow64ext dependent)
The original MacType had three separate executables due to wow64ext limitations:

**Legacy Components:**
- `mt64agnt.exe` (32-bit) - Main agent with wow64ext for 64-bit control
- `MacLoader.exe` (32-bit) - 32-bit process loader
- `MacLoader64.exe` (64-bit) - 64-bit process loader

**Legacy Limitations:**
- Complex cross-architecture communication
- wow64ext dependency for stability issues
- Manual architecture detection and routing
- Three separate binaries to maintain

### Modern MacType Structure (wow64ext free)
Our modernized approach uses a single intelligent agent:

**Modern Components:**
- `mt64agnt.exe` (x64/ARM64) - Universal agent with native multi-architecture support
- `MacType*.dll` - Architecture-specific rendering engines
- *No separate loaders needed*

**Modern Advantages:**
- Single binary handles all architectures
- Native Windows API usage (no wow64ext)
- Automatic architecture detection
- Simplified deployment and maintenance

## Modern GUI Applications Plan

### MacWiz → Modern Setup Wizard (WinUI 3)
**Current Status**: Legacy Win32 installer wizard
**Target**: Modern native Windows setup experience

#### Key Features:
- **WinUI 3 Native UI**: Fluent Design with Mica/Acrylic backdrops
- **ARM64 Support**: Native ARM64 installation and configuration
- **Modern Package Management**: MSIX integration and auto-updates
- **Accessibility**: Narrator support and keyboard navigation
- **Dark Mode**: System theme integration

#### Architecture:
- **MVVM Pattern**: ViewModel-based data binding
- **JSON Configuration**: Modern settings format
- **Async Operations**: Non-blocking installation process
- **Progress Visualization**: Modern progress indicators
- **Error Recovery**: Intelligent rollback mechanisms

#### User Experience:
- **Welcome Screen**: Feature overview and system compatibility check
- **Installation Options**: Custom installation paths and component selection
- **Configuration Wizard**: Profile setup and font preferences
- **Service Integration**: Automatic service registration
- **Completion**: Quick start guide and troubleshooting tips

### MacTuner → Modern Settings App (WinUI 3)
**Current Status**: Legacy Win32 settings dialog
**Target**: Modern native Windows settings experience

#### Key Features:
- **Real-time Preview**: Live font rendering preview
- **Profile Management**: JSON-based profile system
- **Font Analysis**: Advanced font metrics and compatibility
- **Performance Monitoring**: System impact visualization
- **Batch Operations**: Bulk font configuration

#### Architecture:
- **Adaptive Layout**: Responsive design for all screen sizes
- **Settings Sync**: Cloud synchronization (optional)
- **Plugin System**: Extensible font processing modules
- **Advanced Diagnostics**: System font analysis tools
- **Export/Import**: Configuration backup and sharing

#### Core Modules:
1. **Font Scanner**: System font inventory and analysis
2. **Profile Editor**: Visual profile creation and editing
3. **Performance Monitor**: Real-time rendering statistics
4. **Compatibility Checker**: Application-specific font testing
5. **Backup Manager**: Configuration versioning and recovery

### Implementation Roadmap

#### Phase 1: Foundation (4-6 weeks)
- WinUI 3 project setup and basic architecture
- MVVM framework implementation
- JSON configuration system
- Basic UI components development

#### Phase 2: Core Features (6-8 weeks)
- Installation wizard implementation
- Settings app core functionality
- Font analysis and preview systems
- ARM64 compatibility and testing

#### Phase 3: Advanced Features (4-6 weeks)
- Cloud synchronization
- Advanced diagnostics
- Plugin system implementation
- Performance optimization

#### Phase 4: Testing & Polish (4-6 weeks)
- Comprehensive testing across platforms
- User experience refinement
- Documentation and localization
- Beta testing and feedback integration

### Technical Specifications

#### WinUI 3 Requirements:
- Windows 10 version 1903+ or Windows 11
- Windows App SDK 1.3+
- C++/WinRT for native interop
- Windows Runtime APIs

#### Architecture Principles:
- **Modern Async**: All I/O operations asynchronous
- **Error Boundaries**: Comprehensive error handling
- **Accessibility First**: WCAG 2.1 AA compliance
- **Performance Focused**: <100ms UI responsiveness
- **Security Conscious**: Least privilege execution

### Migration Strategy

#### Phase 1: Coexistence
- Legacy and modern apps can coexist
- Gradual migration path for users
- Backward compatibility maintained

#### Phase 2: Feature Parity
- Modern app reaches feature parity with legacy
- User testing and feedback integration
- Performance benchmarking vs legacy

#### Phase 3: Migration Tools
- Automatic settings migration
- User preference transfer
- Uninstall guidance for legacy

#### Phase 4: Deprecation
- Legacy app marked as deprecated
- Clear migration path provided
- Support timeline communicated

This modernization plan ensures MacType remains competitive in the modern Windows ecosystem while providing users with the best possible experience across all supported platforms.
- Multiple injection methods (LoadLibrary, Manual Map, Reflective)
- DLL signature verification
- Memory integrity checks
- Injection rollback and cleanup

### 5. Diagnostics Logger (`logger`)
Comprehensive logging and monitoring:
- Asynchronous structured logging
- Multiple output targets (file, console, ETW)
- Performance metrics collection
- Log rotation and archiving

## Building

### Prerequisites
- Windows 10 19H1 or later
- Windows App SDK 1.4+
- Visual Studio 2022 with C++ desktop development
- CMake 3.20+
- vcpkg package manager

### Build Steps

1. **Clone and setup vcpkg**:
```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
bootstrap-vcpkg.bat
```

2. **Install dependencies**:
```bash
vcpkg install microsoft.windowsappsdk
```

3. **Build the agent**:
```bash
cd MacType-Universal
mkdir build && cd build
cmake .. -DBUILD_MODERN_AGENT=ON
cmake --build . --config Release
```

## Configuration

The agent can be configured via the `AgentConfig` structure:

```cpp
AgentConfig config;
config.serviceName = L"MacType.Agent";
config.displayName = L"MacType Modern Agent";
config.enableEncryption = true;
config.enableDiagnostics = true;
config.maxConnections = 10;
config.connectionTimeoutMs = 5000;
```

## Usage

### Starting the Agent
```cpp
// As a service
auto agentService = std::make_unique<AgentService>();
co_await agentService->RunServiceAsync();

// Or directly
auto controller = std::make_unique<AgentController>();
co_await controller->InitializeAsync();
co_await controller->StartAsync();
```

### Process Injection
```cpp
// Create injector
auto injector = std::make_unique<ModernInjector>();
co_await injector->InitializeAsync();

// Inject DLL
auto result = co_await injector->InjectLibraryAsync(processId, L"MacType.dll");
if (result.success) {
    std::cout << "Injection successful at: 0x" << std::hex << result.moduleBase << std::endl;
}
```

### Secure Communication
```cpp
// Create secure channel
auto channel = std::make_unique<SecureChannel>();
co_await channel->ConnectAsync();

// Send injection request
auto message = std::make_unique<InjectionRequestMessage>(processId, L"MacType.dll");
co_await channel->SendMessageAsync(std::move(message));
```

## Security Features

### Process Verification
- Digital signature validation
- Memory integrity checks
- Privilege escalation detection
- Anti-debugging measures

### Communication Security
- AES-256-GCM encryption
- Perfect forward secrecy
- Certificate-based authentication
- Replay attack prevention

### Injection Security
- DLL signature verification
- Memory corruption detection
- Injection rollback capabilities
- Access permission validation

## Performance Optimizations

### Memory Management
- Smart memory pooling for frequent operations
- Automatic cleanup of unused resources
- Memory usage monitoring and optimization

### Asynchronous Processing
- Non-blocking I/O operations
- Background thread pool for heavy operations
- Cancellation support for long-running tasks

### Caching
- Process information caching
- DLL signature cache
- Injection template caching

## Error Handling

The agent includes comprehensive error handling:

- **Automatic Recovery**: Failed operations are automatically retried
- **Graceful Degradation**: Core functionality preserved during partial failures
- **Detailed Diagnostics**: Extensive logging for troubleshooting
- **User-Friendly Messages**: Clear error messages for end users

## Testing

### Unit Tests
```cpp
// Core component tests
TEST(ProcessBridgeTest, AttachToProcess) {
    auto bridge = std::make_unique<ProcessBridge>();
    // Test process attachment logic
}

TEST(SecureChannelTest, EncryptedCommunication) {
    auto channel = std::make_unique<SecureChannel>();
    // Test encryption/decryption
}
```

### Integration Tests
- Full injection workflow testing
- Cross-architecture compatibility
- Performance benchmarking
- Security vulnerability testing

## Deployment

### Windows Service
The agent can be installed as a Windows service:

```xml
<!-- mt64agnt.service -->
[Unit]
Description=MacType Modern Agent
After=network.target

[Service]
Type=simple
User=mactype
ExecStart=/usr/local/bin/mt64agnt.exe
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
```

### MSIX Package
Modern deployment via MSIX:

```xml
<Package>
  <Identity Name="MacType.Agent" Version="2.0.0.0" />
  <Properties>
    <DisplayName>MacType Modern Agent</DisplayName>
    <Description>Secure cross-architecture agent for MacType</Description>
  </Properties>
  <Capabilities>
    <rescap:Capability Name="runFullTrust" />
  </Capabilities>
</Package>
```

## Migration from Legacy

### Compatibility Layer
The modern agent includes compatibility layers to work with existing MacType components:

- **Environment Variable Support**: Maintains `MACTYPE_X64ADDR` compatibility
- **Legacy API Wrappers**: Existing injection APIs are supported
- **Configuration Migration**: Automatic migration of settings

### Gradual Migration
1. **Parallel Deployment**: Run both agents simultaneously
2. **Feature Testing**: Test modern features with legacy fallback
3. **Incremental Migration**: Migrate components one by one
4. **Full Transition**: Complete legacy agent removal

## Contributing

We welcome contributions to the MacType Modern Agent project. Please see our [Contributing Guide](../CONTRIBUTING.md) for details.

## License

This project is licensed under the GPL-3.0 License - see the [LICENSE](../../LICENSE) file for details.

## Support

- **Issues**: [GitHub Issues](https://github.com/snowie2000/mactype/issues)
- **Discussions**: [GitHub Discussions](https://github.com/snowie2000/mactype/discussions)
- **Documentation**: [Wiki](https://github.com/snowie2000/mactype/wiki)

---

**MacType Modern Agent represents the future of secure, performant Windows font rendering technology.**
