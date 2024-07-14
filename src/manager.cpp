#include "manager.hpp"

#include <Geode/loader/SettingEvent.hpp>
#include <asp/data.hpp>

#include <formats.hpp>
#include <crc32.hpp>
#include <fpng.h>

using namespace geode::prelude;

$execute {
    auto dir = Mod::get()->getSaveDir() / "cached-images";
    (void) file::createDirectoryAll(dir);
}

FastpngManager::FastpngManager() {
    converterThread.setStartFunction([] {
        utils::thread::setName("PNG Converter");
    });

    converterThread.setExceptionFunction([](const auto& exc) {
        log::error("converter thread failed: {}", exc.what());

        Loader::get()->queueInMainThread([msg = std::string(exc.what())] {
            FLAlertLayer::create("FastPNG error", msg, "Ok")->show();
        });
    });

    converterThread.setLoopFunction(&FastpngManager::threadFunc);
    converterThread.start(this);

    this->storeAsRaw = Mod::get()->getSettingValue<bool>("cache-as-raw");
    listenForSettingChanges("cache-as-raw", +[](bool r) {
        FastpngManager::get().storeAsRaw = r;
    });
}

std::filesystem::path FastpngManager::getCacheDir() {
    return Mod::get()->getSaveDir() / "cached-images";
}

std::filesystem::path FastpngManager::cacheFileForChecksum(uint32_t checksum) {
    return this->getCacheDir() / std::to_string(checksum);
}

bool FastpngManager::storeRawImages() {
    return storeAsRaw;
}

bool FastpngManager::reallocFromCache(uint32_t checksum, uint8_t*& outbuf, size_t& outsize) {
    auto filep = this->cacheFileForChecksum(checksum);

    std::basic_ifstream<uint8_t> file(filep, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    std::error_code ec;
    auto fsize = std::filesystem::file_size(filep, ec);

    if (ec != std::error_code{}) {
        auto fsize = file.tellg();
        file.seekg(0, std::ios::end);
        fsize = file.tellg() - fsize;
        file.seekg(0, std::ios::beg);
    }

    outsize = fsize;

    outbuf = reinterpret_cast<uint8_t*>(std::malloc(fsize));

    file.read(outbuf, fsize);

    file.close();

    return true;
}

void FastpngManager::queueForCache(const std::filesystem::path& path, std::vector<uint8_t>&& data) {
    converterQueue.push(std::make_pair(path, std::move(data)));
}

void FastpngManager::threadFunc() {
    auto tasko = converterQueue.popTimeout(std::chrono::seconds(1));

    if (!tasko) return;

    auto& task = tasko.value();
    auto& path = task.first;

    auto data = std::move(task.second);

    // if in low ram mode, no data has been passed to read it ourselves
    if (data.empty()) {
        std::basic_ifstream<uint8_t> stream(path, std::ios::binary | std::ios::in);
        if (stream.is_open()) {
            data = std::vector((std::istreambuf_iterator<uint8_t>(stream)),
                                        std::istreambuf_iterator<uint8_t>());

            stream.close();
        }
    }

    if (data.empty()) {
        log::warn("Failed to convert image (couldn't open file): {}", path);
        return;
    }

    // compute the checksum for saving the file
    auto checksum = fastpng::crc32(data.data(), data.size());

    // decode with spng, re-encode with fpng/raw
    auto res = fastpng::FORMATS.spng.decode(data.data(), data.size());
    if (!res) {
        log::warn("Failed to convert image (decode error: {}): {}", res.unwrapErr(), path);
        return;
    }

    auto result = std::move(res.unwrap());

    const auto& fmt = storeAsRaw ? fastpng::FORMATS.raw : fastpng::FORMATS.fpng;

    auto encodedres = fmt.encode(result.rawData.get(), result.rawSize, result.width, result.height);

    if (!encodedres) {
        log::warn("Failed to convert image (encode error: {}): {}", encodedres.unwrapErr(), path);
        return;
    }

    auto encoded = std::move(encodedres.unwrap());
    auto outPath = this->getCacheDir() / std::to_string(checksum);

    std::basic_ofstream<uint8_t> file(outPath, std::ios::out | std::ios::binary);

    auto magic = asp::data::byteswap(fmt.magic);
    file.write(reinterpret_cast<uint8_t*>(&magic), sizeof(magic));
    file.write(encoded.data(), encoded.size());
    file.close();

    // log::info("Converted and saved {} as cached image {}", path, checksum);
}
