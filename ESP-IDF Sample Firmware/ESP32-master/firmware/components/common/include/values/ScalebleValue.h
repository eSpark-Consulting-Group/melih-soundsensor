
#pragma once

#include <values/FloatValue.h>
#include <string>

namespace common {

class ScalebleValue final : public FloatValue {
public:
    using UPtr = std::unique_ptr<ScalebleValue>;

    ScalebleValue() = default;

    ScalebleValue( const ScalebleValue& ) = default;

    ScalebleValue( FloatType value, bool relative = false );

    ScalebleValue( FloatType value, std::string&& scale, bool relative = false );

    ScalebleValue( FloatType value, const std::string& scale, bool relative = false );

    void accept( IValueVisitorConst& visitor ) const override;

    void accept( IValueVisitor& visitor ) override;

    inline const std::string& getScale() const {
        return m_scale;
    }

    inline void setScale( std::string scale ) {
        m_scale = scale;
    }

    bool getRelative() const {
        return m_isRelative;
    }

    inline void setRelative( const bool isRelative ) {
        m_isRelative = isRelative;
    }

private:
    std::string m_scale;
    bool m_isRelative;
};

}  // namespace common
