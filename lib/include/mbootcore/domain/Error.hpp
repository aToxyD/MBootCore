#pragma once

#include <cstdint>
#include <cstddef>
#include <new>
#include <string>
#include <optional>
#include <type_traits>
#include <utility>

namespace mbootcore {

// ==========================================================================
// ErrorCode — all error codes in the system
// ==========================================================================

enum class ErrorCode : uint32_t {
    Success                  = 0x0000,
    Unknown                  = 0x0001,
    NotSupported             = 0x0002,
    InvalidArgument          = 0x0003,

    TransportError           = 0x0100,
    TransportTimeout         = 0x0101,
    TransportDisconnected    = 0x0102,
    TransportWriteFailed     = 0x0103,
    TransportReadFailed      = 0x0104,

    ProtocolError            = 0x0200,
    ProtocolMismatch         = 0x0201,
    InvalidPacket            = 0x0202,
    UnexpectedPacket         = 0x0203,
    InvalidState             = 0x0204,
    UnsupportedVersion       = 0x0205,

    SaharaNakSuccess             = 0x0300,
    SaharaNakInvalidCmd          = 0x0301,
    SaharaNakProtocolMismatch    = 0x0302,
    SaharaNakInvalidTargetProto  = 0x0303,
    SaharaNakInvalidHostProto    = 0x0304,
    SaharaNakInvalidPktSize      = 0x0305,
    SaharaNakUnexpectedPkt       = 0x0306,
    SaharaNakInvalidTransferMode = 0x0307,
    SaharaNakInvalidHostId       = 0x0308,
    SaharaNakTimeoutRx           = 0x0309,
    SaharaNakTimeoutTx           = 0x030A,
    SaharaNakInvalidMode         = 0x030B,
    SaharaNakInvalidHostReq      = 0x030C,
    SaharaNakReadDataError       = 0x030D,
    SaharaNakWriteDataError      = 0x030E,
    SaharaNakInvalidMemTable     = 0x030F,
    SaharaNakInvalidMemInfo      = 0x0310,
    SaharaNakMemDebugNotSup      = 0x0311,
    SaharaNakMemReadFailed       = 0x0312,
    SaharaNakInvalidImgId        = 0x0313,
    SaharaNakImgNotFound         = 0x0314,
    SaharaNakImgAuthFailed       = 0x0315,
    SaharaNakImgTooLarge         = 0x0316,
    SaharaNakAuthFailed          = 0x0317,
    SaharaNakInvalidImgHdr       = 0x0318,
    SaharaNakImgHdrInvalidVer    = 0x0319,
    SaharaNakInvalidSignature    = 0x031A,
    SaharaNakInvalidHash         = 0x031B,
    SaharaNakFatal               = 0x031C,
    SaharaNakCmdExecFailure      = 0x031D,
    SaharaNakImgEncFailed        = 0x031E,
    SaharaNakImgHdrAuthFailed    = 0x031F,
    SaharaNakImgHdrVerFailed     = 0x0320,
    SaharaNakImgHdrMacFailed     = 0x0321,
    SaharaNakImgHdrMacNotFound   = 0x0322,
    SaharaNakImgHdrSignFailed    = 0x0323,
    SaharaNakImgHdrSignNotFound  = 0x0324,
    SaharaNakInvalidImgContent   = 0x0325,
    SaharaNakInvalidProgrammer   = 0x0326,
    SaharaNakProgrammerAuthFail  = 0x0327,
    SaharaNakProgrammerMismatch  = 0x0328,

    LoaderNotFound           = 0x0400,
    LoaderRejected           = 0x0401,
    LoaderInvalidFormat      = 0x0402,
    LoaderVersionMismatch    = 0x0403,
    InvalidElf               = 0x0404,
    CacheMiss                = 0x0405,

    Cancelled                = 0x0004,
    AlreadyExists            = 0x0005,

    DeviceNotFound           = 0x0500,
    ElfSegmentOverlap        = 0x0600,
    ElfTruncated             = 0x0601,
    ElfUnsupportedMachine    = 0x0602,
    DeviceDisconnected       = 0x0501,
    DeviceAccessDenied       = 0x0502,

    FirehoseNak               = 0x0700,
    FirehoseSectorOutOfRange  = 0x0701,
    FirehoseProgramFailed     = 0x0702,
    FirehoseEraseFailed       = 0x0703,
    FirehoseUnsupportedMemory = 0x0704,
    FirehoseInvalidParam      = 0x0705,
    FirehoseSeqError          = 0x0706,
    FirehoseTimeout           = 0x0707,
    FirehoseCrcMismatch       = 0x0708,
    FirehosePartialWrite      = 0x0709,

    GPTCorrupted              = 0x0800,
    GPTInvalidHeader          = 0x0801,
    GPTInvalidEntry           = 0x0802,
    GPTPrimaryMissing         = 0x0803,
    GPTBackupMissing          = 0x0804,
    GPTCrcMismatch            = 0x0805,
    PartitionNotFound         = 0x0806,
    PartitionOverlap          = 0x0807,
    InvalidImage              = 0x0808,
    ImageTooLarge             = 0x0809,

    DeviceNotIdentified       = 0x0900,
    NegotiationFailed         = 0x0901,
    NoMatchingProtocol        = 0x0902,
    EnumerationFailed         = 0x0903,
    ProbeFailed               = 0x0904,
    DetectorConflict          = 0x0905,
    RegistryEmpty             = 0x0906,

    SessionNotConnected       = 0x0A00,
    SessionAlreadyConnected   = 0x0A01,
    SessionBusy               = 0x0A02,
    SessionTimeout            = 0x0A03,
    SessionTerminal           = 0x0A04,
    SessionLimitExceeded      = 0x0A05,

    PluginLoadFailed          = 0x0B00,
    PluginInitFailed          = 0x0B01,
    PluginShutdownFailed      = 0x0B02,
    PluginNotFound            = 0x0B03,
    PluginIncompatible        = 0x0B04,
    PluginDependencyMissing   = 0x0B05,
    PluginCircularDependency  = 0x0B06,
    PluginDuplicate           = 0x0B07,
    PluginAlreadyLoaded       = 0x0B08,
    PluginNotLoaded           = 0x0B09,
    PluginRegistrationFailed  = 0x0B0A,
    PluginUnregistrationFailed = 0x0B0B,
    PluginVersionMismatch     = 0x0B0C,
    PluginConfigInvalid       = 0x0B0D,

    JobFailed                 = 0x0C00,
    JobCancelled              = 0x0C01,
    JobTimeout                = 0x0C02,
    JobInvalidState           = 0x0C03,
    JobDependencyFailed       = 0x0C04,
    JobNotFound               = 0x0C05,
    JobAlreadyRunning         = 0x0C06,
    JobQueueFull              = 0x0C07,
    JobRecoveryFailed         = 0x0C08,
    JobRollbackFailed         = 0x0C09,
    JobInvalidConfig          = 0x0C0A,
    JobDeviceNotReady         = 0x0C0B,
    JobDataMismatch           = 0x0C0C,
    JobInsufficientSpace      = 0x0C0D,

    FirmwareInvalidFormat     = 0x0D00,
    FirmwareImageNotFound     = 0x0D01,
    FirmwareHashMismatch      = 0x0D02,
    FirmwareInvalidManifest   = 0x0D03,
    FirmwareDependencyConflict = 0x0D04,
    FirmwareUnsupportedDevice = 0x0D05,
    FirmwareUnsupportedVendor = 0x0D06,
    FirmwareUnsupportedStorage = 0x0D07,
    FirmwareDuplicatePartition = 0x0D08,
    FirmwareMissingProgrammer = 0x0D09,
    FirmwareValidationFailed  = 0x0D0A,
    FirmwareExtractionFailed  = 0x0D0B,
    FirmwarePackageCorrupted  = 0x0D0C,
    FirmwareVersionMismatch   = 0x0D0D,
    FirmwarePackageNotFound   = 0x0D0E,
    FirmwareNotEnoughImages   = 0x0D0F,

    WorkflowInvalidState       = 0x0E00,
    WorkflowStepFailed         = 0x0E01,
    WorkflowRollbackFailed     = 0x0E02,
    WorkflowCancelled          = 0x0E03,
    WorkflowTimeout            = 0x0E04,
    WorkflowStepNotFound       = 0x0E05,
    WorkflowExecutionFailed    = 0x0E06,
    WorkflowRecoveryFailed     = 0x0E07,

    TransportNotOpen            = 0x0F00,
    TransportAlreadyOpen        = 0x0F01,
    TransportBusy               = 0x0F02,
    TransportInvalidEndpoint    = 0x0F03,
    TransportBufferFull         = 0x0F04,
    TransportPartialTransfer    = 0x0F05,
    TransportReconnectFailed    = 0x0F06,
    TransportHotplugRemoved     = 0x0F07,
    TransportHotplugArrived     = 0x0F08,
    TransportEnumerationFailed  = 0x0F09,
    TransportBackendUnavailable = 0x0F0A,
    TransportAsyncCancelled     = 0x0F0B,
    TransportRetryExhausted     = 0x0F0C,
    TransportBufferTooSmall     = 0x0F0D,

    CryptoInitializationFailed  = 0x1000,
    CryptoHashFailed            = 0x1001,
    CryptoSignatureVerifyFailed = 0x1002,
    CryptoKeyLoadFailed         = 0x1003,
    CryptoInvalidPadding        = 0x1004,
    CryptoVerificationFailed    = 0x1005,
};

const char* toString(ErrorCode code) noexcept;

// ==========================================================================
// ErrorInfo — rich error information
// ==========================================================================

struct ErrorInfo {
    ErrorCode code{ErrorCode::Success};
    std::string context;
    std::string source;
    std::optional<int> nativeError;
};

// ==========================================================================
// Result<T> — value-or-error with manual storage lifetime
// ==========================================================================

template<typename T>
class [[nodiscard]] Result {
    static constexpr std::size_t StorageSize =
        sizeof(T) > sizeof(ErrorInfo) ? sizeof(T) : sizeof(ErrorInfo);
    static constexpr std::size_t StorageAlign =
        alignof(T) > alignof(ErrorInfo) ? alignof(T) : alignof(ErrorInfo);

    alignas(StorageAlign) unsigned char m_storage[StorageSize];
    bool m_ok;

    void constructValue(T&& val) {
        ::new (m_storage) T(std::move(val));
    }

    void constructError(ErrorInfo info) {
        ::new (m_storage) ErrorInfo(std::move(info));
    }

    T* valuePtr() noexcept {
        return reinterpret_cast<T*>(m_storage);
    }

    const T* valuePtr() const noexcept {
        return reinterpret_cast<const T*>(m_storage);
    }

    ErrorInfo* errorPtr() noexcept {
        return reinterpret_cast<ErrorInfo*>(m_storage);
    }

    const ErrorInfo* errorPtr() const noexcept {
        return reinterpret_cast<const ErrorInfo*>(m_storage);
    }

    void destroy() noexcept(std::is_nothrow_destructible_v<T>) {
        if (m_ok) {
            valuePtr()->~T();
        } else {
            errorPtr()->~ErrorInfo();
        }
    }

public:
    // ---- Construction ---------------------------------------------------

    Result(T val) noexcept(std::is_nothrow_move_constructible_v<T>)
        : m_ok(true) {
        constructValue(std::move(val));
    }

    Result(ErrorCode code) noexcept
        : m_ok(false) {
        constructError(ErrorInfo{code, {}, {}, {}});
    }

    Result(ErrorInfo info) noexcept
        : m_ok(false) {
        constructError(std::move(info));
    }

    // ---- Static factories (backward compatibility) ----------------------

    static Result Ok(T value) noexcept(std::is_nothrow_move_constructible_v<T>) {
        return Result(std::move(value));
    }

    static Result Error(ErrorCode code) noexcept {
        return Result(code);
    }

    static Result Error(ErrorInfo info) noexcept {
        return Result(std::move(info));
    }

    // ---- Destructor ----------------------------------------------------

    ~Result() noexcept(std::is_nothrow_destructible_v<T>) {
        destroy();
    }

    // ---- Move semantics ------------------------------------------------

    Result(Result&& other) noexcept(
        std::is_nothrow_move_constructible_v<T> &&
        std::is_nothrow_move_constructible_v<ErrorInfo>)
        : m_ok(other.m_ok) {
        if (m_ok) {
            constructValue(std::move(*other.valuePtr()));
        } else {
            constructError(std::move(*other.errorPtr()));
        }
    }

    Result& operator=(Result&& other) noexcept(
        std::is_nothrow_move_constructible_v<T> &&
        std::is_nothrow_move_constructible_v<ErrorInfo>) {
        if (this != &other) {
            destroy();
            m_ok = other.m_ok;
            if (m_ok) {
                constructValue(std::move(*other.valuePtr()));
            } else {
                constructError(std::move(*other.errorPtr()));
            }
        }
        return *this;
    }

    Result(const Result&) = delete;
    Result& operator=(const Result&) = delete;

    // ---- State queries -------------------------------------------------

    bool isOk() const noexcept { return m_ok; }
    bool isError() const noexcept { return !m_ok; }

    explicit operator bool() const noexcept { return m_ok; }
    bool hasValue() const noexcept { return m_ok; }
    bool hasError() const noexcept { return !m_ok; }

    // ---- Value access --------------------------------------------------

    T& value() noexcept { return *valuePtr(); }
    const T& value() const noexcept { return *valuePtr(); }

    T&& takeValue() noexcept { return std::move(*valuePtr()); }

    // ---- Error access --------------------------------------------------

    ErrorCode error() const noexcept { return errorPtr()->code; }

    const ErrorInfo& errorInfo() const noexcept { return *errorPtr(); }

    // ---- Monadic operations --------------------------------------------

    template<typename F>
    auto map(F&& f) & -> Result<decltype(f(std::declval<T&>()))> {
        using MappedType = decltype(f(std::declval<T&>()));
        if (m_ok) {
            return Result<MappedType>(f(value()));
        }
        return Result<MappedType>(errorInfo());
    }

    template<typename F>
    auto map(F&& f) const& -> Result<decltype(f(std::declval<const T&>()))> {
        using MappedType = decltype(f(std::declval<const T&>()));
        if (m_ok) {
            return Result<MappedType>(f(value()));
        }
        return Result<MappedType>(errorInfo());
    }

    template<typename F>
    auto map(F&& f) && -> Result<decltype(f(std::declval<T>()))> {
        using MappedType = decltype(f(std::declval<T>()));
        if (m_ok) {
            return Result<MappedType>(f(std::move(*valuePtr())));
        }
        return Result<MappedType>(errorInfo());
    }

    template<typename F>
    auto and_then(F&& f) & -> decltype(f(std::declval<T&>())) {
        using ResultType = decltype(f(std::declval<T&>()));
        if (m_ok) {
            return f(value());
        }
        return ResultType(errorInfo());
    }

    template<typename F>
    auto and_then(F&& f) const& -> decltype(f(std::declval<const T&>())) {
        using ResultType = decltype(f(std::declval<const T&>()));
        if (m_ok) {
            return f(value());
        }
        return ResultType(errorInfo());
    }

    template<typename F>
    auto and_then(F&& f) && -> decltype(f(std::declval<T>())) {
        using ResultType = decltype(f(std::declval<T>()));
        if (m_ok) {
            return f(std::move(*valuePtr()));
        }
        return ResultType(errorInfo());
    }

    template<typename F>
    Result<T> or_else(F&& f) & {
        if (isError()) {
            f(errorInfo());
        }
        return std::move(*this);
    }

    template<typename F>
    Result<T> or_else(F&& f) && {
        if (isError()) {
            f(errorInfo());
        }
        return std::move(*this);
    }

    template<typename F>
    auto transform_error(F&& f) & -> Result<decltype(f(std::declval<ErrorInfo>()))> {
        using NewErrorType = decltype(f(std::declval<ErrorInfo>()));
        if (isError()) {
            return Result<NewErrorType>(f(errorInfo()));
        }
        return std::move(*this);
    }

    template<typename F>
    auto transform_error(F&& f) && -> Result<decltype(f(std::declval<ErrorInfo>()))> {
        using NewErrorType = decltype(f(std::declval<ErrorInfo>()));
        if (isError()) {
            return Result<NewErrorType>(f(std::move(*errorPtr())));
        }
        return std::move(*this);
    }

    T value_or(T defaultVal) const& {
        return m_ok ? value() : std::move(defaultVal);
    }

    T value_or(T defaultVal) && {
        return m_ok ? std::move(*valuePtr()) : std::move(defaultVal);
    }

    template<typename FOk, typename FErr>
    auto match(FOk&& okFn, FErr&& errFn) & -> decltype(okFn(std::declval<T&>())) {
        if (m_ok) {
            return okFn(value());
        }
        return errFn(errorInfo());
    }

    template<typename FOk, typename FErr>
    auto match(FOk&& okFn, FErr&& errFn) const& -> decltype(okFn(std::declval<const T&>())) {
        if (m_ok) {
            return okFn(value());
        }
        return errFn(errorInfo());
    }

    template<typename FOk, typename FErr>
    auto match(FOk&& okFn, FErr&& errFn) && -> decltype(okFn(std::declval<T>())) {
        if (m_ok) {
            return okFn(std::move(*valuePtr()));
        }
        return errFn(std::move(*errorPtr()));
    }
};

// ==========================================================================
// Result<void> — void specialization
// ==========================================================================

template<>
class [[nodiscard]] Result<void> {
    ErrorInfo m_error;
    bool m_ok;

public:
    Result() noexcept : m_ok(true) {}
    Result(ErrorCode code) noexcept : m_error{code, {}, {}, {}}, m_ok(false) {}
    Result(ErrorInfo info) noexcept : m_error(std::move(info)), m_ok(false) {}

    static Result Ok() noexcept { return Result(); }
    static Result Error(ErrorCode code) noexcept { return Result(code); }
    static Result Error(ErrorInfo info) noexcept { return Result(std::move(info)); }

    ~Result() noexcept = default;
    Result(const Result&) = delete;
    Result& operator=(const Result&) = delete;

    Result(Result&&) noexcept = default;
    Result& operator=(Result&&) noexcept = default;

    bool isOk() const noexcept { return m_ok; }
    bool isError() const noexcept { return !m_ok; }

    explicit operator bool() const noexcept { return m_ok; }
    bool hasValue() const noexcept { return m_ok; }
    bool hasError() const noexcept { return !m_ok; }

    ErrorCode error() const noexcept { return m_error.code; }
    const ErrorInfo& errorInfo() const noexcept { return m_error; }

    template<typename F>
    auto map(F&& f) & -> Result<decltype(f())> {
        using MappedType = decltype(f());
        if (m_ok) {
            return Result<MappedType>(f());
        }
        return Result<MappedType>(errorInfo());
    }

    template<typename F>
    auto map(F&& f) const& -> Result<decltype(f())> {
        using MappedType = decltype(f());
        if (m_ok) {
            return Result<MappedType>(f());
        }
        return Result<MappedType>(errorInfo());
    }

    template<typename F>
    auto map(F&& f) && -> Result<decltype(f())> {
        using MappedType = decltype(f());
        if (m_ok) {
            return Result<MappedType>(f());
        }
        return Result<MappedType>(errorInfo());
    }

    template<typename F>
    auto and_then(F&& f) & -> decltype(f()) {
        using ResultType = decltype(f());
        if (m_ok) {
            return f();
        }
        return ResultType(errorInfo());
    }

    template<typename F>
    auto and_then(F&& f) const& -> decltype(f()) {
        using ResultType = decltype(f());
        if (m_ok) {
            return f();
        }
        return ResultType(errorInfo());
    }

    template<typename F>
    auto and_then(F&& f) && -> decltype(f()) {
        using ResultType = decltype(f());
        if (m_ok) {
            return f();
        }
        return ResultType(errorInfo());
    }

    template<typename F>
    Result<void> or_else(F&& f) & {
        if (isError()) {
            f(errorInfo());
        }
        return std::move(*this);
    }

    template<typename F>
    Result<void> or_else(F&& f) && {
        if (isError()) {
            f(errorInfo());
        }
        return std::move(*this);
    }

    template<typename FErr>
    auto transform_error(FErr&& f) & -> Result<decltype(f(std::declval<ErrorInfo>()))> {
        using NewErrorType = decltype(f(std::declval<ErrorInfo>()));
        if (isError()) {
            return Result<NewErrorType>(f(errorInfo()));
        }
        return std::move(*this);
    }

    template<typename FErr>
    auto transform_error(FErr&& f) && -> Result<decltype(f(std::declval<ErrorInfo>()))> {
        using NewErrorType = decltype(f(std::declval<ErrorInfo>()));
        if (isError()) {
            return Result<NewErrorType>(f(std::move(m_error)));
        }
        return std::move(*this);
    }

    template<typename FOk, typename FErr>
    auto match(FOk&& okFn, FErr&& errFn) & -> decltype(okFn()) {
        if (m_ok) {
            return okFn();
        }
        return errFn(errorInfo());
    }

    template<typename FOk, typename FErr>
    auto match(FOk&& okFn, FErr&& errFn) const& -> decltype(okFn()) {
        if (m_ok) {
            return okFn();
        }
        return errFn(errorInfo());
    }

    template<typename FOk, typename FErr>
    auto match(FOk&& okFn, FErr&& errFn) && -> decltype(okFn()) {
        if (m_ok) {
            return okFn();
        }
        return errFn(std::move(m_error));
    }
};

// ==========================================================================
// MBOOT_TRY — propagation macros
// ==========================================================================

#define MBOOT_TRY(expr)                                                        \
    do {                                                                       \
        auto _mboot_result = (expr);                                           \
        if (_mboot_result.isError()) {                                         \
            return _mboot_result.error();                                      \
        }                                                                      \
    } while(false)

#define MBOOT_TRY_ASSIGN(var, expr)                                            \
    auto _mboot_tmp_##var = (expr);                                            \
    if (_mboot_tmp_##var.isError()) {                                          \
        return _mboot_tmp_##var.error();                                       \
    }                                                                          \
    auto var = std::move(_mboot_tmp_##var).takeValue();

} // namespace mbootcore
