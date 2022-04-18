#include <ZWaveDeviceSetting.h>
#include <StorageArchiveFileFactory.h>
#include <plugin_manager.h>
#include <DebugUtils.h>
#include <JsonStreamingParser.h>
#include "IValueCast.h"

namespace gateways {
namespace zwave {

NodeSetting::NodeSetting( std::string path, uint16_t manufacturerId, uint16_t productType, uint16_t productId )
: m_manufacturerId( manufacturerId )
, m_productType( productType )
, m_productId( productId ) {
    auto plugin_id_begin = std::find( path.begin(), path.end(), ':' );
    if( plugin_id_begin == path.end() ) {
        plugin_id_begin = path.begin();
    }
    auto plugin_id_end = std::find( plugin_id_begin, path.end(), '/' );
    if( plugin_id_end != path.end() && ( plugin_id_end + 1 ) != path.end() ) {
        if( *plugin_id_begin == ':' ) {
            ++plugin_id_begin;
        }

        std::string plugin_id( plugin_id_begin, plugin_id_end );

        static const std::string ext( ".json" );
        m_fileName.reserve( std::distance( plugin_id_end + 1, path.end() ) + ext.size() );
        m_fileName.assign( plugin_id_end + 1, path.end() );
        if( m_fileName.size() < ext.size() ||
            m_fileName.compare( m_fileName.size() - ext.size(), ext.size(), ext ) != 0 ) {
            m_fileName.append( ext );
        }

        const auto& plugins_list = plugin::PluginManager::get().getInstalledPluginList();
        const auto found_plugin =
            std::find_if( plugins_list.begin(), plugins_list.end(), [&plugin_id]( const auto& plugin ) {
                return plugin.plugin.id == plugin_id;
            } );

        if( found_plugin == plugins_list.end() ) {
            return;
        }
        m_archiveName = found_plugin->uri;
        FUNC_LOGD( "m_archiveName %s", m_archiveName.c_str() );
    }
}

void NodeSetting::getFieldValue( std::function<void( jssp::JsonListenerDeviceSetting& )> getter ) const {
    if( m_archiveName.empty() ) {
        FUNC_LOGE( "m_archiveName is empty %s", m_archiveName.c_str() );
        return;
    }
    auto plugin_file = storage::ArchiveFileFactory::getArchiveFile( m_archiveName.c_str() );
    if( !plugin_file ) {
        FUNC_LOGE( "Can't open archive %s", m_archiveName.c_str() );
        return;
    }
    if( !plugin_file->open( m_archiveName.c_str(), m_fileName.c_str() ) ) {
        return;
    }

    char device_id[32];
    sprintf( device_id, "%u_%u_%u", m_manufacturerId, m_productType, m_productId );
    FUNC_LOGD( "device_id %s", device_id );

    std::unique_ptr<jssp::JsonStreamingParser> jsonParser = std::make_unique<jssp::JsonStreamingParser>();
    jssp::JsonListenerDeviceSetting jsonListener( device_id );
    jsonParser->setListener( &jsonListener );
    plugin_file->open( m_archiveName.c_str(), m_fileName.c_str() );
    size_t file_buffer_length = plugin_file->size();
    size_t read_bytes{ 0u };
    char byte;
    for( auto i = 0; i < file_buffer_length; ++i ) {
        if( !plugin_file->read( &byte, 1, &read_bytes ) || read_bytes != 1 ) {
            FUNC_LOGE( "Fail reading <%s> file in <%s> archive", m_fileName.c_str(), m_archiveName.c_str() );
            plugin_file->close();
            return;
        }
        jsonParser->parse( byte );
    }
    plugin_file->close();

    getter( jsonListener );
}

void NodeSetting::forEachConfiguration( std::function<void( const znet::Configuration& )> callback ) const {
    getFieldValue( [&callback]( jssp::JsonListenerDeviceSetting& jsonListener ) {
        const auto& table = jsonListener.getConfigurationsTable();

        if( !table.empty() ) {
            for( auto itr = table.cbegin(); itr != table.cend(); ++itr ) {
                const auto tableVal = common::iValueCast<common::TableValue>( itr->second.get() );

                if( !tableVal->isKeyExist( "number" ) ) {
                    FUNC_LOGW( "Could not get field number" );
                    continue;
                }
                auto number = ( common::iValueCast<common::IntValue>( ( *tableVal )["number"] ) )->get();

                if( !tableVal->isKeyExist( "value" ) ) {
                    FUNC_LOGW( "Could not get field value" );
                    continue;
                }
                auto value = ( common::iValueCast<common::IntValue>( ( *tableVal )["value"] ) )->get();

                if( !tableVal->isKeyExist( "size" ) ) {
                    FUNC_LOGW( "Could not get field size" );
                    continue;
                }
                auto size = ( common::iValueCast<common::IntValue>( ( *tableVal )["size"] ) )->get();

                callback( { static_cast<uint8_t>( number ),
                            static_cast<znet_cmd_configuration_value_t>( value ),
                            static_cast<uint8_t>( size ) } );
            }
        }
    } );
}

void NodeSetting::forEachAssociation( std::function<void( const znet::Association& )> callback ) const {
    getFieldValue( [&callback]( jssp::JsonListenerDeviceSetting& jsonListener ) {
        const auto& table = jsonListener.getAssociationTable();

        if( !table.empty() ) {
            for( auto itr = table.cbegin(); itr != table.cend(); ++itr ) {
                znet::Association association = {};
                const auto tableVal = common::iValueCast<common::TableValue>( itr->second.get() );

                if( !tableVal->isKeyExist( "channel_id" ) ) {
                    FUNC_LOGW( "Could not get field channel_id" );
                    continue;
                }

                association.channelId = ( common::iValueCast<common::IntValue>( ( *tableVal )["channel_id"] ) )->get();

                if( !tableVal->isKeyExist( "group_id" ) ) {
                    FUNC_LOGW( "Could not get field group_id" );
                    continue;
                }
                association.groupId = ( common::iValueCast<common::IntValue>( ( *tableVal )["group_id"] ) )->get();

                if( tableVal->isKeyExist( "target_channel_id" ) ) {
                    association.targetChannelId =
                        ( common::iValueCast<common::IntValue>( ( *tableVal )["target_channel_id"] ) )->get();
                }

                callback( association );
            }
        }
    } );
}

znet::utils::optional<bool> NodeSetting::shouldRequestNotifications() const {
    znet::utils::optional<bool> result = {};

    getFieldValue( [&result]( jssp::JsonListenerDeviceSetting& jsonListener ) {
        const auto& table = jsonListener.getParameterTable();
        if( !table.empty() ) {
            for( auto itr = table.cbegin(); itr != table.cend(); ++itr ) {
                const auto boolVal = common::iValueCast<common::BoolValue>( itr->second.get() );
                if( !boolVal ) {
                    continue;
                }

                if( itr->first != "should_request_notifications" ) {
                    continue;
                }

                result = static_cast<bool>( boolVal->get() );
            }
        }
    } );

    return result;

    //    auto it = m_setting.find( "should_request_notifications" );
    //    return it == m_setting.end() ? znet::utils::optional< bool >() : it->get< bool >();
}

znet::utils::optional<bool> NodeSetting::isExtendedUserCodesRequestRestricted() const {
    znet::utils::optional<bool> result = {};
    /*
    auto supportedUserCodesIt = m_setting.find( "supported_user_codes" );

    if( supportedUserCodesIt == m_setting.end() || !supportedUserCodesIt->is_object() ) {
        return {};
    }

    auto it = supportedUserCodesIt->find( "extended_user_codes_request_restricted" );

    return it == supportedUserCodesIt->end() ? znet::utils::optional< bool >() : it->get< bool >();
    */

    return result;
}

znet::utils::optional<uint32_t> NodeSetting::getWakeUpInterval() const {
    znet::utils::optional<uint32_t> interval;

    getFieldValue( [&interval]( jssp::JsonListenerDeviceSetting& jsonListener ) {
        const auto& table = jsonListener.getParameterTable();
        if( !table.empty() ) {
            for( auto itr = table.cbegin(); itr != table.cend(); ++itr ) {
                const auto intVal = common::iValueCast<common::IntValue>( itr->second.get() );
                if( !intVal ) {
                    continue;
                }

                if( itr->first != "wake_up_interval" ) {
                    continue;
                }

                interval = static_cast<int>( intVal->get() );
            }
        }
    } );

    return interval;
}

}  // namespace zwave
}  // namespace gateways
