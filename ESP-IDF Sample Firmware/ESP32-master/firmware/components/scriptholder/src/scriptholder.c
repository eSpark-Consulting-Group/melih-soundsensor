#include <stddef.h>
#include <stdint.h>
#include "scriptholder.h"

#define LUASCRIPT( _name, _major, _minor, _extension, _file ) \
    extern const uint8_t _binary_##_file##_start;             \
    extern const uint8_t _binary_##_file##_end;

#include "scriptslist.h"

#undef LUASCRIPT

#define LUASCRIPT( _name, _major, _minor, _extension, _file ) \
    {                                                         \
        .name = _name,                                        \
        .major = _major,                                      \
        .minor = _minor,                                      \
        .extension = _extension,                              \
        .begin = &_binary_##_file##_start,                    \
        .end = &_binary_##_file##_end,                        \
    },

static const objectDesc_t luaScriptsList[] = {
#include "scriptslist.h"
};

#undef LUASCRIPT

static const size_t luaScriptsListLen = sizeof( luaScriptsList ) / sizeof( luaScriptsList[0] );

const objectDesc_t* getScritpDesc( size_t index ) {
    if( index > ( luaScriptsListLen - 1 ) ) {
        return NULL;
    }
    return &luaScriptsList[index];
}
