# Error Codes

All error codes in the `mbootcore::ErrorCode` enum. Extracted directly from
`lib/include/mbootcore/domain/Error.hpp`.

## General

| Code | Hex | Name |
|------|-----|------|
| 0 | `0x0000` | `Success` |
| 1 | `0x0001` | `Unknown` |
| 2 | `0x0002` | `NotSupported` |
| 3 | `0x0003` | `InvalidArgument` |
| 4 | `0x0004` | `Cancelled` |
| 5 | `0x0005` | `AlreadyExists` |

## Transport (Low-Level)

| Code | Hex | Name |
|------|-----|------|
| 256 | `0x0100` | `TransportError` |
| 257 | `0x0101` | `TransportTimeout` |
| 258 | `0x0102` | `TransportDisconnected` |
| 259 | `0x0103` | `TransportWriteFailed` |
| 260 | `0x0104` | `TransportReadFailed` |

## Protocol

| Code | Hex | Name |
|------|-----|------|
| 512 | `0x0200` | `ProtocolError` |
| 513 | `0x0201` | `ProtocolMismatch` |
| 514 | `0x0202` | `InvalidPacket` |
| 515 | `0x0203` | `UnexpectedPacket` |
| 516 | `0x0204` | `InvalidState` |
| 517 | `0x0205` | `UnsupportedVersion` |

## Sahara NAK

| Code | Hex | Name |
|------|-----|------|
| 768 | `0x0300` | `SaharaNakSuccess` |
| 769 | `0x0301` | `SaharaNakInvalidCmd` |
| 770 | `0x0302` | `SaharaNakProtocolMismatch` |
| 771 | `0x0303` | `SaharaNakInvalidTargetProto` |
| 772 | `0x0304` | `SaharaNakInvalidHostProto` |
| 773 | `0x0305` | `SaharaNakInvalidPktSize` |
| 774 | `0x0306` | `SaharaNakUnexpectedPkt` |
| 775 | `0x0307` | `SaharaNakInvalidTransferMode` |
| 776 | `0x0308` | `SaharaNakInvalidHostId` |
| 777 | `0x0309` | `SaharaNakTimeoutRx` |
| 778 | `0x030A` | `SaharaNakTimeoutTx` |
| 779 | `0x030B` | `SaharaNakInvalidMode` |
| 780 | `0x030C` | `SaharaNakInvalidHostReq` |
| 781 | `0x030D` | `SaharaNakReadDataError` |
| 782 | `0x030E` | `SaharaNakWriteDataError` |
| 783 | `0x030F` | `SaharaNakInvalidMemTable` |
| 784 | `0x0310` | `SaharaNakInvalidMemInfo` |
| 785 | `0x0311` | `SaharaNakMemDebugNotSup` |
| 786 | `0x0312` | `SaharaNakMemReadFailed` |
| 787 | `0x0313` | `SaharaNakInvalidImgId` |
| 788 | `0x0314` | `SaharaNakImgNotFound` |
| 789 | `0x0315` | `SaharaNakImgAuthFailed` |
| 790 | `0x0316` | `SaharaNakImgTooLarge` |
| 791 | `0x0317` | `SaharaNakAuthFailed` |
| 792 | `0x0318` | `SaharaNakInvalidImgHdr` |
| 793 | `0x0319` | `SaharaNakImgHdrInvalidVer` |
| 794 | `0x031A` | `SaharaNakInvalidSignature` |
| 795 | `0x031B` | `SaharaNakInvalidHash` |
| 796 | `0x031C` | `SaharaNakFatal` |
| 797 | `0x031D` | `SaharaNakCmdExecFailure` |
| 798 | `0x031E` | `SaharaNakImgEncFailed` |
| 799 | `0x031F` | `SaharaNakImgHdrAuthFailed` |
| 800 | `0x0320` | `SaharaNakImgHdrVerFailed` |
| 801 | `0x0321` | `SaharaNakImgHdrMacFailed` |
| 802 | `0x0322` | `SaharaNakImgHdrMacNotFound` |
| 803 | `0x0323` | `SaharaNakImgHdrSignFailed` |
| 804 | `0x0324` | `SaharaNakImgHdrSignNotFound` |
| 805 | `0x0325` | `SaharaNakInvalidImgContent` |
| 806 | `0x0326` | `SaharaNakInvalidProgrammer` |
| 807 | `0x0327` | `SaharaNakProgrammerAuthFail` |
| 808 | `0x0328` | `SaharaNakProgrammerMismatch` |

## Loader

| Code | Hex | Name |
|------|-----|------|
| 1024 | `0x0400` | `LoaderNotFound` |
| 1025 | `0x0401` | `LoaderRejected` |
| 1026 | `0x0402` | `LoaderInvalidFormat` |
| 1027 | `0x0403` | `LoaderVersionMismatch` |
| 1028 | `0x0404` | `InvalidElf` |
| 1029 | `0x0405` | `CacheMiss` |

## Device

| Code | Hex | Name |
|------|-----|------|
| 1280 | `0x0500` | `DeviceNotFound` |
| 1281 | `0x0501` | `DeviceDisconnected` |
| 1282 | `0x0502` | `DeviceAccessDenied` |

## ELF

| Code | Hex | Name |
|------|-----|------|
| 1536 | `0x0600` | `ElfSegmentOverlap` |
| 1537 | `0x0601` | `ElfTruncated` |
| 1538 | `0x0602` | `ElfUnsupportedMachine` |

## Firehose

| Code | Hex | Name |
|------|-----|------|
| 1792 | `0x0700` | `FirehoseNak` |
| 1793 | `0x0701` | `FirehoseSectorOutOfRange` |
| 1794 | `0x0702` | `FirehoseProgramFailed` |
| 1795 | `0x0703` | `FirehoseEraseFailed` |
| 1796 | `0x0704` | `FirehoseUnsupportedMemory` |
| 1797 | `0x0705` | `FirehoseInvalidParam` |
| 1798 | `0x0706` | `FirehoseSeqError` |
| 1799 | `0x0707` | `FirehoseTimeout` |
| 1800 | `0x0708` | `FirehoseCrcMismatch` |
| 1801 | `0x0709` | `FirehosePartialWrite` |

## GPT / Partition

| Code | Hex | Name |
|------|-----|------|
| 2048 | `0x0800` | `GPTCorrupted` |
| 2049 | `0x0801` | `GPTInvalidHeader` |
| 2050 | `0x0802` | `GPTInvalidEntry` |
| 2051 | `0x0803` | `GPTPrimaryMissing` |
| 2052 | `0x0804` | `GPTBackupMissing` |
| 2053 | `0x0805` | `GPTCrcMismatch` |
| 2054 | `0x0806` | `PartitionNotFound` |
| 2055 | `0x0807` | `PartitionOverlap` |
| 2056 | `0x0808` | `InvalidImage` |
| 2057 | `0x0809` | `ImageTooLarge` |

## Discovery

| Code | Hex | Name |
|------|-----|------|
| 2304 | `0x0900` | `DeviceNotIdentified` |
| 2305 | `0x0901` | `NegotiationFailed` |
| 2306 | `0x0902` | `NoMatchingProtocol` |
| 2307 | `0x0903` | `EnumerationFailed` |
| 2308 | `0x0904` | `ProbeFailed` |
| 2309 | `0x0905` | `DetectorConflict` |
| 2310 | `0x0906` | `RegistryEmpty` |

## Session

| Code | Hex | Name |
|------|-----|------|
| 2560 | `0x0A00` | `SessionNotConnected` |
| 2561 | `0x0A01` | `SessionAlreadyConnected` |
| 2562 | `0x0A02` | `SessionBusy` |
| 2563 | `0x0A03` | `SessionTimeout` |
| 2564 | `0x0A04` | `SessionTerminal` |
| 2565 | `0x0A05` | `SessionLimitExceeded` |

## Plugin

| Code | Hex | Name |
|------|-----|------|
| 2816 | `0x0B00` | `PluginLoadFailed` |
| 2817 | `0x0B01` | `PluginInitFailed` |
| 2818 | `0x0B02` | `PluginShutdownFailed` |
| 2819 | `0x0B03` | `PluginNotFound` |
| 2820 | `0x0B04` | `PluginIncompatible` |
| 2821 | `0x0B05` | `PluginDependencyMissing` |
| 2822 | `0x0B06` | `PluginCircularDependency` |
| 2823 | `0x0B07` | `PluginDuplicate` |
| 2824 | `0x0B08` | `PluginAlreadyLoaded` |
| 2825 | `0x0B09` | `PluginNotLoaded` |
| 2826 | `0x0B0A` | `PluginRegistrationFailed` |
| 2827 | `0x0B0B` | `PluginUnregistrationFailed` |
| 2828 | `0x0B0C` | `PluginVersionMismatch` |
| 2829 | `0x0B0D` | `PluginConfigInvalid` |

## Job

| Code | Hex | Name |
|------|-----|------|
| 3072 | `0x0C00` | `JobFailed` |
| 3073 | `0x0C01` | `JobCancelled` |
| 3074 | `0x0C02` | `JobTimeout` |
| 3075 | `0x0C03` | `JobInvalidState` |
| 3076 | `0x0C04` | `JobDependencyFailed` |
| 3077 | `0x0C05` | `JobNotFound` |
| 3078 | `0x0C06` | `JobAlreadyRunning` |
| 3079 | `0x0C07` | `JobQueueFull` |
| 3080 | `0x0C08` | `JobRecoveryFailed` |
| 3081 | `0x0C09` | `JobRollbackFailed` |
| 3082 | `0x0C0A` | `JobInvalidConfig` |
| 3083 | `0x0C0B` | `JobDeviceNotReady` |
| 3084 | `0x0C0C` | `JobDataMismatch` |
| 3085 | `0x0C0D` | `JobInsufficientSpace` |

## Firmware

| Code | Hex | Name |
|------|-----|------|
| 3328 | `0x0D00` | `FirmwareInvalidFormat` |
| 3329 | `0x0D01` | `FirmwareImageNotFound` |
| 3330 | `0x0D02` | `FirmwareHashMismatch` |
| 3331 | `0x0D03` | `FirmwareInvalidManifest` |
| 3332 | `0x0D04` | `FirmwareDependencyConflict` |
| 3333 | `0x0D05` | `FirmwareUnsupportedDevice` |
| 3334 | `0x0D06` | `FirmwareUnsupportedVendor` |
| 3335 | `0x0D07` | `FirmwareUnsupportedStorage` |
| 3336 | `0x0D08` | `FirmwareDuplicatePartition` |
| 3337 | `0x0D09` | `FirmwareMissingProgrammer` |
| 3338 | `0x0D0A` | `FirmwareValidationFailed` |
| 3339 | `0x0D0B` | `FirmwareExtractionFailed` |
| 3340 | `0x0D0C` | `FirmwarePackageCorrupted` |
| 3341 | `0x0D0D` | `FirmwareVersionMismatch` |
| 3342 | `0x0D0E` | `FirmwarePackageNotFound` |
| 3343 | `0x0D0F` | `FirmwareNotEnoughImages` |

## Workflow

| Code | Hex | Name |
|------|-----|------|
| 3584 | `0x0E00` | `WorkflowInvalidState` |
| 3585 | `0x0E01` | `WorkflowStepFailed` |
| 3586 | `0x0E02` | `WorkflowRollbackFailed` |
| 3587 | `0x0E03` | `WorkflowCancelled` |
| 3588 | `0x0E04` | `WorkflowTimeout` |
| 3589 | `0x0E05` | `WorkflowStepNotFound` |
| 3590 | `0x0E06` | `WorkflowExecutionFailed` |
| 3591 | `0x0E07` | `WorkflowRecoveryFailed` |

## Transport (High-Level)

| Code | Hex | Name |
|------|-----|------|
| 3840 | `0x0F00` | `TransportNotOpen` |
| 3841 | `0x0F01` | `TransportAlreadyOpen` |
| 3842 | `0x0F02` | `TransportBusy` |
| 3843 | `0x0F03` | `TransportInvalidEndpoint` |
| 3844 | `0x0F04` | `TransportBufferFull` |
| 3845 | `0x0F05` | `TransportPartialTransfer` |
| 3846 | `0x0F06` | `TransportReconnectFailed` |
| 3847 | `0x0F07` | `TransportHotplugRemoved` |
| 3848 | `0x0F08` | `TransportHotplugArrived` |
| 3849 | `0x0F09` | `TransportEnumerationFailed` |
| 3850 | `0x0F0A` | `TransportBackendUnavailable` |
| 3851 | `0x0F0B` | `TransportAsyncCancelled` |
| 3852 | `0x0F0C` | `TransportRetryExhausted` |
| 3853 | `0x0F0D` | `TransportBufferTooSmall` |

## Crypto

| Code | Hex | Name |
|------|-----|------|
| 4096 | `0x1000` | `CryptoInitializationFailed` |
| 4097 | `0x1001` | `CryptoHashFailed` |
| 4098 | `0x1002` | `CryptoSignatureVerifyFailed` |
| 4099 | `0x1003` | `CryptoKeyLoadFailed` |
| 4100 | `0x1004` | `CryptoInvalidPadding` |
| 4101 | `0x1005` | `CryptoVerificationFailed` |

**Total: 146 error codes** (including `Success`)

Source: `lib/include/mbootcore/domain/Error.hpp`
