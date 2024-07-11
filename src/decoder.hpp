#pragma once

#include <Geode/utils/Result.hpp>
#include <vector>
#include <stddef.h>
#include <cocos2d.h>

namespace fastpng {

struct SPNGResult {
    uint32_t width;
    uint32_t height;
    uint8_t bitDepth;
    uint8_t colorType;

    uint8_t* data;
    size_t dataSize;
};

// Decodes the image with SPNG. Returns raw data, without premultiplying the alpha channel.
geode::Result<SPNGResult> decodePNG(void* data, size_t size);

struct FPNGResult {
    uint32_t width;
    uint32_t height;
    uint32_t channels;

    std::vector<uint8_t> data;
};

// Decodes the image with FPNG. Returns raw data, without premultiplying the alpha channel.
geode::Result<FPNGResult> decodeFPNG(void* data, size_t size);

class CCImageExt : public cocos2d::CCImage {
public:
    uint8_t* getImageData() {
        return m_pData;
    }

    void setImageData(uint8_t* buf) {
        delete[] m_pData;
        m_pData = buf;
    }

    void setImageProperties(uint32_t width, uint32_t height, uint32_t bitDepth, bool hasAlpha, bool hasPreMulti);

    // Initializes the image using the SPNG decoder.
    geode::Result<> initWithSPNG(void* data, size_t size);

    // Initializes the image using the FPNG decoder.
    geode::Result<> initWithFPNG(void* data, size_t size);

    // Attempts to use the FPNG decoder, if that is impossible, falls back to SPNG.
    geode::Result<> initWithFastest(void* data, size_t size);

private:
    geode::Result<> _initWithFPNGInternal(void* data, size_t size);
};

}