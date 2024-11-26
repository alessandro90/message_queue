#include "synchronizer.hpp"

namespace synch {
Synchronizer::Synchronizer(sem::Semaphore &sem_a_,
                           sem::Semaphore &sem_b_,
                           std::mutex &m_)
    : sem_a{sem_a_}
    , sem_b{sem_b_}
    , mtx{m_} {
    sem_a.acquire(m_);
}

Synchronizer::~Synchronizer() {
    mtx.unlock();
    sem_b.release();
}
}  // namespace synch
