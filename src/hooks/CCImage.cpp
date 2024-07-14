#include <Geode/modify/CCImage.hpp>
#include <Geode/Geode.hpp>

#include <asp/data.hpp>

#include <manager.hpp>
#include <formats.hpp>
#include <ccimageext.hpp>
#include <crc32.hpp>

using namespace geode::prelude;
using fastpng::CCImageExt;

static bool lowMemoryMode = false;

$execute {
    lowMemoryMode = Mod::get()->getSettingValue<bool>("low-memory-mode");
}

#ifndef FASTPNG_BENCH
class $modify(CCImage) {
    CCImageExt* ext() {
        return static_cast<CCImageExt*>(static_cast<CCImage*>(this));
    }

    static void onModify(auto& self) {
        // run last since we dont call the originals
        (void) self.setHookPriority("cocos2d::CCImage::initWithImageFile", 999999).unwrap();
        (void) self.setHookPriority("cocos2d::CCImage::initWithImageFileThreadSafe", 999999).unwrap();
        (void) self.setHookPriority("cocos2d::CCImage::_initWithPngData", 999999).unwrap();
    }

    bool initWithImageFileCommon(uint8_t* buffer, size_t size, EImageFormat format, const char* path) {
        // if not png, just do the default impl of this func
        if (format != CCImage::EImageFormat::kFmtPng && !std::string_view(path).ends_with(".png")) {
            bool ret = CCImage::initWithImageData(buffer, size);
            delete[] buffer;
            return ret;
        }

        // check if we have cached it already
        auto checksum = fastpng::crc32(buffer, size);
        uint8_t* cachedBuf = nullptr;
        size_t cachedSize = 0;

        if (!FastpngManager::get().reallocFromCache(checksum, cachedBuf, cachedSize)) {
            // queue the image to be cached..
            std::vector<uint8_t> data;
            if (!lowMemoryMode) {
                data = std::vector(buffer, buffer + size);
            }

            FastpngManager::get().queueForCache(std::filesystem::path(CCFileUtils::get()->fullPathForFilename(path, false)), std::move(data));
            bool result = this->_initWithPngData(buffer, size);
            delete[] buffer;
            return result;
        }

        const auto& usedFormat = FastpngManager::get().storeRawImages() ? fastpng::FORMATS.raw : fastpng::FORMATS.fpng;

        auto magic = asp::data::byteswap(*reinterpret_cast<uint32_t*>(cachedBuf));

        std::string error;
        if (magic == usedFormat.magic) {
            auto result = this->ext()->initWithFormat(cachedBuf + sizeof(uint32_t), cachedSize - sizeof(uint32_t), usedFormat);
            if (result) {
                delete[] buffer;
                delete[] cachedBuf;
                return true;
            }

            error = std::move(result.unwrapErr());
        } else {
            error = "magic mismatch";
        }

        delete[] cachedBuf;

        std::filesystem::path p = CCFileUtils::get()->fullPathForFilename(path, false);
        auto cachedFile = FastpngManager::get().cacheFileForChecksum(checksum);

        log::warn("Failed to load cached image: {}", error);
        log::warn("Real: {}, cached: {}", p, cachedFile);

        std::error_code ec;
        std::filesystem::remove(cachedFile, ec);

        FastpngManager::get().queueForCache(p, {});

        bool ret = this->_initWithPngData(buffer, size);
        delete[] buffer;

        return ret;
    }

    $override
    bool initWithImageFile(const char* path, EImageFormat format) {
        size_t size = 0;
        unsigned long sizeP;
        // TODO: maybe replace with ifstream
        unsigned char* buffer = CCFileUtils::get()->getFileData(path, "rb", &sizeP);
        size = sizeP;

        if (!buffer || size == 0) {
            delete[] buffer;
            return false;
        }

        return this->initWithImageFileCommon(buffer, size, format, path);
    }

    $override
    bool initWithImageFileThreadSafe(const char* path, EImageFormat imageType) {
        size_t size = 0;
        unsigned long sizeP;
#ifdef GEODE_IS_ANDROID
        CCFileUtilsAndroid* fu = (CCFileUtilsAndroid*)CCFileUtils::get();
        uint8_t* buffer = fu->getFileDataForAsync(path, "rb", &sizeP);
#else
        uint8_t* buffer = CCFileUtils::get()->getFileData(path, "rb", &sizeP);
#endif

        size = sizeP;

        if (!buffer || size == 0) {
            delete[] buffer;
            return false;
        }

        return this->initWithImageFileCommon(buffer, size, imageType, path);
    }

    $override
    bool _initWithPngData(void* data, int size) {
        auto res = this->ext()->initWithFastest(data, size);
        if (!res) {
            log::warn("{}", res.unwrapErr());
            return false;
        }

        return true;
    }
};
#endif
