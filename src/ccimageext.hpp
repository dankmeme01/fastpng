#pragma once
#include <cocos2d.h>
#include <Geode/utils/Result.hpp>

#include <formats.hpp>

namespace fastpng {

class CCImageExt : public cocos2d::CCImage {
public:
    uint8_t* getImageData();

    void setImageData(uint8_t* buf);

    void setImageProperties(uint32_t width, uint32_t height, uint32_t bitDepth, bool hasAlpha, bool hasPreMulti);

    geode::Result<> initWithFormat(void* data, size_t size, const ImageFormat& format);

    // Attempts to use the FPNG decoder, if that is impossible, falls back to SPNG.
    // Pass in the data without the prepended magic.
    geode::Result<> initWithFastest(void* data, size_t size);
};

}