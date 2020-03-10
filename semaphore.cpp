#include "semaphore.hpp"

namespace sem {
    Semaphore::Semaphore(std::size_t max_slots_, std::size_t slots_ = 0): max_slots{max_slots_}, slots{slots_} {}
    void Semaphore::acquire() {
        std::unique_lock lk{m};
        cv.wait(lk, [&]{ return slots > 0; });
        --slots;
    }

    void Semaphore::release() {
        std::unique_lock lk{m};
        if (slots < max_slots) ++slots;
        lk.unlock();
        cv.notify_all();
    }
}