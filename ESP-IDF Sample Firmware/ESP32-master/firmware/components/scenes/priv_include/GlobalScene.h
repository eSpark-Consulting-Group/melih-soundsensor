#pragma once

#include "Scene.h"

namespace scenes {

class GlobalScene : public Scene<GlobalScene> {
public:
    GlobalScene() = default;

    GlobalScene( Uuid uuid,
                 StringType&& name,
                 StringType&& tag,
                 UuidSet&& whenBlocks,
                 UuidList&& thenBlocks,
                 bool isEnabled,
                 ExecPolicy execPolicy );

    GlobalScene( Uuid uuid,
                 const char* name,
                 StringType&& tag,
                 UuidSet&& whenBlocks,
                 UuidList&& thenBlocks,
                 bool isEnabled,
                 ExecPolicy execPolicy );

    inline const auto& tag() const {
        return m_tag;
    }

    inline bool isValidTag() const {
        return !tag().empty();
    }

    bool read( common::IReadStream& stream ) override;

    bool write( common::IWriteStream& stream ) const override;

private:
    StringType m_tag;
};  // class GlobalScene

}  // namespace scenes
