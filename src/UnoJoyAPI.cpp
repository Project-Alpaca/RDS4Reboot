#include "UnoJoyAPI.hpp"

namespace rds4 {

template <class T>
void UnoJoyAPI<T>::setControllerData(dataForController_t buf) {
    auto *t = static_cast<T *>(this);
    t->setKeyUniversal(Key::X, buf.triangleOn);
    t->setKeyUniversal(Key::A, buf.circleOn);
    t->setKeyUniversal(Key::Y, buf.squareOn);
    t->setKeyUniversal(Key::B, buf.crossOn);
    t->setKeyUniversal(Key::LButton, buf.l1On);
    t->setKeyUniversal(Key::LTrigger, buf.l2On);
    t->setKeyUniversal(Key::LStick, buf.l3On);
    t->setKeyUniversal(Key::RButton, buf.r1On);
    t->setKeyUniversal(Key::RTrigger, buf.r2On);
    t->setKeyUniversal(Key::RStick, buf.r3On);
    t->setKeyUniversal(Key::Select, buf.selectOn);
    t->setKeyUniversal(Key::Start, buf.startOn);
    t->setKeyUniversal(Key::Home, buf.homeOn);
    t->setDpadUniversalSOCD(buf.dpadUpOn, buf.dpadRightOn, buf.dpadDownOn, buf.dpadLeftOn);
    t->setStick(Stick::L, buf.leftStickX, buf.leftStickY);
    t->setStick(Stick::R, buf.rightStickX, buf.rightStickY);
}

dataForController_t getBlankDataForController() {
    dataForController_t buf;
    memset(&buf, 0, sizeof(buf));
    buf.leftStickX = 0x80;
    buf.leftStickY = 0x80;
    buf.rightStickX = 0x80;
    buf.rightStickY = 0x80;
    return buf;
}

}
