#pragma once

namespace scenes {

class ThenBlockSetItemValue;
class ThenBlockSetDeviceArmed;
class ThenBlockSendHttpRequest;
#ifdef CONFIG_ADVANCED_SCENES
class ThenBlockSwitchHouseMode;
class ThenBlockExecuteScene;
#endif  // CONFIG_ADVANCED_SCENES
class ThenBlockCloudCommand;
class ThenBlockSetValueMulticast;
class ThenBlockSetValueMulticast2;
class ThenBlockSetValueMulticast3;

struct ThenBlockConstVisitor {
    virtual ~ThenBlockConstVisitor() = default;

    virtual void visit( const ThenBlockSetItemValue& block ){};

    virtual void visit( const ThenBlockSetValueMulticast& block ){};

    virtual void visit( const ThenBlockSetValueMulticast2& block ){};

    virtual void visit( const ThenBlockSetValueMulticast3& block ){};

    virtual void visit( const ThenBlockSetDeviceArmed& block ){};

    virtual void visit( const ThenBlockSendHttpRequest& block ){};

#ifdef CONFIG_ADVANCED_SCENES
    virtual void visit( const ThenBlockSwitchHouseMode& block ){};

    virtual void visit( const ThenBlockExecuteScene& block ){};
#endif  // CONFIG_ADVANCED_SCENES

    virtual void visit( const ThenBlockCloudCommand& block ){};
};

struct ThenBlockVisitor {
    virtual ~ThenBlockVisitor() = default;

    virtual void visit( ThenBlockSetItemValue& block ){};

    virtual void visit( ThenBlockSetValueMulticast& block ){};

    virtual void visit( ThenBlockSetValueMulticast2& block ){};

    virtual void visit( ThenBlockSetValueMulticast3& block ){};

    virtual void visit( ThenBlockSetDeviceArmed& block ){};

    virtual void visit( ThenBlockSendHttpRequest& block ){};

#ifdef CONFIG_ADVANCED_SCENES
    virtual void visit( ThenBlockSwitchHouseMode& block ){};

    virtual void visit( ThenBlockExecuteScene& block ){};
#endif  // CONFIG_ADVANCED_SCENES

    virtual void visit( ThenBlockCloudCommand& block ){};
};

}  // end of namespace scenes
