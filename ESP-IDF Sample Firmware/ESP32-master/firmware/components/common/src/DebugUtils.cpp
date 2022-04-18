#include <stdio.h>
#include <stdint.h>
#include <vector>

#include <esp_log.h>

namespace common {
namespace debug {

void mlogUint32( const char* file, const char* func, int line, const char* name, uint32_t value ) {
    ESP_LOGD( func,
              "%s:%s:%d %s=%02x:%02x:%02x:%02x",
              file,
              func,
              line,
              name,
              ( value & 0xFF000000 ) >> 24,
              ( value & 0x00FF0000 ) >> 16,
              ( value & 0x0000FF00 ) >> 8,
              value & 0x000000FF );
}

void mlogUint8( const char* file, const char* func, int line, const char* name, uint8_t value ) {
    ESP_LOGD( func, "%s:%s:%d %s=%02x", file, func, line, name, value );
}

void mlogUint8Array( const char* file, const char* func, int line, const char* name, uint8_t* value, uint32_t count ) {
    if( 0 == count ) {
        ESP_LOGD( func, "%s:%s:%d %s[0]=", file, func, line, name );
        return;
    }

    std::vector<char> buffer( count * 3, 0 );
    size_t charsWritten = 0, numbersLeft = count;
    while( numbersLeft-- ) {
        charsWritten += ::snprintf( buffer.data() + charsWritten, buffer.size() - charsWritten, "%02x:", *value++ );
    }
    buffer.back() = 0;
    ESP_LOGD( func, "%s:%s:%d %s[%d]=%s", file, func, line, name, count, buffer.data() );
}

}  // namespace debug
}  // namespace common
