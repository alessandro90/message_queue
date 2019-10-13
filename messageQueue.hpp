/** @file 
 * \brief Header-only template \c C++17 library which implements a message queue.
 * 
 * Exposes two main class templates:
 * \li \c Receiver<MessageType>;
 * \li \c Producer<MessageType>.
*/

#ifndef MESSAGE_QUEUE
#define MESSAGE_QUEUE

#include <deque>
#include <mutex>
#include <chrono>
#include <exception>
#include <type_traits>
#include <thread>


namespace mq {

    template<typename T, typename U = int>
    using enable_if_copy_constructible = typename std::enable_if_t<std::is_copy_constructible_v<T>, U>;


    template<typename MessageType, enable_if_copy_constructible<MessageType> = 0> class Producer;

    /** \brief Enum class describing if the queue is a FIFO or a LIFO. */
    enum class Mode {
        FIFO,
        LIFO,
    };


    /** \brief  Enum class which describes the action to undertake if the queue is full.
     * 
     * \li \c DO_NOTHING actually does nothing, the new message is lost;
     * \li \c SUB_FRONT substitute the last message with the incoming message;
     * \li \c REMOVE_BACK pop the first message and add the incoming message to the head of the queue;
     * \li \c THROW throw a \c FullQueueException. The incoming message is lost.
    */
    enum class FullQueuePolicy {
        DO_NOTHING,
        SUB_FRONT,
        REMOVE_BACK,
        THROW,
    };


    /** \brief Base exception class. */
    class BaseMessageQueueException: public std::exception {
        const char *msg{"Error: Base exception message."};
    public:
        virtual const char* what() const noexcept {
            return msg;
        }        
    };

    /** \brief Thrown when the message queue is full and \c FullQueuePolicy::THROW is specified. */
    class FullQueueException: public BaseMessageQueueException {
        const char *msg{"Error: Message queue reached maximum value."};
    };

    /** \brief Thrown when a Receiver try to get a message fron an empty queue. */
    class EmptyQueueException: public BaseMessageQueueException {
        const char *msg{"Error: No message to process."};
    };

    /** \brief Thrown when \c .listen() is called from a Receiver not connected with a queue. */
    class DetachedListenerException: public BaseMessageQueueException {
        const char *msg{"Error: your Receiver is detached."};
    };

    /** \brief Thrown when a blocking \c .listen() call reaches the specified timeout. */
    class WaitTimeoutException: public BaseMessageQueueException {
        const char *msg{"Error: elapsed timeout waiting for message."};
    };

    /** \brief Abstract base class template.
     * 
     * Can listen for an arbitrary (copyable) MessageType. It holds a pointer to a
     * message queue object belonging to a Producer object.
     */
    template<typename MessageType, enable_if_copy_constructible<MessageType> = 0>
    class Receiver {

    public:
        /** \brief Return an object of type MessageType.
         * 
         * The call can be either blocking or non-blocking depending on how the
         * receiver has been set. If the receiver is detached throws a \c DetachedListenerException.
         * Also \c EmptyQueueException or \c WaitTimeoutException can be thrown, depending on how
         * the receiver has been set.
        */
        MessageType listen() {
            if (is_detached) throw DetachedListenerException{};
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

        /** \brief Set the \c Receiver::listen method to a blocking or non-blocking mode.
         * 
         * @param[in] is_blocking Blocking or non-blocking mode;
         * @param[in] timeout Optional, default to 120. In blocking mode, the \listen
         * method will throw a \c WaitTimeoutException after \c timeout seconds has passed without being able to get a message.
         * A value \c <= 0 will let \c listen to wait possibly forever.
         * @param[in] wait_time In blocking mode, the time (in seconds) for which the thread will sleep before trying again to get a message.
        */
        void set_blocking(
            bool is_blocking,
            long int timeout = 120,
            float wait_time = 0.5
        ) noexcept {
            this->is_blocking = is_blocking;
            this->timeout = timeout;
            this->wait_time = wait_time;
        }

        /** \brief Set the queue mode (FIFO or LIFO) */
        void set_mode(Mode const& queue_mode) noexcept {
            this->queue_mode = queue_mode;
        }

        /** \brief Get the queue mode */
        Mode mode() {
            return queue_mode;
        }

        /** \brief Return \c true if the Receiver has a blocking \c listen method */
        bool blocking() {
            return is_blocking;
        }

        /** \brief Detach the Receiver from the Producer */
        void detach() {
            message_queue = nullptr;
            queue_rw = nullptr;
            is_detached = true;
        }

        /** \brief Return whether the Receiver is in a detached state. */
        bool detached() {
            return is_detached;
        }

    private:

        friend class Producer<MessageType>;

        std::deque<MessageType> *message_queue{nullptr};
        std::mutex *queue_rw{nullptr};
        Mode queue_mode{Mode::FIFO};
        bool is_blocking{false}, is_detached{true};
        long int timeout{120};
        float wait_time{1.};

        /** \brief Return a message in non-blocking mode and remove the message
         * from the queue if consumed(message) returns true */
        MessageType get_message_non_blocking() {
            MessageType message;
            if (std::lock_guard lck{*queue_rw}; !message_queue->empty()) {
                message = extract_message();
                if (consumed(message)) pop_message();
            } else {
                throw EmptyQueueException{};
            }
            return message;
        }


        /** \brief Return a message in blocking mode and remove the message
         * from the queue if consumed(message) returns true */
        MessageType get_message_blocking() {
            MessageType message;
            auto now = std::chrono::system_clock::now();
            while (true) {
                std::unique_lock ulck(*queue_rw);
                if (!message_queue->empty()) {
                    message = extract_message();
                    if (consumed(message)) pop_message();
                    ulck.unlock();
                    return message;
                } else {
                    ulck.unlock();
                    if (
                        timeout > 0 &&
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

        /** \brief Extract a message from the message queue. */
        MessageType extract_message() {
            MessageType message;
            if (queue_mode == Mode::LIFO) {
                message = message_queue->back();
            } else if (queue_mode == Mode::FIFO) {
                message = message_queue->front();
            }
            return message;
        }

        /** \brief Remove a message from the queue. */
        void pop_message() {
            if (queue_mode == Mode::LIFO) {
                message_queue->pop_back();
            } else if (queue_mode == Mode::FIFO) {
                message_queue->pop_front();
            }
        }

        /** \brief Initialize the message queue pointer to point to that of a producer object. */
        void set_message_queue(
            std::deque<MessageType>& producer_message_queue
        ) noexcept {
            message_queue = &producer_message_queue;
        }

        /** \brief Initialize the mutex pointer to point to that of a producer object. */
        void set_mutex(std::mutex& m) noexcept {
            queue_rw = &m;
        }

    protected:
        /** \brief Pure virtual function which is used to test if a message can be
         * removed from the message queue. */
        virtual bool consumed(MessageType const& message) const noexcept = 0;
    };



    /** \brief The producer class is used to send messages to various receivers objects.
     * 
     * Instances of this class cannot be copy-constructed or copy-assigned, but can be moved.
     * Any number of listeners can be attached to a single instance. */
    template<typename MessageType, enable_if_copy_constructible<MessageType>>
    class Producer {

    public:

        Producer(Producer const& p) = delete;
        Producer& operator=(Producer const& p) = delete;
        Producer() = default;
        Producer(Producer&& p) = default;
        Producer& operator=(Producer&& p) = default;

        /** \brief Set the maximum number of messages the queue can accumulate.
         * 
         * Setting zero disable any type of constraint. */
        void set_max_len(typename std::deque<MessageType>::size_type max_queue_size) noexcept {
            this->max_queue_size = max_queue_size;
        }

        /** \brief Send a message to any attached Receiver. */
        void send(MessageType const& message) {
            if (
                std::lock_guard grd{queue_rw};
                message_queue.size() >= max_queue_size
            ) {
                apply_full_queue_policy(message);
            } else {
                message_queue.push_back(message);
            }
        }

        /** \brief Attach a Receiver. */
        void attach(Receiver<MessageType>& receiver) noexcept {
            receiver.set_message_queue(message_queue);
            receiver.set_mutex(queue_rw);
            receiver.is_detached = false;
        }

        /** \brief Set the policy to undertake if the reaches its maximum allowed size. */
        void set_full_queue_policy(FullQueuePolicy const& policy) noexcept {
            full_queue_policy = policy;
        }

        /** \brief Return the current size of the message queue. */
        typename std::deque<MessageType>::size_type queue_size() {
            std::lock_guard lck{queue_rw};
            return message_queue.size();
        }

    private:
        typename std::deque<MessageType>::size_type max_queue_size{0};
        std::deque<MessageType> message_queue;
        std::mutex queue_rw;  // * Cannot be copied
        FullQueuePolicy full_queue_policy{FullQueuePolicy::DO_NOTHING};

        /** \brief Apply the chosen action if the queue reached the maximum lenght. */
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
                throw FullQueueException{};
                break;
            }
        }
    };
}

#endif
