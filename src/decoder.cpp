#include "decoder.hpp"

#include "alpha.hpp"

#include <fpng.h>
#include <spng.h>

using namespace geode::prelude;

namespace fastpng {

#define SPNG_EC(fun) if (auto _ec = (fun) != 0) { \
    spng_ctx_free(ctx); \
	return Err(fmt::format("{} failed: {}", #fun, spng_strerror(_ec))); \
}

Result<SPNGResult> decodePNG(void* data, size_t size) {
    spng_ctx* ctx = spng_ctx_new(0);

    if (!ctx) {
        return Err("failed to allocate spng context");
    }

    SPNG_EC(spng_set_png_buffer(ctx, data, size));

    spng_ihdr hdr;
    SPNG_EC(spng_get_ihdr(ctx, &hdr));

    size_t imgSize;
    SPNG_EC(spng_decoded_image_size(ctx, SPNG_FMT_RGBA8, &imgSize));

    uint8_t* rawData = new uint8_t[imgSize];

    if (auto code = spng_decode_image(ctx, rawData, imgSize, SPNG_FMT_RGBA8, SPNG_DECODE_TRNS) != 0) {
        delete[] rawData;
        spng_ctx_free(ctx);
        return Err(fmt::format("spng_decode_image failed: {}", spng_strerror(code)));
    }

    spng_ctx_free(ctx);

    return Ok(SPNGResult {
        .width = hdr.width,
        .height = hdr.height,
        .bitDepth = hdr.bit_depth,
        .colorType = hdr.color_type,
        .data = rawData,
        .dataSize = imgSize
    });
}

Result<FPNGResult> decodeFPNG(void* data, size_t size) {
    uint32_t width, height, channels;

    std::vector<uint8_t> decoded;
    if (auto code = fpng::fpng_decode_memory(data, size, decoded, width, height, channels, 4)) {
        return Err(fmt::format("fpng_decode_memory failed: code {}", code));
    }

    return Ok(FPNGResult {
        .width = width,
        .height = height,
        .channels = channels,
        .data = std::move(decoded),
    });
}

void CCImageExt::setImageProperties(uint32_t width, uint32_t height, uint32_t bitDepth, bool hasAlpha, bool hasPreMulti) {
    this->m_nWidth = width;
    this->m_nHeight = height;
    this->m_nBitsPerComponent = bitDepth;
    this->m_bHasAlpha = hasAlpha;
    this->m_bPreMulti = hasPreMulti;
}

Result<> CCImageExt::initWithSPNG(void* data, size_t size) {
    auto res = decodePNG(data, size);
    if (!res) {
        return Err(std::move(res.unwrapErr()));
    }

    auto decoded = std::move(res.unwrap());

    size_t channels = decoded.dataSize / (decoded.width * decoded.height);

    bool hasAlpha = channels == 4;

    if (hasAlpha) {
        premultiplyAlpha(decoded.data, decoded.data, decoded.dataSize);
    }

    this->setImageData(decoded.data);
    this->setImageProperties(decoded.width, decoded.height, decoded.bitDepth, hasAlpha, hasAlpha);

    return Ok();
}

inline Result<> CCImageExt::_initWithFPNGInternal(void* data, size_t size) {
    auto res = decodeFPNG(data, size);
    if (!res) {
        return Err(std::move(res.unwrapErr()));
    }

    auto decoded = std::move(res.unwrap());

    uint8_t* dataCopy = new uint8_t[decoded.data.size()];

    bool hasAlpha = decoded.channels == 4;
    if (hasAlpha) {
        premultiplyAlpha(dataCopy, decoded.data.data(), decoded.data.size());
    } else {
        std::memcpy(dataCopy, decoded.data.data(), decoded.data.size());
    }

    this->setImageData(dataCopy);
    this->setImageProperties(decoded.width, decoded.height, 8, hasAlpha, hasAlpha);

    return Ok();
}

Result<> CCImageExt::initWithFPNG(void* data, size_t size) {
    return this->_initWithFPNGInternal(data, size);
}

Result<> CCImageExt::initWithFastest(void* data, size_t size) {
    uint32_t width, height, channels;
    int code = fpng::fpng_get_info(data, size, width, height, channels);

    if (code == fpng::FPNG_DECODE_NOT_FPNG) {
        return this->initWithSPNG(data, size);
    }

    auto res = this->_initWithFPNGInternal(data, size);

    // try spng as last effort
    return res ? res : this->initWithSPNG(data, size);
}

}