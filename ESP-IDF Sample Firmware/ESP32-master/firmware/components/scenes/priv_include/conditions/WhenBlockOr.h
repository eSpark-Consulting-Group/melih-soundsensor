#pragma once

#include "WhenBlockAnd.h"

namespace scenes {

#ifdef CONFIG_ADVANCED_SCENES
class WhenBlockOr final : public WhenBlockAnd {
public:
    using WhenBlockAnd::WhenBlockAnd;

    inline Type type() const override {
        return Type::WhenBlockOr;
    }

    inline void accept( WhenBlockConstVisitor& visitor ) const override {
        visitor.visit( *this );
    }
};
#endif  // CONFIG_ADVANCED_SCENES

}  // end of namespace scenes
