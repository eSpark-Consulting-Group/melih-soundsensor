#include "IWriteStream.h"

namespace common {

int VectorWStream::write( const char* data, std::size_t dataSize ) {
    const auto oldVecSize = m_buf.size();
    m_buf.insert( std::end( m_buf ), data, data + dataSize );
    return m_buf.size() - oldVecSize;
}

}  // namespace common