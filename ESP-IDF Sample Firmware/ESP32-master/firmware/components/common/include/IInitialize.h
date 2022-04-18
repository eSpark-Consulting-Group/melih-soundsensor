#pragma once

namespace common {

class IInitialize {
public:
    virtual ~IInitialize() = default;

    inline bool isInitialized() const {
        return m_isInitialized;
    }

    inline bool initialize() {
        if( isInitialized() ) {
            return true;
        }

        m_isInitialized = initializeImpl();
        return m_isInitialized;
    }

protected:
    virtual bool initializeImpl() = 0;

private:
    bool m_isInitialized = false;
};

}  // namespace common
