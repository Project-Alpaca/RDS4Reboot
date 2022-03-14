/**
 * Defines common data blobs for multiple transport classes.
 */
#include "Transport.hpp"

namespace rds4 {
namespace ds4 {

#ifndef RDS4_TEENSY_3
// Only declare this when not on teensy 3
const uint8_t _ds4_report_desc[] = {
    0x05, 0x01,       /*  Usage Page (Desktop),           */
    0x09, 0x05,       /*  Usage (Gamepad),                */
    0xA1, 0x01,       /*  Collection (Application),       */
    0x85, 0x01,       /*    Report ID (1),                */
    0x09, 0x30,       /*    Usage (X),                    */
    0x09, 0x31,       /*    Usage (Y),                    */
    0x09, 0x32,       /*    Usage (Z),                    */
    0x09, 0x35,       /*    Usage (Rz),                   */
    0x15, 0x00,       /*    Logical Minimum (0),          */
    0x26, 0xFF, 0x00, /*    Logical Maximum (255),        */
    0x75, 0x08,       /*    Report Size (8),              */
    0x95, 0x04,       /*    Report Count (4),             */
    0x81, 0x02,       /*    Input (Variable),             */
    0x09, 0x39,       /*    Usage (Hat Switch),           */
    0x15, 0x00,       /*    Logical Minimum (0),          */
    0x25, 0x07,       /*    Logical Maximum (7),          */
    0x35, 0x00,       /*    Physical Minimum (0),         */
    0x46, 0x3B, 0x01, /*    Physical Maximum (315),       */
    0x65, 0x14,       /*    Unit (Degrees),               */
    0x75, 0x04,       /*    Report Size (4),              */
    0x95, 0x01,       /*    Report Count (1),             */
    0x81, 0x42,       /*    Input (Variable, Null State), */
    0x65, 0x00,       /*    Unit,                         */
    0x05, 0x09,       /*    Usage Page (Button),          */
    0x19, 0x01,       /*    Usage Minimum (01h),          */
    0x29, 0x0E,       /*    Usage Maximum (0Eh),          */
    0x15, 0x00,       /*    Logical Minimum (0),          */
    0x25, 0x01,       /*    Logical Maximum (1),          */
    0x75, 0x01,       /*    Report Size (1),              */
    0x95, 0x0E,       /*    Report Count (14),            */
    0x81, 0x02,       /*    Input (Variable),             */
    0x06, 0x00, 0xFF, /*    Usage Page (FF00h),           */
    0x09, 0x20,       /*    Usage (20h),                  */
    0x75, 0x06,       /*    Report Size (6),              */
    0x95, 0x01,       /*    Report Count (1),             */
    0x81, 0x02,       /*    Input (Variable),             */
    0x05, 0x01,       /*    Usage Page (Desktop),         */
    0x09, 0x33,       /*    Usage (Rx),                   */
    0x09, 0x34,       /*    Usage (Ry),                   */
    0x15, 0x00,       /*    Logical Minimum (0),          */
    0x26, 0xFF, 0x00, /*    Logical Maximum (255),        */
    0x75, 0x08,       /*    Report Size (8),              */
    0x95, 0x02,       /*    Report Count (2),             */
    0x81, 0x02,       /*    Input (Variable),             */
    0x06, 0x00, 0xFF, /*    Usage Page (FF00h),           */
    0x09, 0x21,       /*    Usage (21h),                  */
    0x95, 0x36,       /*    Report Count (54),            */
    0x81, 0x02,       /*    Input (Variable),             */
    0x85, 0x05,       /*    Report ID (5),                */
    0x09, 0x22,       /*    Usage (22h),                  */
    0x95, 0x1F,       /*    Report Count (31),            */
    0x91, 0x02,       /*    Output (Variable),            */
    0x85, 0x03,       /*    Report ID (3),                */
    0x0A, 0x21, 0x27, /*    Usage (2721h),                */
    0x95, 0x2F,       /*    Report Count (47),            */
    0xB1, 0x02,       /*    Feature (Variable),           */
    0xC0,             /*  End Collection,                 */
    0x06, 0xF0, 0xFF, /*  Usage Page (FFF0h),             */
    0x09, 0x40,       /*  Usage (40h),                    */
    0xA1, 0x01,       /*  Collection (Application),       */
    0x85, 0xF0,       /*    Report ID (240),              */
    0x09, 0x47,       /*    Usage (47h),                  */
    0x95, 0x3F,       /*    Report Count (63),            */
    0xB1, 0x02,       /*    Feature (Variable),           */
    0x85, 0xF1,       /*    Report ID (241),              */
    0x09, 0x48,       /*    Usage (48h),                  */
    0x95, 0x3F,       /*    Report Count (63),            */
    0xB1, 0x02,       /*    Feature (Variable),           */
    0x85, 0xF2,       /*    Report ID (242),              */
    0x09, 0x49,       /*    Usage (49h),                  */
    0x95, 0x0F,       /*    Report Count (15),            */
    0xB1, 0x02,       /*    Feature (Variable),           */
    0x85, 0xF3,       /*    Report ID (243),              */
    0x0A, 0x01, 0x47, /*    Usage (4701h),                */
    0x95, 0x07,       /*    Report Count (7),             */
    0xB1, 0x02,       /*    Feature (Variable),           */
    0xC0              /*  End Collection                  */
};
#endif

} // namespace ds4
} // namespace rds4
