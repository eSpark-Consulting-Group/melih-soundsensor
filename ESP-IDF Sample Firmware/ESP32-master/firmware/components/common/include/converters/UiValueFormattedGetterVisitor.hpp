#pragma once

#include "values/BoolValue.h"
#include "values/IntValue.h"
#include "values/Int64Value.hpp"
#include "values/FloatValue.h"
#include "values/RgbValue.h"
#include "values/ScalebleValue.h"
#include "values/StringValue.h"
#include "values/ArrayValue.h"
#include "values/TableValue.h"
#include "values/UserCodeValue.h"
#include "values/TokenValue.h"
#include "IValueVisitor.h"
#include <DeviceInfo.h>

namespace common {

class UiValueFormattedGetterVisitor : public IValueVisitorConst {
public:
    explicit UiValueFormattedGetterVisitor();
    ~UiValueFormattedGetterVisitor() override;

    const std::string& getValue( const IValue& value );

private:
    void visit( const FloatValue& value ) override;
    void visit( const ScalebleValue& value ) override;
    void visit( const IntValue& value ) override;
    void visit( const Int64Value& value ) override;
    void visit( const BoolValue& value ) override;
    void visit( const StringValue& value ) override;
    void visit( const TokenValue& value ) override;
    void visit( const RgbValue& value ) override;
    void visit( const NotificationValue& value ) override;
    void visit( const UserCodeValue& value ) override;
    void visit( const ArrayValue& value ) override;
    void visit( const TableValue& value ) override;

    std::string rgbToFormattedString( uint8_t red, uint8_t green, uint8_t blue );

private:
    std::string m_result;
};

}  // namespace common
