#include <NmaErrors.h>
#include <JsonStreamGenerator.h>
#include <CommonResources.h>

#include <string.h>

#include <DebugUtils.h>

namespace nma {

void writeErrorBlock( common::JsonStreamGenerator& jsonGenerator, Errors error ) {
    using namespace common::resources;
    const char* message = nullptr;
    const char* data = nullptr;
    auto result = jsonGenerator.writeOpeningObject();
    auto errorCode = errors::ApplicationError;
    switch( error ) {
    case Errors::UserDoesntHavePermission:
        data = errors::data::UserDoesntHavePermission;
        message = errors::messages::UserDoesntHavePermission;
        break;
    case Errors::ClosedForAll:
        data = errors::data::ClosedForAll;
        message = errors::messages::ClosedForAll;
        break;
    case Errors::DevicesLocked:
        data = errors::data::DevicesLocked;
        message = errors::messages::DevicesLocked;
        break;
    case Errors::CantEditUser:
        data = errors::data::CantEditUser;
        message = errors::messages::CantEditUser;
        break;
    case Errors::CantGetEzlosForThisUser:
        data = errors::data::CantGetEzlosForThisUser;
        message = errors::messages::CantGetEzlosForThisUser;
        break;
    case Errors::CantGetTokenForThisUser:
        data = errors::data::CantGetTokenForThisUser;
        message = errors::messages::CantGetTokenForThisUser;
        break;

    case Errors::UnknownPermission:
        data = errors::data::UnknownPermission;
        message = errors::messages::UnknownPermission;
        break;
    case Errors::BadPassword:
        data = errors::data::BadPassword;
        message = errors::messages::BadPassword;
        break;
    case Errors::UserHasLogged:
        data = errors::data::UserHasLogged;
        message = errors::messages::UserHasLogged;
        break;
    case Errors::WrongCurrentPassword:
        data = errors::data::WrongCurrentPassword;
        message = errors::messages::WrongCurrentPassword;
        break;
    case Errors::CantDeleteUser:
        data = errors::data::CantDeleteUser;
        message = errors::messages::CantDeleteUser;
        break;
    case Errors::CantBlockUser:
        data = errors::data::CantBlockUser;
        message = errors::messages::CantBlockUser;
        break;
    case Errors::CantEditEmail:
        data = errors::data::CantEditEmail;
        message = errors::messages::CantEditEmail;
        break;
    case Errors::UserExists:
        data = errors::data::UserExists;
        message = errors::messages::UserExists;
        break;
    case Errors::OwnerCantBeUnbound:
        data = errors::data::OwnerCantBeUnbound;
        message = errors::messages::OwnerCantBeUnbound;
        break;
    case Errors::DBError:
        data = errors::data::DBError;
        message = errors::messages::DBError;
        break;

    case Errors::EmailIsNotConfirmed:
        data = errors::data::EmailIsNotConfirmed;
        message = errors::messages::EmailIsNotConfirmed;
        break;
    case Errors::UserNotFound:
        data = errors::data::UserNotFound;
        message = errors::messages::UserNotFound;
        break;
    case Errors::KeyNotFound:
        data = errors::data::KeyNotFound;
        message = errors::messages::KeyNotFound;
        break;
    case Errors::EmailNotExists:
        data = errors::data::EmailNotExists;
        message = errors::messages::EmailNotExists;
        break;
    case Errors::CantGetHash:
        data = errors::data::CantGetHash;
        message = errors::messages::CantGetHash;
        break;
    case Errors::UserBlocked:
        data = errors::data::UserBlocked;
        message = errors::messages::UserBlocked;
        break;
    case Errors::UserDeleted:
        data = errors::data::UserDeleted;
        message = errors::messages::UserDeleted;
        break;
    case Errors::UserNotLogged:
        data = errors::data::UserNotLogged;
        message = errors::messages::UserNotLogged;
        break;
    case Errors::UserDisabled:
        data = errors::data::UserDisabled;
        message = errors::messages::UserDisabled;
        break;

    case Errors::TokenGenerateFail:
        data = errors::data::TokenGenerateFail;
        message = errors::messages::TokenGenerateFail;
        break;
    case Errors::NoTokenFound:
        data = errors::data::NoTokenFound;
        message = errors::messages::NoTokenFound;
        break;
    case Errors::TokenHasNoUser:
        data = errors::data::TokenHasNoUser;
        message = errors::messages::TokenHasNoUser;
        break;
    case Errors::TokenHasNoSerial:
        data = errors::data::TokenHasNoSerial;
        message = errors::messages::TokenHasNoSerial;
        break;
    case Errors::TokenUsed:
        data = errors::data::TokenUsed;
        message = errors::messages::TokenUsed;
        break;
    case Errors::TokenExpired:
        data = errors::data::TokenExpired;
        message = errors::messages::TokenExpired;
        break;

    case Errors::ImageTooLarge:
        data = errors::data::ImageTooLarge;
        message = errors::messages::ImageTooLarge;
        break;
    case Errors::ImageNotFound:
        data = errors::data::ImageNotFound;
        message = errors::messages::ImageNotFound;
        break;

    case Errors::EzloNotRegistered:
        data = errors::data::EzloNotRegistered;
        message = errors::messages::EzloNotRegistered;
        break;
    case Errors::EzloNotFound:
        data = errors::data::EzloNotFound;
        message = errors::messages::EzloNotFound;
        break;
    case Errors::EzloNotConnected:
        FUNC_LOGD( "Errors::EzloNotConnected" );
        data = errors::data::EzloNotConnected;
        message = errors::messages::EzloNotConnected;
        break;
    case Errors::EzloNotLinked:
        data = errors::data::EzloNotLinked;
        message = errors::messages::EzloNotLinked;
        break;
    case Errors::CantSendToEzlo:
        data = errors::data::CantSendToEzlo;
        message = errors::messages::CantSendToEzlo;
        break;
    case Errors::EzloDeleted:
        data = errors::data::EzloDeleted;
        message = errors::messages::EzloDeleted;
        break;

    case Errors::EzloBecomeOnline:
        data = errors::data::EzloBecomeOnline;
        message = errors::messages::EzloBecomeOnline;
        break;
    case Errors::BedBackendPass:
        data = errors::data::BedBackendPass;
        message = errors::messages::BedBackendPass;
        break;
    case Errors::EzloAlreadyRegisteredOnNma:
        data = errors::data::EzloAlreadyRegisteredOnNma;
        message = errors::messages::EzloAlreadyRegisteredOnNma;
        break;

    case Errors::NoBackups:
        data = errors::data::NoBackups;
        message = errors::messages::NoBackups;
        break;
    case Errors::AdapterUnsupported:
        data = errors::data::AdapterUnsupported;
        message = errors::messages::AdapterUnsupported;
        break;
    case Errors::AdapterUnavailable:
        data = errors::data::AdapterUnavailable;
        message = errors::messages::AdapterUnavailable;
        break;

    case Errors::EzloAlreadyRegistered:
        data = errors::data::EzloAlreadyRegistered;
        message = errors::messages::EzloAlreadyRegistered;
        break;
    case Errors::CantRunScriptAfterRestoreBackup:
        data = errors::data::CantRunScriptAfterRestoreBackup;
        message = errors::messages::CantRunScriptAfterRestoreBackup;
        break;
    case Errors::StopScriptRunError:
        data = errors::data::StopScriptRunError;
        message = errors::messages::StopScriptRunError;
        break;
    case Errors::WrongBackupVersion:
        data = errors::data::WrongBackupVersion;
        message = errors::messages::WrongBackupVersion;
        break;
    case Errors::BadBackupData:
        data = errors::data::BadBackupData;
        message = errors::messages::BadBackupData;
        break;
    case Errors::NoBackupDataPresent:
        data = errors::data::NoBackupDataPresent;
        message = errors::messages::NoBackupDataPresent;
        break;
    case Errors::Uncompressrror:
        data = errors::data::Uncompressrror;
        message = errors::messages::Uncompressrror;
        break;
    case Errors::UnpackedSizeError:
        data = errors::data::UnpackedSizeError;
        message = errors::messages::UnpackedSizeError;
        break;
    case Errors::MD5Error:
        data = errors::data::MD5Error;
        message = errors::messages::MD5Error;
        break;
    case Errors::UnknownDataType:
        data = errors::data::UnknownDataType;
        message = errors::messages::UnknownDataType;
        break;
    case Errors::NoEnvmSectionInBackup:
        data = errors::data::NoEnvmSectionInBackup;
        message = errors::messages::NoEnvmSectionInBackup;
        break;
    case Errors::NoZddxSectionInBackup:
        data = errors::data::NoZddxSectionInBackup;
        message = errors::messages::NoZddxSectionInBackup;
        break;
    case Errors::NoZddxSizeSectionInBackup:
        data = errors::data::NoZddxSizeSectionInBackup;
        message = errors::messages::NoZddxSizeSectionInBackup;
        break;
    case Errors::NoHomeIdSectionInBackup:
        data = errors::data::NoHomeIdSectionInBackup;
        message = errors::messages::NoHomeIdSectionInBackup;
        break;
    case Errors::EnvmUncompressError:
        data = errors::data::EnvmUncompressError;
        message = errors::messages::EnvmUncompressError;
        break;
    case Errors::ZddxUncompressError:
        data = errors::data::ZddxUncompressError;
        message = errors::messages::ZddxUncompressError;
        break;
    case Errors::StickWriteError:
        data = errors::data::StickWriteError;
        message = errors::messages::StickWriteError;
        break;

    case Errors::CantDownloadFirmware:
        data = errors::data::CantDownloadFirmware;
        message = errors::messages::CantDownloadFirmware;
        break;

    case Errors::CantDownloadData:
        data = errors::data::CantDownloadData;
        message = errors::messages::CantDownloadData;
        break;

    case Errors::CantVerified:
        data = errors::data::CantVerified;
        message = errors::messages::CantVerified;
        break;

    case Errors::CantDecrypt:
        data = errors::data::CantDecrypt;
        message = errors::messages::CantDecrypt;
        break;

    case Errors::CantUnpack:
        data = errors::data::CantUnpack;
        message = errors::messages::CantUnpack;
        break;

    case Errors::UpdateZddxEerror:
        data = errors::data::UpdateZddxEerror;
        message = errors::messages::UpdateZddxEerror;
        break;

    case Errors::DeprndenciesUpdateError:
        data = errors::data::DeprndenciesUpdateError;
        message = errors::messages::DeprndenciesUpdateError;
        break;

    case Errors::UpdateOperationsError:
        data = errors::data::UpdateOperationsError;
        message = errors::messages::UpdateOperationsError;
        break;

    case Errors::UnexpectedError:
        data = errors::data::UnexpectedError;
        message = errors::messages::UnexpectedError;
        break;

    case Errors::SendFileError:
        data = errors::data::SendFileError;
        message = errors::messages::SendFileError;
        break;
    case Errors::ErrorOpeningFile:
        data = errors::data::ErrorOpeningFile;
        message = errors::messages::ErrorOpeningFile;
        break;
    case Errors::SetLoglevelError:
        data = errors::data::SetLoglevelError;
        message = errors::messages::SetLoglevelError;
        break;

    case Errors::ScriptOpenError:
        data = errors::data::ScriptOpenError;
        message = errors::messages::ScriptOpenError;
        break;
    case Errors::ScriptError:
        data = errors::data::ScriptError;
        message = errors::messages::ScriptError;
        break;
    case Errors::RemoveNetError:
        data = errors::data::RemoveNetError;
        message = errors::messages::RemoveNetError;
        break;
    case Errors::AddNetError:
        data = errors::data::AddNetError;
        message = errors::messages::AddNetError;
        break;
    case Errors::CheckError:
        data = errors::data::CheckError;
        message = errors::messages::CheckError;
        break;
    case Errors::ResetWifiError:
        data = errors::data::ResetWifiError;
        message = errors::messages::ResetWifiError;
        break;

    case Errors::NotSentToGateway:
        data = errors::data::NotSentToGateway;
        message = errors::messages::NotSentToGateway;
        break;
    case Errors::ZwaveBusy:
        data = errors::data::ZwaveBusy;
        message = errors::messages::ZwaveBusy;
        break;
    case Errors::GatewayBusy:
        data = errors::data::GatewayBusy;
        message = errors::messages::GatewayBusy;
        break;
    case Errors::NoDeviceFound:
        data = errors::data::NoDeviceFound;
        message = errors::messages::NoDeviceFound;
        break;
    case Errors::NoSubdeviceFound:
        data = errors::data::NoSubdeviceFound;
        message = errors::messages::NoSubdeviceFound;
        break;
    case Errors::SuchNameAlreadyExists:
        data = errors::data::SuchNameAlreadyExists;
        message = errors::messages::SuchNameAlreadyExists;
        break;

    case Errors::RuleWithoutCondition:  // no defined const strings
        break;
    case Errors::RuleWithoutAction:
        break;
    case Errors::RuleIsOverlapping:
        break;
    case Errors::RemotePortForwardingFailed:
        break;
    case Errors::StaticServerUnavailable:
        break;
    case Errors::StaticServerBadJson:
        break;
    case Errors::StaticServerBadData:
        break;
    case Errors::NmaTemplateExists:
        break;
    case Errors::MediaStreamSubsystemFailure:
        break;
    case Errors::MediaStreamInvalidPageId:
        break;
    case Errors::FFMpegFailureAdd:
        break;
    case Errors::FFMpegFailureRemove:
        break;
    case Errors::FFMpegFailureCfgLoad:
        break;
    case Errors::FFMpegFailureCfgSave:
        break;
    case Errors::FFMpegFailureStreamRegistered:
        break;
    case Errors::FFMpegFailureStreamRun:
        break;
    case Errors::FFMpegFailureStreamStop:
        break;
    case Errors::FFMpegFailureStreamRestart:
        break;
    case Errors::FFMpegFailureStreamStatus:
        break;  // no defined const strings
    case Errors::ShutdownModeEntered:
        data = errors::data::ShutdownModeEntered;
        message = errors::messages::ShutdownModeEntered;
        break;
    case Errors::RpcCallQueueOverflow:
        data = errors::data::RpcCallQueueOverflow;
        message = errors::messages::RpcCallQueueOverflow;
        break;
    case Errors::SpreadBusIsNotOperatable:
        data = errors::data::SpreadBusIsNotOperatable;
        message = errors::messages::SpreadBusIsNotOperatable;
        break;
    case Errors::SpreadBusError:
        data = errors::data::SpreadBusError;
        message = errors::messages::SpreadBusError;
        break;
    case Errors::ObjectNotFound:
        data = errors::data::ObjectNotFound;
        message = errors::messages::ObjectNotFound;
        break;
    case Errors::ConfirmationEmailFailure:
        data = errors::data::ConfirmationEmailFailure;
        message = errors::messages::ConfirmationEmailFailure;
        break;
    case Errors::EzloHubSerialNunberIsInvalid:
        data = errors::data::EzloHubSerialNunberIsInvalid;
        message = errors::messages::EzloHubSerialNunberIsInvalid;
        break;

    case Errors::BadRequestHashNotExists:  // error code is InvalidRequest
        errorCode = errors::InvalidRequest;
        data = errors::data::BadRequestHashNotExists;
        message = errors::messages::BadRequestHashNotExists;
        break;
    case Errors::BadParams:
        errorCode = errors::InvalidRequest;
        data = errors::data::BadParams;
        message = errors::messages::BadParams;
        break;
    case Errors::UnexpectedRequest:
        errorCode = errors::InvalidRequest;
        data = errors::data::UnexpectedRequest;
        message = errors::messages::UnexpectedRequest;
        break;

    case Errors::BadUserData:  // error code is InternalError
        errorCode = errors::InternalError;
        data = errors::data::BadUserData;
        message = errors::messages::BadUserData;
        break;
    case Errors::TokenCreationError:
        errorCode = errors::InternalError;
        data = errors::data::TokenCreationError;
        message = errors::messages::TokenCreationError;
        break;
    case Errors::KeyGenerationError:
        errorCode = errors::InternalError;
        data = errors::data::KeyGenerationError;
        message = errors::messages::KeyGenerationError;
        break;
    case Errors::CompressError:
        errorCode = errors::InternalError;
        data = errors::data::CompressError;
        message = errors::messages::CompressError;
        break;
    case Errors::DataSaveError:
        errorCode = errors::InternalError;
        data = errors::data::DataSaveError;
        message = errors::messages::DataSaveError;
        break;
    case Errors::MailSendingError:
        errorCode = errors::InternalError;
        data = errors::data::MailSendingError;
        message = errors::messages::MailSendingError;
        break;
    case Errors::InternalOperationError:
        errorCode = errors::InternalError;
        data = errors::data::InternalOperationError;
        message = errors::messages::InternalOperationError;
        break;
    case Errors::InternalUnhandledExceptionError:
        errorCode = errors::InternalError;
        data = errors::data::InternalUnhandledExceptionError;
        message = errors::messages::InternalUnhandledExceptionError;
        break;
    case Errors::LengthyAsyncOperationError:
        errorCode = errors::InternalError;
        data = errors::data::LengthyAsyncOperationError;
        message = errors::messages::LengthyAsyncOperationError;
        break;
    case Errors::UncancalanbleOperationIsInProgress:
        errorCode = errors::InternalError;
        data = errors::data::UncancalanbleOperationIsInProgress;
        message = errors::messages::UncancalanbleOperationIsInProgress;
        break;  // error code is InternalError

    case Errors::SerialTooShort:  // error code is InvalidParams
        errorCode = errors::InvalidParams;
        data = errors::data::SerialTooShort;
        message = errors::messages::SerialTooShort;
        break;
    case Errors::PasswordIsTooShort:
        errorCode = errors::InvalidParams;
        data = errors::data::PasswordIsTooShort;
        message = errors::messages::PasswordIsTooShort;
        break;
    case Errors::KeyIsEmpty:
        errorCode = errors::InvalidParams;
        data = errors::data::KeyIsEmpty;
        message = errors::messages::KeyIsEmpty;
        break;
    case Errors::TokenEmpty:
        errorCode = errors::InvalidParams;
        data = errors::data::TokenEmpty;
        message = errors::messages::TokenEmpty;
        break;
    case Errors::HashIsEmpty:
        errorCode = errors::InvalidParams;
        data = errors::data::HashIsEmpty;
        message = errors::messages::HashIsEmpty;
        break;
    case Errors::IdIsEmpty:
        errorCode = errors::InvalidParams;
        data = errors::data::IdIsEmpty;
        message = errors::messages::IdIsEmpty;
        break;
    case Errors::ParentIdIsEmpty:
        errorCode = errors::InvalidParams;
        data = errors::data::ParentIdIsEmpty;
        message = errors::messages::ParentIdIsEmpty;
        break;
    case Errors::MessageHasEmptyMethod:
        errorCode = errors::InvalidParams;
        data = errors::data::MessageHasEmptyMethod;
        message = errors::messages::MessageHasEmptyMethod;
        break;
    case Errors::WrongUnitAvailableUnits:
        errorCode = errors::InvalidParams;
        data = errors::data::WrongUnitAvailableUnits;
        message = errors::messages::WrongUnitAvailableUnits;
        break;
    case Errors::UnknownConditionFilter:
        errorCode = errors::InvalidParams;
        data = errors::data::UnknownConditionFilter;
        message = errors::messages::UnknownConditionFilter;
        break;
    case Errors::WrongParams:
        errorCode = errors::InvalidParams;
        data = errors::data::WrongParams;
        message = errors::messages::WrongParams;
        break;
    case Errors::EmptyField:
        errorCode = errors::InvalidParams;
        data = errors::data::EmptyField;
        message = errors::messages::EmptyField;
        break;  // error code is InvalidParams

    case Errors::UnknownMethod:  // error code is MethodNotFound
        errorCode = errors::MethodNotFound;
        data = errors::data::UnknownMethod;
        message = errors::messages::UnknownMethod;
        break;
    case Errors::LastElement:
        // Do nothing! It marks last element of enum only!
        break;
    };

    if( data != nullptr ) {
        result = result && jsonGenerator.writeProperty( tokens::Data, data );
    }

    result = result && jsonGenerator.writeProperty( tokens::Code, errorCode );

    if( message != nullptr ) {
        result = result && jsonGenerator.writeProperty( tokens::Message, message );
    }

    result = result && jsonGenerator.writeClosingSymbol();
}
}  // namespace nma
