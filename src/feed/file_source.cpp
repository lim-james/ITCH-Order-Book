#include "feed/file_source.h"

#include "market_data/types.h"

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

std::optional<DataFrame> FileSource::next() {
    if (ptr_.size() < sizeof(nasdaq::PacketSize)) return std::nullopt;

    const auto packet_size = *reinterpret_cast<const nasdaq::PacketSize*>(ptr_.data());
    ptr_ = ptr_.subspan(sizeof(nasdaq::PacketSize));

    if (ptr_.size() < packet_size) return std::nullopt;

    std::span data_frame{ptr_.data(), packet_size};
    ptr_ = ptr_.subspan(packet_size);

    return data_frame;
}

