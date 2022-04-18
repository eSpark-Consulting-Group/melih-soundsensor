#include <znet/znet_io_wrapper.hpp>
#include <memory>
#include <fstream>

namespace cloud_storage {

class FileSource : public znet::IOWrapper::Source {
public:
    FileSource( std::shared_ptr<std::ifstream> stream );

    size_t read( size_t size, uint8_t* dst ) override;

    void seek( size_t pos ) override;

private:
    std::shared_ptr<std::ifstream> m_stream;
};

class FileDestination : public znet::IOWrapper::Destination {
public:
    FileDestination( std::shared_ptr<std::ofstream> stream );

    void write( size_t size, const uint8_t* src ) override;

private:
    std::shared_ptr<std::ofstream> m_stream;
};

}  // namespace cloud_storage