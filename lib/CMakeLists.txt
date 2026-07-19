set(MBOOTCORE_SOURCES
    src/domain/Error.cpp

    src/application/Session.cpp
    src/device/DefaultDevice.cpp

    src/core/state/StateMachine.cpp

    src/core/protocols/sahara/SaharaPackets.cpp
    src/core/protocols/sahara/SaharaProtocol.cpp
    src/core/protocols/sahara/SaharaStateMachine.cpp
    src/core/protocols/sahara/SaharaPacketSerializer.cpp
    src/core/protocols/sahara/SaharaPacketParser.cpp

    src/core/protocols/firehose/FirehosePackets.cpp
    src/core/protocols/firehose/FirehoseProtocol.cpp
    src/core/protocols/firehose/FirehoseXmlEngine.cpp
    src/core/protocols/firehose/FirehoseStreamEngine.cpp
    src/core/protocols/firehose/ChunkEngine.cpp

    # Transport Layer
    src/transport/USBTransport.cpp
    src/transport/SerialTransport.cpp
    src/transport/TcpTransport.cpp
    src/transport/UdpTransport.cpp
    src/transport/SerialEnumerator.cpp
    src/transport/TransportManager.cpp
    src/transport/TransportFactory.cpp
    src/transport/VirtualTransports.cpp

    # Network abstraction
    src/network/NetworkAddress.cpp

    # Platform abstraction
    src/platform/DynamicLibrary.cpp
    src/platform/WinSockRuntime.cpp
    src/platform/Win32Handle.cpp
    src/platform/UniqueSocket.cpp
    src/platform/Win32SerialEnumeration.cpp

    # USB Backend (platform-conditional below)
    src/transport/usb/makeUsbBackend.cpp

    # Serial Backend (platform-conditional below)
    src/transport/serial/makeSerialBackend.cpp

    # TCP Backend (platform-conditional below)
    src/transport/network/makeTcpBackend.cpp

    # UDP Backend
    src/transport/network/makeUdpBackend.cpp

    src/loader/LoaderManager.cpp
    src/loader/ProgrammerLoader.cpp
    src/loader/LoaderRepository.cpp
    src/loader/LoaderMatcher.cpp
    src/loader/LoaderValidator.cpp
    src/loader/LoaderCache.cpp
    src/loader/ElfInspector.cpp
    src/loader/LoaderFramework.cpp

    src/logging/ConsoleLogger.cpp
    src/logging/FileLogger.cpp
    src/logging/NullLogger.cpp

    src/generic/OperationPipeline.cpp
    src/generic/adapter/SaharaAdapter.cpp
    src/generic/adapter/FirehoseAdapter.cpp

    src/elf/ElfParser.cpp
    src/elf/ElfValidator.cpp
    src/elf/MemoryImageBuilder.cpp
    src/elf/VirtualProgrammer.cpp

    src/core/pipeline/BootPipeline.cpp

    src/gpt/Guid.cpp
    src/gpt/GPTParser.cpp
    src/gpt/GPTWriter.cpp
    src/gpt/PartitionManager.cpp
    src/gpt/ImageReaders.cpp

    # Device Discovery & Protocol Negotiation
    src/discovery/ProtocolRegistry.cpp
    src/discovery/DeviceDiscoveryEngine.cpp
    src/discovery/ProtocolNegotiationEngine.cpp
    src/discovery/VirtualDeviceDetector.cpp

    # Device Session & Multi-Device Manager
    src/session/DeviceSession.cpp
    src/session/DeviceManager.cpp
    src/session/DeviceSessionFactory.cpp
    src/session/SessionLogger.cpp

    # Plugin SDK & Vendor Extension Framework
    src/plugin/PluginManager.cpp

    # Universal Boot Operations & Job Engine
    src/job/RecoveryPolicies.cpp
    src/job/JobHistory.cpp
    src/job/ProgressAggregator.cpp
    src/job/JobPipeline.cpp
    src/job/JobScheduler.cpp
    src/job/VirtualJobDevice.cpp
    src/job/GenericJobs.cpp

    # Universal Firmware Package Engine
    src/firmware/FirmwarePackage.cpp
    src/firmware/FirmwareReaders.cpp
    src/firmware/FirmwareValidator.cpp
    src/firmware/FirmwareResolver.cpp
    src/firmware/ImageEngine.cpp
    src/firmware/FlashPlan.cpp
    src/firmware/FirmwareExecutor.cpp
    src/firmware/VirtualFirmware.cpp

    # Diagnostics Framework
    src/diagnostics/DiagnosticsManager.cpp
    src/diagnostics/DiagnosticCheck.cpp
    src/diagnostics/DiagnosticSession.cpp

    # Performance Profiler
    src/profiler/Profiler.cpp

    # Memory Tracker
    src/memory/MemoryTracker.cpp

    # Stress Test Framework
    src/stress/VirtualStressEnvironment.cpp

    # Fault Injection Engine
    src/faults/FaultInjector.cpp
    src/faults/FaultManager.cpp

    # Security Framework
    src/security/SecurityTypes.cpp
    src/security/SecurityManager.cpp
    src/security/PermissionSet.cpp
    src/security/InMemoryStorage.cpp

    # Benchmark Suite
    src/benchmark/BenchmarkRunner.cpp

    # Telemetry Framework
    src/telemetry/TelemetryCollector.cpp

    # Structured Logger
    src/logging/StructuredLogger.cpp

    # Configuration Framework
    src/config/ConfigManager.cpp

    # DSP Module
    src/dsp/DSPManager.cpp
    src/dsp/DSPRepository.cpp
    src/dsp/DSPCache.cpp
    src/dsp/DSPValidator.cpp
    src/dsp/DSPDependencyGraph.cpp
    src/dsp/DSPBuilder.cpp
    src/dsp/DSPInspector.cpp
    src/dsp/HardwareProfile.cpp
    src/dsp/LoaderRepository.cpp
    src/dsp/VendorQuirk.cpp
    # Universal Boot Runtime & Workflow Engine
    src/workflow/WorkflowBuilder.cpp
    src/workflow/WorkflowEngine.cpp
    src/workflow/WorkflowExecutor.cpp
    src/workflow/WorkflowRecovery.cpp
    src/workflow/WorkflowProgressEngine.cpp
    src/workflow/WorkflowHistory.cpp
    src/workflow/WorkflowFactory.cpp
    src/workflow/VirtualWorkflowRuntime.cpp

    # Universal Vendor Integration Framework
    src/vendor/VendorRegistry.cpp
    src/vendor/VendorFactory.cpp
    src/vendor/VendorRuntime.cpp
    src/vendor/CapabilityResolver.cpp
    src/vendor/VendorPipeline.cpp
    src/vendor/VendorSession.cpp
    src/vendor/VendorMonitor.cpp
    src/vendor/VendorPackageResolver.cpp

# Universal Runtime Orchestrator & Public Production API
src/runtime/DeviceService.cpp
src/runtime/FirmwareService.cpp
src/runtime/WorkflowService.cpp
src/runtime/PluginService.cpp
src/runtime/Runtime.cpp
src/runtime/RuntimeBuilder.cpp
src/runtime/RuntimeFactory.cpp
src/runtime/RuntimeHardware.cpp
src/runtime/DiagnosticsService.cpp

)

# SDK sources — compiled directly into mbootcore (see sdk/CMakeLists.txt)
list(APPEND MBOOTCORE_SOURCES
    ${CMAKE_SOURCE_DIR}/sdk/src/Version.cpp
    ${CMAKE_SOURCE_DIR}/sdk/src/VendorSDK.cpp
    ${CMAKE_SOURCE_DIR}/sdk/src/VendorSDKFactory.cpp
    ${CMAKE_SOURCE_DIR}/sdk/src/PluginManifest.cpp
    ${CMAKE_SOURCE_DIR}/sdk/src/PluginValidator.cpp
    ${CMAKE_SOURCE_DIR}/sdk/src/PluginCompatibility.cpp
    ${CMAKE_SOURCE_DIR}/sdk/src/PluginDependencyGraph.cpp
    ${CMAKE_SOURCE_DIR}/sdk/src/SDKInfo.cpp
    ${CMAKE_SOURCE_DIR}/sdk/src/SDKDoctor.cpp
    ${CMAKE_SOURCE_DIR}/sdk/src/EnvironmentChecker.cpp
    ${CMAKE_SOURCE_DIR}/sdk/src/SDKValidator.cpp
    ${CMAKE_SOURCE_DIR}/sdk/src/APICompatibility.cpp
)

# ---------------------------------------------------------------------------
# Crypto backend — enabled by default via DependencyManager.cmake.
# Mbed TLS is auto-downloaded, no host dependencies required.
# ---------------------------------------------------------------------------
option(MBOOTCORE_ENABLE_CRYPTO
    "Enable cryptographic backend (Mbed TLS, auto-managed)"
    ON
)

set(CRYPTO_PROVIDER "mbedtls" CACHE STRING "Cryptographic backend provider")

if(MBOOTCORE_ENABLE_CRYPTO AND NOT CRYPTO_PROVIDER)
    message(FATAL_ERROR
        "MBOOTCORE_ENABLE_CRYPTO=ON was requested but no cryptographic backend "
        "provider has been configured.  Set -DCRYPTO_PROVIDER=mbedtls."
    )
endif()

# ---------------------------------------------------------------------------
# Vendor Scaffold Protocols — OFF by default (not production ready)
# Enable with -DMBOOTCORE_ENABLE_VENDOR_SCAFFOLDS=ON for development only
# ---------------------------------------------------------------------------
option(MBOOTCORE_ENABLE_VENDOR_SCAFFOLDS
    "Build vendor scaffold reference protocols (MediaTek, UNISOC) — not production ready"
    OFF
)

if(MBOOTCORE_ENABLE_VENDOR_SCAFFOLDS)
    list(APPEND MBOOTCORE_SOURCES
        src/protocols/unisoc/UNISOCProtocol.cpp
        src/protocols/mediatek/MTKProtocol.cpp
    )
    message(STATUS "Vendor scaffold protocols ENABLED — not production ready")
else()
    message(STATUS "Vendor scaffold protocols DISABLED (default)")
endif()

# ---------------------------------------------------------------------------
# Mbed TLS crypto provider — added when MBOOTCORE_ENABLE_CRYPTO=ON
# ---------------------------------------------------------------------------
if(MBOOTCORE_ENABLE_CRYPTO)
    list(APPEND MBOOTCORE_SOURCES
        src/security/mbedtls/MbedTlsUtils.cpp
        src/security/mbedtls/MbedTLSHashProvider.cpp
        src/security/mbedtls/MbedTLSSignatureVerifier.cpp
        src/security/mbedtls/MbedTLSIntegrityVerifier.cpp
        src/security/mbedtls/MbedTLSSecureStorage.cpp
    )
endif()

# ===========================================================================
# OBJECT library — internal build target (compilation only)
# All compile definitions, include directories, options, warnings, and
# platform-specific sources live here.  Do not reference this target
# outside of lib/CMakeLists.txt.
# ===========================================================================
add_library(mbootcore_objects OBJECT ${MBOOTCORE_SOURCES})

target_include_directories(mbootcore_objects PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/sdk/include
)

target_compile_definitions(mbootcore_objects PRIVATE MBOOTCORE_VERSION="${PROJECT_VERSION}")

if(MBOOTCORE_ENABLE_VENDOR_SCAFFOLDS)
    target_compile_definitions(mbootcore_objects PRIVATE MBOOTCORE_ENABLE_VENDOR_SCAFFOLDS=1)
endif()
if(MBOOTCORE_ENABLE_CRYPTO)
    target_compile_definitions(mbootcore_objects PRIVATE MBOOTCORE_ENABLE_CRYPTO=1)
endif()

if(MBOOTCORE_ENABLE_CRYPTO)
    target_compile_definitions(mbootcore_objects PRIVATE MBOOTCORE_HAVE_MBEDTLS=1)
    target_include_directories(mbootcore_objects PRIVATE
        ${CMAKE_SOURCE_DIR}/deps/mbedtls/source/include
        ${CMAKE_SOURCE_DIR}/deps/mbedtls/source/tf-psa-crypto/include
    )
endif()

# Platform-specific USB backends
if(WIN32)
    target_sources(mbootcore_objects PRIVATE
        src/transport/usb/WinUsbBackend.cpp
        src/transport/usb/UsbDeviceEnumerator.cpp
    )
elseif(MBOOTCORE_HAVE_USB)
    target_sources(mbootcore_objects PRIVATE
        src/transport/usb/LibUsbBackend.cpp
    )
    target_compile_definitions(mbootcore_objects PRIVATE MBOOTCORE_HAVE_LIBUSB=1)
endif()

# Platform-specific serial backends
if(WIN32)
    target_sources(mbootcore_objects PRIVATE
        src/transport/serial/Win32SerialBackend.cpp
    )
    target_compile_definitions(mbootcore_objects PRIVATE MBOOTCORE_HAVE_WIN32_SERIAL=1)
else()
    target_sources(mbootcore_objects PRIVATE
        src/transport/serial/PosixSerialBackend.cpp
    )
    target_compile_definitions(mbootcore_objects PRIVATE MBOOTCORE_HAVE_POSIX_SERIAL=1)
endif()

# Platform-specific TCP backends
if(WIN32)
    target_sources(mbootcore_objects PRIVATE
        src/transport/network/WinSockTcpBackend.cpp
    )
    target_compile_definitions(mbootcore_objects PRIVATE MBOOTCORE_HAVE_WIN32_TCP=1)
else()
    target_sources(mbootcore_objects PRIVATE
        src/transport/network/PosixTcpBackend.cpp
    )
    target_compile_definitions(mbootcore_objects PRIVATE MBOOTCORE_HAVE_POSIX_TCP=1)
endif()

# Platform-specific UDP backends
if(WIN32)
    target_sources(mbootcore_objects PRIVATE
        src/transport/network/WinSockUdpBackend.cpp
    )
    target_compile_definitions(mbootcore_objects PRIVATE MBOOTCORE_HAVE_WIN32_UDP=1)
else()
    target_sources(mbootcore_objects PRIVATE
        src/transport/network/PosixUdpBackend.cpp
    )
    target_compile_definitions(mbootcore_objects PRIVATE MBOOTCORE_HAVE_POSIX_UDP=1)
endif()

mbootcore_apply_warnings(mbootcore_objects)
mbootcore_apply_sanitizers(mbootcore_objects)

# PRIVATE build dependencies — on mbootcore_objects so they establish the
# build graph and provide usage requirements for compilation, without
# leaking into install(EXPORT).
target_link_libraries(mbootcore_objects PRIVATE ZLIB::ZLIB nlohmann_json::nlohmann_json)
if(MBOOTCORE_ENABLE_CRYPTO)
    if(TARGET MbedTLS::MbedCrypto)
        target_link_libraries(mbootcore_objects PRIVATE MbedTLS::MbedCrypto)
    elseif(TARGET tfpsacrypto)
        target_link_libraries(mbootcore_objects PRIVATE tfpsacrypto)
    endif()
endif()
if(MBOOTCORE_HAVE_USB AND NOT WIN32)
    target_link_libraries(mbootcore_objects PRIVATE LibUSB::LibUSB)
endif()
if(WIN32)
    target_link_libraries(mbootcore_objects PRIVATE setupapi ws2_32)
endif()

# ===========================================================================
# STATIC library — export-clean wrapper
# Assembles objects from mbootcore_objects and provides the PUBLIC interface
# for consumers.  No third-party link dependencies — third-party objects
# are absorbed into the monolithic archive.  Windows system libraries
# (setupapi, ws2_32) are declared as PUBLIC link deps below.
# ===========================================================================
add_library(mbootcore STATIC)
target_sources(mbootcore PRIVATE $<TARGET_OBJECTS:mbootcore_objects>)
set_target_properties(mbootcore PROPERTIES LINKER_LANGUAGE CXX)

# PUBLIC interface — visible to consumers
target_compile_definitions(mbootcore PUBLIC MBOOTCORE_VERSION="${PROJECT_VERSION}")
if(MBOOTCORE_ENABLE_VENDOR_SCAFFOLDS)
    target_compile_definitions(mbootcore PUBLIC MBOOTCORE_ENABLE_VENDOR_SCAFFOLDS=1)
endif()
if(MBOOTCORE_ENABLE_CRYPTO)
    target_compile_definitions(mbootcore PUBLIC MBOOTCORE_ENABLE_CRYPTO=1)
endif()

target_compile_features(mbootcore PUBLIC cxx_std_17)

target_include_directories(mbootcore
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/sdk/include>
)

# Collect public headers for install
file(GLOB_RECURSE MBOOTCORE_PUBLIC_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/mbootcore/*.hpp"
)
set_target_properties(mbootcore PROPERTIES
    PUBLIC_HEADER "${MBOOTCORE_PUBLIC_HEADERS}"
)

# ---------------------------------------------------------------------------
# Monolithic archive: merge dependency object files into mbootcore.
# Deps are auto-derived from available targets — no manual list needed.
# ---------------------------------------------------------------------------
set(_monolithic_deps "")
if(TARGET zlibstatic)
    list(APPEND _monolithic_deps zlibstatic)
endif()
if(TARGET tfpsacrypto)
    list(APPEND _monolithic_deps tfpsacrypto)
elseif(TARGET MbedTLS::MbedCrypto)
    list(APPEND _monolithic_deps MbedTLS::MbedCrypto)
endif()
if(TARGET usb-1.0)
    list(APPEND _monolithic_deps usb-1.0)
endif()
if(_monolithic_deps)
    mbootcore_create_monolithic_archive(mbootcore ${_monolithic_deps})
endif()

# Sanitizer link flags on the STATIC wrapper (link flags on OBJECT libs are no-ops).
mbootcore_apply_sanitizers(mbootcore)

# Windows system libraries required by objects baked into libmbootcore.a.
# Static archives do not record dependent system libraries; therefore they
# must be propagated through the target link interface.
if(WIN32)
    target_link_libraries(mbootcore PUBLIC
        setupapi
        ws2_32
        bcrypt
    )
endif()

# macOS: libusb's darwin_usb.c requires CoreFoundation, IOKit, and Security.
# These frameworks are linked PRIVATE to usb-1.0, but the monolithic merge
# absorbs those objects into libmbootcore.a without propagating link flags.
# We redeclare them as PUBLIC so every consumer of mbootcore gets them.
if(APPLE)
    target_link_libraries(mbootcore PUBLIC
        "-framework CoreFoundation"
        "-framework IOKit"
        "-framework Security"
    )
endif()
