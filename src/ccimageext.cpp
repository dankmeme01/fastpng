#include "ccimageext.hpp"

#include <asp/data.hpp>

#include <alpha.hpp>
#include <fpng.h>

using namespace geode::prelude;

namespace fastpng {

uint8_t* CCImageExt::getImageData() {
    return m_pData;
}

void CCImageExt::setImageData(uint8_t* buf) {
    delete[] m_pData;
    m_pData = buf;
}

void CCImageExt::setImageProperties(uint32_t width, uint32_t height, uint32_t bitDepth, bool hasAlpha, bool hasPreMulti) {
    this->m_nWidth = width;
    this->m_nHeight = height;
    this->m_nBitsPerComponent = bitDepth;
    this->m_bHasAlpha = hasAlpha;
    this->m_bPreMulti = hasPreMulti;
}

Result<> CCImageExt::initWithFormat(void* data, size_t size, const ImageFormat& format) {
    auto result = format.decode(static_cast<uint8_t*>(data), size);
    if (!result) {
        return Err(std::move(result.unwrapErr()));
    }

    auto img = std::move(result.unwrap());

    m_nWidth = img.width;
    m_nHeight = img.height;
    m_bHasAlpha = img.channels == 4;
    m_bPreMulti = m_bHasAlpha;
    m_pData = img.rawData.release();

    // if the image is transparent and not premultiplied, premultiply it
    if (m_bHasAlpha && !img.premultiplied) {
        premultiplyAlpha(m_pData, m_pData, img.rawSize);
    }

    return Ok();
}

Result<> CCImageExt::initWithFastest(void* data, size_t size) {
    uint32_t width, height, channels;
    int code = fpng::fpng_get_info(data, size, width, height, channels);

    if (code == fpng::FPNG_DECODE_NOT_FPNG) {
        return this->initWithFormat(data, size, fastpng::FORMATS.spng);
    }

    auto res = this->initWithFormat(data, size, fastpng::FORMATS.fpng);

    // try spng as last effort
    return res ? res : this->initWithFormat(data, size, fastpng::FORMATS.spng);
}

}