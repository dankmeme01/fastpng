#include "encoder.hpp"

#include <fpng.h>

using namespace geode::prelude;

namespace fastpng {

Result<std::vector<uint8_t>> encodePNG(const uint8_t* data, size_t size, uint32_t width, uint32_t height, bool slower) {
    std::vector<uint8_t> converted;
    size_t channels = size / (width * height);
    if (!fpng::fpng_encode_image_to_memory(
        data,
        width, height,
        channels,
        converted,
        slower ? fpng::FPNG_ENCODE_SLOWER : 0
    )) {
        return Err(fmt::format("FPNG encode failed! (img data: {}x{}, {} channels, {} bytes)", width, height, channels, size));
    }

    return Ok(std::move(converted));
}

}