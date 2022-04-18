#pragma once

#include <cinttypes>

namespace common {
class JsonStreamGenerator;
}  // namespace common

namespace nma {

enum class Errors : uint8_t {
    UserDoesntHavePermission = 1,
    ClosedForAll,
    DevicesLocked,
    CantEditUser,
    CantGetEzlosForThisUser,
    CantGetTokenForThisUser,
    UnknownPermission,
    BadPassword,
    UserHasLogged,
    WrongCurrentPassword,
    CantDeleteUser,
    CantBlockUser,
    CantEditEmail,
    UserExists,
    OwnerCantBeUnbound,
    DBError,  // problems during working with DB

    EmailIsNotConfirmed,
    UserNotFound,
    KeyNotFound,
    EmailNotExists,
    CantGetHash,
    UserBlocked,
    UserDeleted,
    UserDisabled,
    UserNotLogged,

    TokenGenerateFail,
    NoTokenFound,
    TokenHasNoUser,
    TokenHasNoSerial,
    TokenUsed,
    TokenExpired,

    ImageTooLarge,
    ImageNotFound,

    EzloNotRegistered,
    EzloNotFound,
    EzloNotConnected,
    EzloNotLinked,
    CantSendToEzlo,
    EzloDeleted,
    EzloBecomeOnline,

    BedBackendPass,
    EzloAlreadyRegisteredOnNma,
    NoBackups,
    AdapterUnsupported,
    AdapterUnavailable,

    EzloAlreadyRegistered,
    CantRunScriptAfterRestoreBackup,
    StopScriptRunError,
    WrongBackupVersion,
    BadBackupData,
    NoBackupDataPresent,
    Uncompressrror,
    UnpackedSizeError,
    MD5Error,
    UnknownDataType,
    NoEnvmSectionInBackup,
    NoZddxSectionInBackup,
    NoZddxSizeSectionInBackup,
    NoHomeIdSectionInBackup,
    EnvmUncompressError,
    ZddxUncompressError,
    StickWriteError,

    CantDownloadFirmware,  // here UpdateFirmware errors start
    CantDownloadData,
    CantVerified,
    CantDecrypt,
    CantUnpack,
    UpdateZddxEerror,
    DeprndenciesUpdateError,
    UpdateOperationsError,
    UnexpectedError,

    SendFileError,
    ErrorOpeningFile,
    SetLoglevelError,

    ScriptOpenError,
    ScriptError,
    RemoveNetError,
    AddNetError,
    CheckError,
    ResetWifiError,

    NotSentToGateway,
    ZwaveBusy,
    GatewayBusy,
    NoDeviceFound,
    NoSubdeviceFound,
    SuchNameAlreadyExists,

    RuleWithoutCondition,
    RuleWithoutAction,
    RuleIsOverlapping,

    RemotePortForwardingFailed,

    StaticServerUnavailable,
    StaticServerBadJson,
    StaticServerBadData,

    NmaTemplateExists,
    MediaStreamSubsystemFailure,
    MediaStreamInvalidPageId,
    FFMpegFailureAdd,
    FFMpegFailureRemove,
    FFMpegFailureCfgLoad,
    FFMpegFailureCfgSave,
    FFMpegFailureStreamRegistered,
    FFMpegFailureStreamRun,
    FFMpegFailureStreamStop,
    FFMpegFailureStreamRestart,
    FFMpegFailureStreamStatus,

    ShutdownModeEntered,
    RpcCallQueueOverflow,
    SpreadBusIsNotOperatable,
    SpreadBusError,
    ObjectNotFound,
    ConfirmationEmailFailure,
    EzloHubSerialNunberIsInvalid,

    BadRequestHashNotExists,
    BadParams,
    UnexpectedRequest,

    BadUserData,
    TokenCreationError,
    KeyGenerationError,
    CompressError,
    DataSaveError,
    MailSendingError,
    InternalOperationError,
    InternalUnhandledExceptionError,
    LengthyAsyncOperationError,
    UncancalanbleOperationIsInProgress,

    SerialTooShort,
    PasswordIsTooShort,
    KeyIsEmpty,
    TokenEmpty,
    HashIsEmpty,
    IdIsEmpty,
    ParentIdIsEmpty,
    MessageHasEmptyMethod,
    WrongUnitAvailableUnits,
    UnknownConditionFilter,
    WrongParams,
    EmptyField,

    UnknownMethod,
    LastElement  // put any new elements before this last element
};

void writeErrorBlock( common::JsonStreamGenerator& jsonGenerator, Errors error );

}  // namespace nma