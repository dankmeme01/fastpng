#include <Geode/Geode.hpp>
#include "info_popup.hpp"

using namespace geode::prelude;

#include <Geode/modify/MenuLayer.hpp>
class $modify(MyMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) {
            return false;
        }

        // TODO: img
        auto myButton = CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("edit_eEdgeBtn_001.png"),
            this,
            menu_selector(MyMenuLayer::onMyButton)
        );

        auto menu = this->getChildByID("bottom-menu");
        menu->addChild(myButton);

        myButton->setID("my-button"_spr);

        menu->updateLayout();

        return true;
    }

    void onMyButton(CCObject*) {
        FastpngPopup::create()->show();
    }
};

// Benchmarks
#ifdef FASTPNG_BENCH

#include <decoder.hpp>
#include <encoder.hpp>

#include <chrono>
namespace chrono = std::chrono;
using hclock = chrono::high_resolution_clock;
#define BENCH(code) [&]{ \
        auto _start = hclock::now(); \
        code; \
        return hclock::now() - _start; \
    }()

// example: 2.123s, 69.123ms
template <typename Rep, typename Period>
std::string formatDuration(const std::chrono::duration<Rep, Period>& time) {
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(time).count();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(time).count();
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(time).count();

    if (seconds > 0) {
        return std::to_string(seconds) + "." + std::to_string(millis % 1000) + "s";
    } else if (millis > 0) {
        return std::to_string(millis) + "." + std::to_string(micros % 1000) + "ms";
    } else {
        return std::to_string(micros) + "μs";
    }
}

$execute {
    // get a regular png image
    auto path = geode::dirs::getGameDir() / "Resources" / "GJ_GameSheet03-uhd.png";
    std::basic_ifstream<uint8_t> file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        log::error("failed to run benchmark: no gamesheet");
        return;
    }

    std::vector data((std::istreambuf_iterator<uint8_t>(file)),
                                        std::istreambuf_iterator<uint8_t>());


    auto img = new CCImage();
    constexpr size_t iters = 32;

    log::info("Benchmarking ({} iters)", iters);

    // preconvert image to fpng
    auto rawResult = fastpng::decodePNG(data.data(), data.size());
    if (!rawResult) {
        log::warn("SPNG failed: {}", rawResult.unwrapErr());
        return;
    }

    auto rawData = std::move(rawResult.unwrap());
    auto fpngResult = fastpng::encodePNG(rawData.data, rawData.dataSize, rawData.width, rawData.height);
    if (!fpngResult) {
        log::warn("FPNG failed: {}", fpngResult.unwrapErr());
        return;
    }

    auto fpngData = std::move(fpngResult.unwrap());

    auto took1 = BENCH({
        for (size_t i = 0; i < iters; i++) {
            img->initWithImageData(data.data(), data.size());
        }
    });

    auto took2 = BENCH({
        for (size_t i = 0; i < iters; i++) {
            auto res = static_cast<fastpng::CCImageExt*>(img)->initWithSPNG(data.data(), data.size());
            if (!res) {
                log::warn("SPNG failed: {}", res.unwrapErr());
                break;
            }
        }
    });

    auto took3 = BENCH({
        for (size_t i = 0; i < iters; i++) {
            auto res = static_cast<fastpng::CCImageExt*>(img)->initWithFPNG(fpngData.data(), fpngData.size());
            if (!res) {
                log::warn("FPNG failed: {}", res.unwrapErr());
                break;
            }
        }
    });

    log::info("libpng took: {}", formatDuration(took1));
    log::info("spng took: {}", formatDuration(took2));
    log::info("fpng took: {}", formatDuration(took3));
}

#endif