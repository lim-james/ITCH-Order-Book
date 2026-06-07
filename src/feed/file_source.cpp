#include "feed/file_source.h"

explicit FileSource::FileSource(std::string_view filepath) {
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

    const auto packet_size = *reinterpret_cast<nasdaq::PacketSize*>(ptr_.data());
    ptr_ = ptr_.subspan(sizeof(nasdaq::PacketSize));

    if (ptr_.size() < packet_size) return std::nullopt;

    const auto data_frame = DataFrame{std::span{ptr_.data(), packet_size}};
    ptr_ = ptr_.subspan(packet_size);

    return data_frame;
}

