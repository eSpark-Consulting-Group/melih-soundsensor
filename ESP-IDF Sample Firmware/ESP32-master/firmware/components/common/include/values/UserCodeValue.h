
#pragma once

#include <IValue.h>
#include <string>
#include <stdio.h>

namespace common {

class UserCodeValue final : public IValue {
public:
    using UPtr = std::unique_ptr<UserCodeValue>;

    UserCodeValue() = default;

    UserCodeValue( const UserCodeValue& ) = default;

    UserCodeValue( UserCodeValue&& ) = default;

    UserCodeValue( std::string&& name, std::string&& code );

    UserCodeValue( const std::string& name, const std::string& code );

    void accept( IValueVisitorConst& visitor ) const override;

    void accept( IValueVisitor& visitor ) override;

    const std::string& getName() const {
        return m_name;
    }

    const std::string& getCode() const {
        return m_code;
    }

    void setName( const std::string& name ) {
        m_name = name;
    }

    void setCode( const std::string& code ) {
        m_code = code;
    }

private:
    std::string m_name;
    std::string m_code;
};

}  // namespace common
