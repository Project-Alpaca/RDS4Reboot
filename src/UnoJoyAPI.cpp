#include "UnoJoyAPI.hpp"

namespace rds4 {

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
