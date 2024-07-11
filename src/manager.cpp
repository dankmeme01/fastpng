#include "manager.hpp"

#include <decoder.hpp>
#include <encoder.hpp>
#include <crc32.hpp>
#include <fpng.h>

using namespace geode::prelude;

$execute {
    auto dir = Mod::get()->getSaveDir() / "cached-images";
    (void) file::createDirectoryAll(dir);
}

FastpngManager::FastpngManager() {
    fpng::fpng_init();

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
}

std::filesystem::path FastpngManager::getCacheDir() {
    return Mod::get()->getSaveDir() / "cached-images";
}

bool FastpngManager::reallocFromCache(unsigned char*& buf, size_t& size, uint32_t checksum) {
    auto filep = this->getCacheDir() / std::to_string(checksum);

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

    if (fsize > size) {
        // rellocate
        delete[] buf;
        buf = reinterpret_cast<unsigned char*>(std::malloc(fsize));
    }

    file.read(buf, fsize);
    size = fsize;

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

    // decode with spng, re-encode with fpng
    auto res = fastpng::decodePNG(data.data(), data.size());
    if (!res) {
        log::warn("Failed to convert image (decode error: {}): {}", res.unwrapErr(), path);
        return;
    }

    auto result = std::move(res.unwrap());

    auto encodedres = fastpng::encodePNG(result.data, result.dataSize, result.width, result.height);
    if (!res) {
        log::warn("Failed to convert image (encode error: {}): {}", res.unwrapErr(), path);
        return;
    }

    auto encoded = std::move(encodedres.unwrap());
    auto outPath = this->getCacheDir() / std::to_string(checksum);

    std::basic_ofstream<uint8_t> file(outPath, std::ios::out | std::ios::binary);
    file.write(encoded.data(), encoded.size());
    file.close();

    // log::info("Converted and saved {} as cached image {}", path, checksum);
}
