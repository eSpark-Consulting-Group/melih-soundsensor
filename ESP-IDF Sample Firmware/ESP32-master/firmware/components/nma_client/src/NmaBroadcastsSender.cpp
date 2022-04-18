
#include <znet/znet_provisioning_list_entry.hpp>

#include <NmaCommand.h>
#include <NmaBroadcastsSender.h>
#include <NmaErrors.h>

#include <DeviceInfo.h>
#include <JsonStreamGenerator.h>
#include <CommonResources.h>
#include <DebugUtils.h>
#include <GatewayUtils.h>
#include <DeviceSettingsManager.h>

#include <Bus.h>
#include <BusCommandNmaClientClbk.h>
#include <Ezlo.h>
#include <Utils.h>
#include <GatewayUtils.h>
#include <GatewayID.h>
#include <values/TableValue.h>

namespace nma {
using namespace common::resources;
using namespace std::chrono_literals;

bool BroadcastsSender::writeDefaultHeader( common::JsonStreamGenerator& jsonGenerator,
                                           const char* broadcastName,
                                           const char* operationId ) {
    bool result = jsonGenerator.writeProperty( tokens::Id, broadcasts::Id );
    result = result && jsonGenerator.writeProperty( tokens::MsgSubclass, broadcastName );
    if( operationId != nullptr ) {
        result = result && jsonGenerator.writeProperty( tokens::OperationId, operationId );
    }

    result = result && jsonGenerator.writePropertyKey( tokens::Result );
    result = result && jsonGenerator.writeOpeningObject();

    return result;
}

bool BroadcastsSender::sendUpdatingFirmware( const uint8_t completed,
                                             const uint32_t operationId,
                                             UpdatingStatus statusId ) {
    auto clbk = [completed, operationId, statusId]( common::IWriteStream& ws ) -> bool {
        common::JsonStreamGenerator jsonGenerator( ws );

        std::array<char, 16> buffer;
        bool result = sprintf( buffer.data(), "AT32-%" PRIX32, operationId ) != 0;
        result = result && writeDefaultHeader( jsonGenerator, broadcasts::HubFirmwareUpdateProgress );

        result = result && jsonGenerator.writeProperty( tokens::Completed, completed );
        result = result && jsonGenerator.writeProperty( tokens::Error, nullptr );
        result = result && jsonGenerator.writeProperty( tokens::OperationId, buffer.data() );
        result = result && jsonGenerator.writeProperty( tokens::Serial, core::serial() );

        const char* statusStr = nullptr;
        switch( statusId ) {
        case Started:
            statusStr = broadcasts::events::Started;
            break;
        case Process:
            statusStr = broadcasts::events::Process;
            break;
        case Finished:
            statusStr = broadcasts::events::Finished;
            break;
        case Aborted:
            statusStr = broadcasts::events::Aborted;
            break;
        }

        return result && jsonGenerator.writeProperty( tokens::Status, statusStr );
    };

    constexpr auto CmdTimeoutSec = std::chrono::seconds( 5 );
    return bus::waitToResp<bus::command::nma::ClientClbk>( "nma", CmdTimeoutSec, std::move( clbk ) );
}

bool BroadcastsSender::sendUpdatingFirmwareError( Errors error, const uint32_t operationId ) {
    auto clbk = [&error, &operationId]( common::IWriteStream& nmaStream ) -> bool {
        common::JsonStreamGenerator jsonGenerator( nmaStream );

        std::array<char, 16> buffer;
        bool result = sprintf( buffer.data(), "AT32-%" PRIX32, operationId ) != 0;

        result = result && writeDefaultHeader( jsonGenerator, broadcasts::HubFirmwareUpdateProgress );

        result = result && jsonGenerator.writeProperty( tokens::Completed, 0 );

        result = result && jsonGenerator.writePropertyKey( tokens::Error );

        nma::writeErrorBlock( jsonGenerator, error );

        result = result && jsonGenerator.writeProperty( tokens::OperationId, buffer.data() );
        result = result && jsonGenerator.writeProperty( tokens::Serial, core::serial() );
        result = result && jsonGenerator.writeProperty( tokens::Status, broadcasts::events::Aborted );
        result = result && jsonGenerator.writeProperty( tokens::UnpackResult, (int)nma::Errors::LastElement - 1 );
        return result && jsonGenerator.writeJsonEnd();
    };

    constexpr auto CmdTimeoutSec = std::chrono::seconds( 5 );
    return bus::waitToResp<bus::command::nma::ClientClbk>( "nma", CmdTimeoutSec, std::move( clbk ) );
}

bool BroadcastsSender::sendDescriptionEdited( const char* changedValue ) {
    auto clbk = [&changedValue]( common::IWriteStream& nmaStream ) -> bool {
        common::JsonStreamGenerator jsonGenerator( nmaStream );

        bool result = true;
        result = result && writeDefaultHeader( jsonGenerator, broadcasts::DescriptionEdited );

        result = result && jsonGenerator.writeProperty( tokens::Description, changedValue );

        return result && jsonGenerator.writeJsonEnd();
    };

    constexpr auto CmdTimeoutSec = std::chrono::seconds( 5 );
    return bus::waitToResp<bus::command::nma::ClientClbk>( "nma", CmdTimeoutSec, std::move( clbk ) );
}

/*
bool BroadcastsSender::sendS2AuthMode( uint8_t authModes, bool isReplace ) {
    CloudNmaWriteStream nmaStream;
    common::JsonStreamGenerator jsonGenerator( nmaStream );

    auto result = BroadcastsSender::writeDefaultHeader( jsonGenerator, broadcasts::FoGatewayMethod );

    result = result && jsonGenerator.writeProperty( tokens::GatewayId, core::serial() );
    result = result && jsonGenerator.writeProperty( tokens::Event, broadcasts::events::S2AuthMode );
    result = result && jsonGenerator.writePropertyKey( tokens::Masks );
    result = result && jsonGenerator.writeOpeningArray();
    if( authModes & S2AuthModeS2 ) {
        result = result && jsonGenerator.writeProperty( nullptr, broadcasts::s2::modes::S2 );
    }
    if( authModes & S2AuthModeS2Authenticated ) {
        result = result && jsonGenerator.writeProperty( nullptr, broadcasts::s2::modes::S2Authenticated );
    }
    if( authModes & S2AuthModeS2Unauthenticated ) {
        result = result && jsonGenerator.writeProperty( nullptr, broadcasts::s2::modes::S2Unauthenticated );
    }
    if( authModes & S2AuthModeS0 ) {
        result = result && jsonGenerator.writeProperty( nullptr, broadcasts::s2::modes::S0 );
    }
    if( authModes & S2AuthModeCSA ) {
        result = result && jsonGenerator.writeProperty( nullptr, broadcasts::s2::modes::CSA );
    }
    result = result && jsonGenerator.writeClosingSymbol();
    result = result && jsonGenerator.writeProperty( tokens::IsReplace, isReplace );

    result = result && jsonGenerator.writeJsonEnd();

    return nmaStream.close() && result;
}

bool BroadcastsSender::sendS2SendKey( const char* event, const uint16_t key[], size_t keySize ) {
    CloudNmaWriteStream nmaStream;
    common::JsonStreamGenerator jsonGenerator( nmaStream );

    auto result = BroadcastsSender::writeDefaultHeader( jsonGenerator, broadcasts::FoGatewayMethod );

    result = result && jsonGenerator.writeProperty( tokens::GatewayId, core::serial() );
    result = result && jsonGenerator.writeProperty( tokens::Event, event );
    result = result && jsonGenerator.writePropertyKey( tokens::Key );
    result = result && jsonGenerator.writeOpeningArray();
    for( size_t i = 0; i < keySize; ++i ) {
        char buff[6] = {};  // uint16_t, base 10
        snprintf( buff, sizeof( buff ), "%05" PRIu16, key[i] );
        result = result && jsonGenerator.writeProperty( nullptr, buff );
    }
    result = result && jsonGenerator.writeClosingSymbol();

    result = result && jsonGenerator.writeJsonEnd();

    return nmaStream.close() && result;
}

bool BroadcastsSender::sendS2SendDSK( const uint16_t key[], size_t keySize ) {
    return sendS2SendKey( broadcasts::events::S2SendDSK, key, keySize );
}

bool BroadcastsSender::sendS2SendFullDSK( const uint16_t key[], size_t keySize ) {
    return sendS2SendKey( broadcasts::events::S2SendFullDSK, key, keySize );
}

bool BroadcastsSender::sendS2SendCSAKey( const uint16_t key[], size_t keySize ) {
    return sendS2SendKey( broadcasts::events::S2SendCSAKey, key, keySize );
}

bool BroadcastsSender::sendS2Finished( const char* authResult ) {
    CloudNmaWriteStream nmaStream;
    common::JsonStreamGenerator jsonGenerator( nmaStream );

    auto result = BroadcastsSender::writeDefaultHeader( jsonGenerator, broadcasts::FoGatewayMethod );

    result = result && jsonGenerator.writeProperty( tokens::GatewayId, core::serial() );
    result = result && jsonGenerator.writeProperty( tokens::Event, broadcasts::events::S2Finished );
    result = result && jsonGenerator.writeProperty( tokens::Auth, authResult );

    result = result && jsonGenerator.writeJsonEnd();

    return nmaStream.close() && result;
}
*/

bool BroadcastsSender::sendFoDevicePageUpdated( gateways::GatewayID gatewayID,
                                                const gateways::DeviceId& deviceId,
                                                const core::PagesManager::PageId& pageId ) {
    bool result = true;
    if( pageId != 0 ) {
        std::array<char, 10> buffer;
        result = result && ( snprintf( buffer.data(), buffer.size(), "%" PRIX32, pageId ) != 0 );
    }

    return result;  // && sendFoDeviceUpdatedByKey( gatewayID, deviceId, tokens::PageId, buffer.get<char>() );
}

bool BroadcastsSender::sendFeatureStatusUpdate( const ::core::features::FeatureFlag& featureFlag ) {
    auto clbk = [&featureFlag]( common::IWriteStream& nmaStream ) -> bool {
        common::JsonStreamGenerator jsonGenerator( nmaStream );

        bool result = writeDefaultHeader( jsonGenerator, broadcasts::HubFeatureStatusUpdate );
        result = result && jsonGenerator.writeProperty( tokens::Feature, featureFlag.typeStr() );
        result = result && jsonGenerator.writeProperty( tokens::Status, featureFlag.statusStr() );
        result = result && jsonGenerator.writeJsonEnd();
        return result;
    };

    constexpr auto CmdTimeoutSec = std::chrono::seconds( 5 );
    return bus::waitToResp<bus::command::nma::ClientClbk>( "nma", CmdTimeoutSec, std::move( clbk ) );
}

bool BroadcastsSender::sendSettingStatusUpdateToken( const std::string& name, const std::string& value ) {
    auto clbk = [name, value]( common::IWriteStream& nmaStream ) -> bool {
        common::JsonStreamGenerator jsonGenerator( nmaStream );

        bool result = writeDefaultHeader( jsonGenerator, broadcasts::HubSettingsUpdated );
        result = result && jsonGenerator.writeProperty( tokens::Name, name );
        result = result && jsonGenerator.writeProperty( tokens::Value, value );
        result = result && jsonGenerator.writeJsonEnd();
        return result;
    };

    constexpr auto CmdTimeoutSec = std::chrono::seconds( 5 );
    return bus::waitToResp<bus::command::nma::ClientClbk>( "nma", CmdTimeoutSec, std::move( clbk ) );
}

bool BroadcastsSender::sendSettingStatusUpdateDictionary( const std::string& name,
                                                          const std::string& key,
                                                          const int value ) {
    auto clbk = [name, key, value]( common::IWriteStream& nmaStream ) -> bool {
        common::JsonStreamGenerator jsonGenerator( nmaStream );

        bool result = writeDefaultHeader( jsonGenerator, broadcasts::HubSettingsUpdated );
        result = result && jsonGenerator.writeProperty( tokens::Name, name );
        result = result && jsonGenerator.writeProperty( tokens::Key, key );
        result = result && jsonGenerator.writeProperty( tokens::Value, value );
        result = result && jsonGenerator.writeJsonEnd();
        return result;
    };

    constexpr auto CmdTimeoutSec = std::chrono::seconds( 5 );
    return bus::waitToResp<bus::command::nma::ClientClbk>( "nma", CmdTimeoutSec, std::move( clbk ) );
}

bool BroadcastsSender::sendSettingStatusUpdateFlag( const std::string& name, bool value ) {
    auto clbk = [name, value]( common::IWriteStream& nmaStream ) -> bool {
        common::JsonStreamGenerator jsonGenerator( nmaStream );

        bool result = writeDefaultHeader( jsonGenerator, broadcasts::HubSettingsUpdated );
        result = result && jsonGenerator.writeProperty( tokens::Name, name );
        result = result && jsonGenerator.writeProperty( tokens::Value, value );
        result = result && jsonGenerator.writeJsonEnd();
        return result;
    };

    constexpr auto CmdTimeoutSec = std::chrono::seconds( 5 );
    return bus::waitToResp<bus::command::nma::ClientClbk>( "nma", CmdTimeoutSec, std::move( clbk ) );
    return false;
}

void BroadcastsSender::sendProvisioningListEntryAdded( const ::znet::ProvisioningListEntry& entry ) {
    auto clbk = [entry]( common::IWriteStream& nmaStream ) -> bool {
        common::JsonStreamGenerator generator( nmaStream );
        bool result = writeDefaultHeader( generator, broadcasts::HubAddonsZwaveNodesProvisioningAdded );
        result = result && generator.writePropertyKey( provisioning::Provisioning );
        result = result && generator.writeOpeningArray();
        result = result && generator.writeOpeningObject();
        result = result && generator.writeProperty( tokens::SubId, std::to_string( entry.m_provisioningId ) );
        result = result && generator.writeProperty( provisioning::Dsk, entry.m_dsk );
        result = result && ::gateways::utils::writeProvisioningListEntryFields( generator, entry.m_fields );
        result = result && generator.writeJsonEnd();
        return result;
    };
    bus::sendFromNowhere( "nma", std::make_unique<bus::command::nma::ClientClbk>( std::move( clbk ) ) );
}
void BroadcastsSender::sendProvisioningListEntryUpdated( uint32_t entryId,
                                                         const ::znet::ProvisioningListEntryFields& diff ) {
    auto clbk = [entryId, diff]( common::IWriteStream& nmaStream ) -> bool {
        common::JsonStreamGenerator generator( nmaStream );
        bool result = writeDefaultHeader( generator, broadcasts::HubAddonsZwaveNodesProvisioningUpdated );
        result = result && generator.writePropertyKey( provisioning::Provisioning );
        result = result && generator.writeOpeningArray();
        result = result && generator.writeOpeningObject();
        result = result && generator.writeProperty( tokens::SubId, std::to_string( entryId ) );
        result = result && ::gateways::utils::writeProvisioningListEntryFields( generator, diff );
        result = result && generator.writeJsonEnd();
        return result;
    };
    bus::sendFromNowhere( "nma", std::make_unique<bus::command::nma::ClientClbk>( std::move( clbk ) ) );
}

void BroadcastsSender::sendProvisioningListEntryRemoved( uint32_t entryId ) {
    auto clbk = [entryId]( common::IWriteStream& nmaStream ) -> bool {
        common::JsonStreamGenerator generator( nmaStream );
        bool result = writeDefaultHeader( generator, broadcasts::HubAddonsZwaveNodesProvisioningRemoved );
        result = result && generator.writePropertyKey( provisioning::Provisioning );
        result = result && generator.writeOpeningArray();
        result = result && generator.writeProperty( nullptr, std::to_string( entryId ) );
        result = result && generator.writeJsonEnd();
        return result;
    };
    bus::sendFromNowhere( "nma", std::make_unique<bus::command::nma::ClientClbk>( std::move( clbk ) ) );
}

void BroadcastsSender::sendSettingDictionaryStatusUpdated( const std::string& settingId,
                                                           const std::string& elementId,
                                                           const std::string& status ) {
    auto clbk = [settingId, elementId, status](::common::IWriteStream& nmaStream ) -> bool {
        ::common::JsonStreamGenerator generator( nmaStream );
        bool result = writeDefaultHeader( generator, broadcasts::HubDeviceSettingDictionaryUpdated );
        result = result && generator.writeProperty( tokens::SubId, settingId );
        result = result && generator.writeProperty( tokens::Operation, tokens::Updated );
        result = result && generator.writePropertyKey( tokens::Element );
        result = result && generator.writeOpeningObject();
        result = result && generator.writePropertyKey( elementId.c_str() );
        result = result && generator.writeOpeningObject();
        result = result && generator.writeProperty( tokens::Status, status );
        result = result && generator.writeJsonEnd();
        return result;
    };
    bus::sendFromNowhere( "nma", std::make_unique<bus::command::nma::ClientClbk>( std::move( clbk ) ) );
}

void BroadcastsSender::sendGatewayUpdated(::gateways::GatewayID gatewayId,
                                          std::unique_ptr<::common::TableValue>&& updatedFields ) {
    if( !updatedFields || updatedFields->empty() ) {
        return;
    }
    std::shared_ptr<::common::TableValue> sharedUpdatedFields{ std::move( updatedFields ) };
    auto clbk = [gatewayId, sharedUpdatedFields](::common::IWriteStream& nmaStream ) {
        ::common::JsonStreamGenerator generator( nmaStream );
        bool result = writeDefaultHeader( generator, broadcasts::HubGatewayUpdated );
        result = result && generator.writeProperty( tokens::SubId, ::gateways::toString( gatewayId ) );
        for( auto& field : *sharedUpdatedFields ) {
            result = result && generator.writeProperty( field.first.c_str(), field.second.get() );
        }
        result = result && generator.writeJsonEnd();
        return result;
    };
    bus::sendFromNowhere( "nma", std::make_unique<bus::command::nma::ClientClbk>( std::move( clbk ) ) );
}

}  // namespace nma
