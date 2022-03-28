/**
 * Wrapper to system threading helpers and synchronization primitives.
 */

#pragma once

#include "platform.hpp"

#if defined(ARDUINO_ARCH_RP2040)
#include <pico/sync.h>
#include <pico/multicore.h>
#define RDS4_HAS_THREADING
#define RDS4_THREADING_RP2040_SDK
#endif

#ifdef RDS4_HAS_THREADING
namespace rds4 {
namespace threading {

class Lock {
public:
    Lock();
    ~Lock();
    bool acquire(int32_t timeout_ms);
    void release();
private:
#if defined(RDS4_THREADING_RP2040_SDK)
    mutex_t nativeLock;
#endif // RDS4_THREADING_RP2040_SDK
};

/** Simple event used to wakeup threads. */
class Event {
public:
    Event();
    ~Event();
    bool get();
    void set();
    void clear();
    bool wait();
private:
#if defined(RDS4_THREADING_RP2040_SDK)
    volatile bool flag;
#endif // RDS4_THREADING_RP2040_SDK
};
}
}
#endif // RDS4_HAS_THREADING
