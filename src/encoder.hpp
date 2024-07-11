#pragma once

#include <Geode/utils/Result.hpp>

#include <stddef.h>
#include <stdint.h>

namespace fastpng {

geode::Result<std::vector<uint8_t>> encodePNG(const uint8_t* data, size_t size, uint32_t width, uint32_t height, bool slower = false);

}