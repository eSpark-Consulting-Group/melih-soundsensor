#pragma once

#include "values/StringValue.h"

namespace common {

class TokenValue final : public StringValue {
public:
    using UPtr = std::unique_ptr<TokenValue>;
    using StringValue::StringValue;

    void accept( IValueVisitorConst& visitor ) const override;

    void accept( IValueVisitor& visitor ) override;
};

}  // namespace common
