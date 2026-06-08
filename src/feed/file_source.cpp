#include "feed/file_source.h"

#include "market_data/types.h"

#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

FileSource::FileSource(std::string_view filepath) {
    int fd = open(filepath.data(), O_RDONLY);
    struct stat sb;
    fstat(fd, &sb);

    size_ = sb.st_size;
    map_ = mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd, 0);
    madvise(map_, size_, MADV_SEQUENTIAL);

    ptr_ = std::span{static_cast<const std::byte*>(map_), size_};

    close(fd);
}

FileSource::~FileSource() {
     munmap(map_, size_);
}

ReadResult FileSource::next() {
    if (ptr_.empty()) return std::unexpected{DataReadFailure::StreamClosed};
    if (ptr_.size() < sizeof(nasdaq::PacketSize)) return std::unexpected{DataReadFailure::InvalidFormat};

    nasdaq::PacketSize packet_size{};
    std::memcpy(&packet_size, ptr_.data(), sizeof(nasdaq::PacketSize));
    ptr_ = ptr_.subspan(sizeof(nasdaq::PacketSize));

    if (ptr_.size() < packet_size) return std::unexpected{DataReadFailure::InvalidFormat};

    std::span data_frame{ptr_.data(), packet_size};
    ptr_ = ptr_.subspan(packet_size);

    return data_frame;
}

