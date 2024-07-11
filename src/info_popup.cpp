#include "info_popup.hpp"

#include <UIBuilder.hpp>
#include <manager.hpp>

using namespace geode::prelude;

static std::string formatBytes(uint64_t bytes) {
    // i did not write this myself
    static const char* suffixes[] = {"B", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB", "ZiB", "YiB"};

    if (bytes == 0) {
        return "0B";
    }

    int exp = static_cast<int>(std::log2(bytes) / 10);
    double value = static_cast<double>(bytes) / std::pow(1024, exp);

    std::ostringstream oss;

    if (std::fmod(value, 1.0) == 0) {
        oss << static_cast<int>(value);
    } else {
        oss << std::fixed << std::setprecision(1) << value;
    }

    oss << suffixes[exp];

    return oss.str();
}

bool FastpngPopup::setup() {
    this->setTitle("FastPNG setup");

    this->remakeLabel();

    Build<ButtonSprite>::create("Clear", "bigFont.fnt", "GJ_button_01.png", 0.5f)
        .intoMenuItem([this](auto) {
            auto dir = FastpngManager::get().getCacheDir();

            std::error_code ec;
            std::filesystem::directory_iterator iterator(dir);
            if (ec != std::error_code{}) {
                FLAlertLayer::create("FastPNG", fmt::format("Failed to remove files: {}", ec.message()), "Ok")->show();
                return;
            }

            for (auto& file : iterator) {
                if (!file.is_regular_file()) continue;

                std::error_code ec;
                std::filesystem::remove(file, ec);
            }

            FLAlertLayer::create("FastPNG", "Deleted all cached images!", "Ok")->show();
            this->remakeLabel();
        })
        .pos(m_size.width / 2.f, m_size.height - 90.f)
        .intoNewParent(CCMenu::create())
        .pos(0.f, 0.f)
        .parent(m_mainLayer);
    ;

    return true;
}

void FastpngPopup::remakeLabel() {
    if (infolabel) infolabel->removeFromParent();

    // calculate size
    auto dir = FastpngManager::get().getCacheDir();

    size_t totalSize = 0;
    size_t totalImages = 0;

    std::error_code ec;
    auto iterator = std::filesystem::directory_iterator(dir, ec);
    if (ec != std::error_code{}) {
        FLAlertLayer::create("FastPNG", fmt::format("Failed to calculate the cache size: {}", ec.message()), "Ok")->show();
        return;
    }

    for (auto& file : iterator) {
        if (!file.is_regular_file()) continue;

        std::error_code ec;
        totalSize += std::filesystem::file_size(file, ec);

        if (ec == std::error_code{}) {
            totalImages++;
        }
    }

    Build<CCLabelBMFont>::create(fmt::format("Cache size: {} images ({})", totalImages, formatBytes(totalSize)).c_str(), "bigFont.fnt")
        .scale(0.38f)
        .pos(m_size.width / 2.f, m_size.height - 50.f)
        .parent(m_mainLayer)
        .store(this->infolabel);
}

FastpngPopup* FastpngPopup::create() {
    auto ret = new FastpngPopup;
    if (ret->initAnchored(260.f, 120.f)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
