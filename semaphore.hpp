#ifndef SEMAPHORE
#define SEMAPHORE

#include <condition_variable>
#include <mutex>

namespace sem {
    class Semaphore {
    public:
        Semaphore(std::size_t max_slots_, std::size_t slots_);
        void acquire();
        void release();
    private:
        std::size_t const max_slots;
        std::size_t slots;
        std::condition_variable cv{};
        std::mutex m{};
    };
}
#endif