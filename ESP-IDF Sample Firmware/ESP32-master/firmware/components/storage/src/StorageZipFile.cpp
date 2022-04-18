#include <StorageZipFile.h>
#include <DebugUtils.h>
#include <cstring>

namespace storage {

ZipFile::ZipFile() {
    clear();
}

ZipFile::~ZipFile() {
    close();
}

ZipFile::ZipFile( ZipFile&& other )
: m_zip_archive( other.m_zip_archive )
, m_file_iterator( other.m_file_iterator ) {
    other.clear();
}

ZipFile& ZipFile::operator=( ZipFile&& other ) {
    m_zip_archive = other.m_zip_archive;
    m_file_iterator = other.m_file_iterator;
    other.clear();
    return *this;
}

bool ZipFile::open( const char* zip_archive, const char* filename ) {
    close();
    mz_uint32 file_index;
    if( !mz_zip_reader_init_file( &m_zip_archive, zip_archive, 0 ) )
        goto error;
    if( !mz_zip_reader_locate_file_v2( &m_zip_archive, filename, nullptr, 0, &file_index ) )
        goto error_zip_uninit;
    if( !mz_zip_validate_file( &m_zip_archive, file_index, 0 ) )
        goto error_zip_uninit;
    m_file_iterator = mz_zip_reader_extract_iter_new( &m_zip_archive, file_index, 0 );
    if( !m_file_iterator )
        goto error_zip_uninit;

    return true;

error_zip_uninit:
    mz_zip_reader_end( &m_zip_archive );
error:
    clear();
    return false;
}

bool ZipFile::read( void* buff, size_t buff_size, size_t* output_size ) {
    *output_size = mz_zip_reader_extract_iter_read( m_file_iterator, buff, buff_size );
    return *output_size > 0;
}

void ZipFile::close() {
    if( m_file_iterator ) {
        mz_zip_reader_extract_iter_free( m_file_iterator );
        mz_zip_reader_end( &m_zip_archive );
        clear();
    }
}

size_t ZipFile::size() {
    FUNC_LOGE( "ZipFile the method size()  has not been implemented" );
    return 0;
}

void ZipFile::clear() {
    m_file_iterator = nullptr;
    std::memset( &m_zip_archive, 0, sizeof( m_zip_archive ) );
}

}  // namespace storage
