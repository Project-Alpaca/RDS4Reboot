#include "Transport.hpp"
#include "utils/utils.hpp"

namespace rds4 {
namespace ds4 {

#if defined(RDS4_ARDUINO_MBED)
const uint8_t *TransportArduinoMbedUSBHID::report_desc() {
    RDS4_DBG_PRINTLN("send report desc");
    return _ds4_report_desc;
}

#endif

} // namespace ds4
} // namespace rds4
