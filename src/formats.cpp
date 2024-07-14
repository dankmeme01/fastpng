#include "formats.hpp"

#include <spng.h>
#include <fpng.h>
#include <alpha.hpp>
#include <asp/data.hpp>

using namespace geode::prelude;

#define SPNG_EC(fun) if (auto _ec = (fun) != 0) { \
    spng_ctx_free(ctx); \
	return Err(fmt::format("{} failed: {}", #fun, spng_strerror(_ec))); \
}

$execute {
    fpng::fpng_init();
}

namespace fastpng {

Result<boost::container::vector<uint8_t>> encodeSPNG(const uint8_t* data, size_t size, uint32_t width, uint32_t height) {
    return Err("encodeSPNG not implemented");
}

Result<boost::container::vector<uint8_t>> encodeFPNG(const uint8_t* data, size_t size, uint32_t width, uint32_t height) {
    boost::container::vector<uint8_t> out;

    size_t channels = size / (width * height);
    if (!fpng::fpng_encode_image_to_memory(
        data,
        width, height,
        channels,
        out,
        0
    )) {
        return Err(fmt::format("FPNG encode failed! (img data: {}x{}, {} channels, {} bytes)", width, height, channels, size));
    }

    return Ok(std::move(out));
}

Result<boost::container::vector<uint8_t>> encodeRaw(const uint8_t* data, size_t size, uint32_t width, uint32_t height) {
    boost::container::vector<uint8_t> out;

    // encode magic, width and height, then the data
    out.resize(8 + size, boost::container::default_init);

    width = asp::data::byteswap(width);
    height = asp::data::byteswap(height);

    std::memcpy(out.data(), &width, sizeof(width));
    std::memcpy(out.data() + sizeof(width), &height, sizeof(height));

    // we assume source is not premultiplied
    premultiplyAlpha(out.data() + sizeof(width) + sizeof(height), data, size);
    // std::memcpy(out.data() + sizeof(width) + sizeof(height), data, size);

    return Ok(std::move(out));
}

Result<DecodedImage> decodeSPNG(const uint8_t* data, size_t size) {
    DecodedImage image;

    spng_ctx* ctx = spng_ctx_new(0);

    if (!ctx) {
        return Err("failed to allocate spng context");
    }

    SPNG_EC(spng_set_png_buffer(ctx, data, size));

    spng_ihdr hdr;
    SPNG_EC(spng_get_ihdr(ctx, &hdr));

    image.width = hdr.width;
    image.height = hdr.height;
    image.bitDepth = hdr.bit_depth;
    image.premultiplied = false;

    SPNG_EC(spng_decoded_image_size(ctx, SPNG_FMT_RGBA8, &image.rawSize));

    image.channels = image.rawSize / (image.width * image.height);

    image.rawData = std::make_unique<uint8_t[]>(image.rawSize);

    SPNG_EC(spng_decode_image(ctx, image.rawData.get(), image.rawSize, SPNG_FMT_RGBA8, SPNG_DECODE_TRNS));

    spng_ctx_free(ctx);

    return Ok(std::move(image));
}

Result<DecodedImage> decodeFPNG(const uint8_t* data, size_t size) {
    DecodedImage image;

    uint32_t channels;

    uint8_t* rawData;
    size_t rawSize;

    if (auto code = fpng::fpng_decode_memory_ptr(data, size, rawData, rawSize, image.width, image.height, channels, 4)) {
        return Err(fmt::format("fpng_decode_memory failed: code {}", code));
    }

    image.rawData = std::unique_ptr<uint8_t[]>(rawData);
    image.rawSize = rawSize;
    image.channels = channels;
    image.bitDepth = 8;
    image.premultiplied = false;

    return Ok(std::move(image));
}

Result<DecodedImage> decodeRaw(const uint8_t* data, size_t size) {
    DecodedImage image;

    if (size < 8) {
        return Err("RawImageFormat got an image of less than 8 bytes");
    }

    image.width = asp::data::byteswap(*reinterpret_cast<const uint32_t*>(data));
    image.height = asp::data::byteswap(*reinterpret_cast<const uint32_t*>(data + 4));
    image.rawSize = size - 8;
    image.bitDepth = 8;
    image.channels = image.rawSize / (image.width * image.height);
    image.rawData = std::make_unique<uint8_t[]>(image.rawSize);
    image.premultiplied = true;

    std::memcpy(image.rawData.get(), data + 8, image.rawSize);

    return Ok(std::move(image));
}

}