#pragma once
#include <boost/container/vector.hpp>
#include <Geode/utils/Result.hpp>

namespace fastpng {

struct DecodedImage {
    std::unique_ptr<uint8_t[]> rawData;
    size_t rawSize;

    uint32_t width;
    uint32_t height;
    uint8_t bitDepth;
    uint8_t channels;
    bool premultiplied;
};

struct ImageFormat {
    using EncodeFn = geode::Result<boost::container::vector<uint8_t>>(const uint8_t* data, size_t size, uint32_t width, uint32_t height);
    using DecodeFn = geode::Result<DecodedImage>(const uint8_t* data, size_t size);

    EncodeFn* encode;
    DecodeFn* decode;
    uint32_t magic;
};

geode::Result<boost::container::vector<uint8_t>> encodeSPNG(const uint8_t* data, size_t size, uint32_t width, uint32_t height);
geode::Result<boost::container::vector<uint8_t>> encodeFPNG(const uint8_t* data, size_t size, uint32_t width, uint32_t height);
geode::Result<boost::container::vector<uint8_t>> encodeRaw(const uint8_t* data, size_t size, uint32_t width, uint32_t height);

geode::Result<DecodedImage> decodeSPNG(const uint8_t* data, size_t size);
geode::Result<DecodedImage> decodeFPNG(const uint8_t* data, size_t size);
geode::Result<DecodedImage> decodeRaw(const uint8_t* data, size_t size);

struct _Formats {
    ImageFormat fpng;
    ImageFormat spng;
    ImageFormat raw;
};

constexpr auto FORMATS = _Formats {
    .fpng = ImageFormat {
        .encode = &encodeFPNG,
        .decode = &decodeFPNG,
        .magic = 0x19f4ba42,
    },
    .spng = ImageFormat {
        .encode = &encodeSPNG,
        .decode = &decodeSPNG,
        .magic = 0xf294fa94,
    },
    .raw = ImageFormat {
        .encode = &encodeRaw,
        .decode = &decodeRaw,
        .magic = 0xb1a94fd3
    }
};

}