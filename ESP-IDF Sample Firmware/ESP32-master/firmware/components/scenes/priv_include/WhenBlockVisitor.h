#pragma once

namespace common {
class IReadStream;
class IWriteStream;
}  // namespace common

namespace scenes {

class WhenBlock;
class WhenBlockIsItemState;
class WhenBlockInterval;
class WhenBlockIsDate;
class WhenBlockIsSunState;
class WhenBlockIsOnce;
#ifdef CONFIG_ADVANCED_SCENES
class WhenBlockAnd;
class WhenBlockOr;
class WhenBlockNot;
class WhenBlockIsDateRange;
#endif  // CONFIG_ADVANCED_SCENES
class WhenBlockEqual;
class WhenBlockIsUserLockOperation;
class WhenBlockHasAtLeastOneDictionaryValue;
class WhenBlockIsUnknowDeviceDetected;
class WhenBlockIsDeviceDetected;

struct WhenBlockConstVisitor {
    virtual ~WhenBlockConstVisitor() = default;

    virtual void visit( const WhenBlockIsItemState& block ){};

    virtual void visit( const WhenBlockInterval& block ){};

    virtual void visit( const WhenBlockIsDate& block ){};

    virtual void visit( const WhenBlockIsSunState& block ){};

    virtual void visit( const WhenBlockIsOnce& block ){};

#ifdef CONFIG_ADVANCED_SCENES
    virtual void visit( const WhenBlockAnd& block ){};

    virtual void visit( const WhenBlockOr& block ){};

    virtual void visit( const WhenBlockNot& block ){};

    virtual void visit( const WhenBlockIsDateRange& block ){};
#endif  // CONFIG_ADVANCED_SCENES

    virtual void visit( const WhenBlockEqual& block ){};

    virtual void visit( const WhenBlockIsUserLockOperation& block ){};

    virtual void visit( const WhenBlockHasAtLeastOneDictionaryValue& block ){};

    virtual void visit( const WhenBlockIsUnknowDeviceDetected& block ){};

    virtual void visit( const WhenBlockIsDeviceDetected& block ){};
};

}  // namespace scenes
