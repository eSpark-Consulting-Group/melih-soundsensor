#pragma once

#include "values/BoolValue.h"
#include "values/IntValue.h"
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

class ScalesConverterBaseVisitor : public ::common::IValueVisitor {
public:
    explicit ScalesConverterBaseVisitor( std::string _dstScale );

    void visit( common::FloatValue& value ) override{};
    void visit( common::ScalebleValue& value ) override;
    void visit( common::IntValue& value ) override{};
    void visit( common::Int64Value& value ) override{};
    void visit( common::BoolValue& value ) override{};
    void visit( common::StringValue& value ) override{};
    void visit( common::TokenValue& value ) override{};
    void visit( common::RgbValue& value ) override{};
    void visit( common::NotificationValue& value ) override{};
    void visit( common::ArrayValue& value ) override{};
    void visit( common::TableValue& value ) override{};
    void visit( common::UserCodeValue& value ) override{};

protected:
    std::string dstScale;
    common::IValue::UPtr dstScaleValuePtr;
};

class ScalesConverterVisitor : public ScalesConverterBaseVisitor {
public:
    static const uint8_t Precision;  // TODO  - temporary. Use the Hub's configuration value subsequently

    explicit ScalesConverterVisitor( std::string _dstScale, uint8_t _precision );

    void visit( common::ScalebleValue& value ) override;

protected:
    uint8_t precision;
};

class ScalesConverterFromFloatVisitor final : public ScalesConverterVisitor {
public:
    explicit ScalesConverterFromFloatVisitor( std::string _srcScale,
                                              std::string _dstScale,
                                              uint8_t _precision,
                                              float _srcValue );

    void visit( common::ScalebleValue& value ) override;

private:
    std::string m_srcScale;
    float m_srcValue;
};

}  // namespace common
