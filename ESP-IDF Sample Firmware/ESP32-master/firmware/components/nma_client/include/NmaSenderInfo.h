
#pragma once

#include <string>

namespace nma {

class SenderInfo final {
public:
    void setId( const char* id );

    void setType( const char* type );

    const char* getId() const {
        return m_id.c_str();
    }

    const char* getType() const {
        return m_type.c_str();
    }

    bool isComplete() const {
        return true;
    }

    void reset() {
        m_id.clear();
        m_type.clear();
    }

private:
    std::string m_id;
    std::string m_type;
};

}  // namespace nma
