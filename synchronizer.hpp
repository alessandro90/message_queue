#ifndef SYNCHRONIZER
#define SYNCHRONIZER

#include <mutex>
#include "semaphore.hpp"

namespace sync {
    class Synchronizer {
    public:
        Synchronizer(sem::Semaphore &sem_a_, sem::Semaphore &sem_b_, std::mutex &m_);
        ~Synchronizer();

    private:
        sem::Semaphore &sem_a, &sem_b;
        std::unique_lock<std::mutex> lck;
    };
}
#endif