#include "synchronizer.hpp"

namespace sync {
Synchronizer::Synchronizer(sem::Semaphore& sem_a_,
    sem::Semaphore& sem_b_,
    std::mutex& m_)
    : sem_a { sem_a_ }
    , sem_b { sem_b_ }
    , lck { m_, std::defer_lock }
{
    sem_a.acquire();
    lck.lock();
}

Synchronizer::~Synchronizer()
{
    lck.unlock();
    sem_b.release();
}
}