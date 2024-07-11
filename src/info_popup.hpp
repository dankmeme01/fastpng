#pragma once
#include <Geode/Geode.hpp>

class FastpngPopup : public geode::Popup<> {
public:
    static FastpngPopup* create();

private:
    cocos2d::CCLabelBMFont* infolabel;

    bool setup();
    void remakeLabel();
};
