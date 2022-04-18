#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char* name;
    uint8_t major;
    uint8_t minor;
    const char* extension;
    const uint8_t* begin;
    const uint8_t* end;
} objectDesc_t;

const objectDesc_t* getScritpDesc( size_t index );

#ifdef __cplusplus
}
#endif
