#ifndef SYNCHRONIZER
#define SYNCHRONIZER

#include "semaphore.hpp"
#include <mutex>

namespace synch {
class Synchronizer {
public:
    Synchronizer(Synchronizer const &) = delete;
    Synchronizer(Synchronizer &&) = delete;
    Synchronizer &operator=(Synchronizer const &) = delete;
    Synchronizer &operator=(Synchronizer &&) = delete;
    Synchronizer(sem::Semaphore &sem_a_, sem::Semaphore &sem_b_, std::mutex &m_);
    ~Synchronizer();

private:
    // NOLINTNEXTLINE
    sem::Semaphore &sem_a, &sem_b;
    std::unique_lock<std::mutex> lck;
};
}  // namespace synch
#endif
