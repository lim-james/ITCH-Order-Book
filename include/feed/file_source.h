#pragma once

#include "data_source.h"

#include <string_view>
#include <fstream>
#include <span>
#include <cstddef>

class FileSource {

public:

    explicit FileSource(std::string_view filepath);
    ~FileSource();
    std::optional<DataFrame> next();

private:

    void* map_;
    std::size_t size_;

    std::span<const std::byte> ptr_;

};

static_assert(data_source<FileSource>);
