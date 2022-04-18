#include <SettingsInfo.h>
#include <DeviceInfo.h>
#include <IValueCast.h>
#include <HouseModesManager.h>
#include <CommonResources.h>
#include <Cache.h>
#include <StorageFile.h>
#include <Storage.h>
#include <BinaryFileStorageFactory.h>
#include <StorageSection.h>
#include <DeviceInfo.h>
#include <ObjectSerializer.h>
#include "HubSettings.h"

#include "values/Int64Value.hpp"

#include <cmath>

#include <DebugUtils.h>

namespace gateways {

using namespace common::resources;

void SettingsInfo::setEnumValues( common::TableValue*& table_ptr ) {
    for( auto it = table_ptr->begin(); it != table_ptr->end(); ++it ) {
        auto key1level = it->first;
        if( key1level.empty() ) {
            continue;
        }

        auto arr = common::iValueCast<common::TableValue>( it->second.get() );
        if( !arr ) {
            continue;
        }

        for( auto itAr = arr->begin(); itAr != arr->end(); ++itAr ) {
            auto key2level = itAr->first;

            if( key2level.empty() ) {
                continue;
            }

            auto sec = common::iValueCast<common::StringValue>( itAr->second.get() );
            if( !sec ) {
                continue;
            }

            auto value = sec->get();

            if( value.empty() ) {
                continue;
            }

            EnumType enum_var;
            enum_var.first = key1level;
            enum_var.second.lang_tag = key2level;
            enum_var.second.text = value;
            m_enums.emplace_back( enum_var );
        }
    }
}

bool SettingsInfo::checkEnumValue( const StringType& value ) const {
    for( const auto& enumVal : m_enums ) {
        if( enumVal.first == value ) {
            return true;
        }
    }
    return false;
}

common::IValue::UPtr SettingsInfo::castToElementType( common::IValue::UPtr&& ival ) const {
    return m_elementType ? BaseInfo::castToType( std::move( ival ), *m_elementType ) : nullptr;
}

}  // namespace gateways
