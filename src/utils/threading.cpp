#include "threading.hpp"

#ifdef RDS4_HAS_THREADING
namespace rds4 {
namespace threading {
#ifdef RDS4_THREADING_RP2040_SDK
// RP2040 SDK mutex binding
Lock::Lock() {
    mutex_init(&(this->nativeLock));
}

Lock::~Lock() {}

bool Lock::acquire(int32_t timeout_ms) {
    if (timeout_ms == 0) {
        return mutex_try_enter(&(this->nativeLock), nullptr);
    }
    return mutex_enter_timeout_ms(&(this->nativeLock), timeout_ms);
}

void Lock::release() {
    mutex_exit(&(this->nativeLock));
}

// SEV/WFE based event implementation for baremetal ARM and Arduino RP2040
Event::Event(): flag(false) {};
Event::~Event() {};

bool Event::get() {
    return this->flag;
}

void Event::set() {
    this->flag = true;
    __sev();
}

void Event::clear() {
    this->flag = false;
}

bool Event::wait() {
    while (!this->flag) __wfe();
    return true;
}

#endif
}
}
#endif
