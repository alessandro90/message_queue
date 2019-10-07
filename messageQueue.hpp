#ifndef MESSAGE_QUEUE
#define MESSAGE_QUEUE

#include <deque>
#include <mutex>
#include <chrono>
#include <exception>


namespace mq {

    template<typename MessageType> class Producer;

    enum class Mode {
        FIFO,
        LIFO,
    };


    enum class FullQueuePolicy {
        DO_NOTHING,
        SUB_FRONT,
        REMOVE_BACK,
        THROW,
    };


    class BaseMessageQueueException: public std::exception {
        const char *msg{"Error: Base exception message."};
    public:
        virtual const char* what() const noexcept {
            return msg;
        }        
    };

    class FullQueueException: public BaseMessageQueueException {
        const char *msg{"Error: Message queue reached maximum value."};
    };

    class EmptyQueueException: public BaseMessageQueueException {
        const char *msg{"Error: No message to process."};
    };

    class DetachedListenerException: public BaseMessageQueueException {
        const char *msg{"Error: your listener is detached."};
    };

    class WaitTimeoutException: public BaseMessageQueueException {
        const char *msg{"Error: elapsed timeout waiting for message."};
    };


    template<typename MessageType>
    class Listener {

    public:


        MessageType listen() {
            if (!message_queue || !queue_rw) throw DetachedListenerException{};
            MessageType message;
            try {
                if (is_blocking)
                    message = get_message_blocking();
                else
                    message = get_message_non_blocking();
            } catch (BaseMessageQueueException const& e) {
                throw;
            }
            return message;
        }

        void set_blocking(
            bool is_blocking,
            long int timeout = 120,
            float wait_time = 0.5
        ) noexcept {
            this->is_blocking = is_blocking;
            this->timeout = timeout;
            this->wait_time = wait_time;
        }

        void set_mode(Mode const& queue_mode) noexcept {
            this->queue_mode = queue_mode;
        }

        Mode mode() {
            return queue_mode;
        }

        bool blocking() {
            return is_blocking;
        }

        void detach() {
            message_queue = nullptr;
            queue_rw = nullptr;
            is_detached = true;
        }

        bool detached() {
            return is_detached;
        }

    private:

        friend class Producer<MessageType>;

        std::deque<MessageType> *message_queue{nullptr};
        std::mutex *queue_rw{nullptr};
        Mode queue_mode{Mode::FIFO};
        bool is_blocking{false}, is_detached{false};
        long int timeout{120};
        float wait_time{1.};

        MessageType get_message_non_blocking() {
            MessageType message;
            std::lock_guard<std::mutex> unlk(*queue_rw);
            if (!message_queue->empty()) {
                message = extract_message();
                if (consumed(message)) pop_message();
            } else {
                throw EmptyQueueException{};
            }
            return message;
        }

        MessageType get_message_blocking() {
            MessageType message;
            auto now = std::chrono::system_clock::now();
            while (true) {
                std::unique_lock<std::mutex> ulck(*queue_rw);
                if (!message_queue->empty()) {
                    message = extract_message();
                    if (consumed(message)) pop_message();
                    ulck.unlock();
                    return message;
                } else {
                    ulck.unlock();
                    if (
                        std::chrono::duration<float>(
                            std::chrono::system_clock::now() - now
                        ).count() >= timeout
                    ) {
                        throw WaitTimeoutException{};
                    }
                    std::this_thread::sleep_for(
                        std::chrono::duration<float>(wait_time)
                    );
                }
            }
        }

        MessageType extract_message() {
            MessageType message;
            if (queue_mode == Mode::LIFO) {
                message = message_queue->back();
            } else if (queue_mode == Mode::FIFO) {
                message = message_queue->front();
            }
            return message;
        }

        void pop_message() {
            if (queue_mode == Mode::LIFO) {
                message_queue->pop_back();
            } else if (queue_mode == Mode::FIFO) {
                message_queue->pop_front();
            }
        }

        void set_message_queue(
            std::deque<MessageType>& producer_message_queue
        ) noexcept {
            message_queue = &producer_message_queue;
        }

        void set_mutex(std::mutex& m) noexcept {
            queue_rw = &m;
        }

    protected:
        virtual bool consumed(MessageType const& message) const noexcept = 0;
    };



    template<typename MessageType>
    class Producer {

    public:

        void set_max_len(unsigned int max_queue_size) noexcept {
            this->max_queue_size = max_queue_size;
        }

        void send(MessageType const& message) {
            std::lock_guard<std::mutex> grd(queue_rw);
            if (message_queue.size() >= max_queue_size) {
                apply_full_queue_policy(message);
            } else {
                message_queue.push_back(message);
            }
        }

        void attach(Listener<MessageType>& listener) noexcept {
            listener.set_message_queue(message_queue);
            listener.set_mutex(queue_rw);
            listener.is_detached = false;
        }

        void set_full_queue_policy(FullQueuePolicy const& policy) noexcept {
            full_queue_policy = policy;
        }

    private:
        unsigned int max_queue_size{30};
        std::deque<MessageType> message_queue;
        std::mutex queue_rw;  // * Cannot be copied
        FullQueuePolicy full_queue_policy{FullQueuePolicy::DO_NOTHING};

        void apply_full_queue_policy(MessageType const& message) {
            switch (full_queue_policy) {
            case FullQueuePolicy::DO_NOTHING:
                break;
            case FullQueuePolicy::SUB_FRONT:
                message_queue.front() = message;
                break;
            case FullQueuePolicy::REMOVE_BACK:
                message_queue.pop_back();
                message_queue.push_back(message);
                break;
            case FullQueuePolicy::THROW:
                throw(FullQueueException{});
                break;
            }
        }
    };
}

#endif
