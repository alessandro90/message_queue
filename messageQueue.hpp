#ifndef MESSAGE_QUEUE
#define MESSAGE_QUEUE

#include <mutex>
#include <type_traits>
#include <memory>
#include <utility>

namespace mq {

    template<typename T, typename U = int>
    using enabled = typename std::enable_if_t<std::is_copy_constructible_v<T>, U>;


    enum class Mode {
        FIFO,
        LIFO,
    };

    template<typename Mtype, enabled<Mtype> = 0>
    class BaseQueue {
    public:
        virtual void pop_front() = 0;
        virtual void pop_back() = 0;
        virtual void push(Mtype const& msg) = 0;
        virtual Mtype& back() = 0;
        virtual Mtype& front() = 0;
        virtual std::size_t size() = 0;
        virtual bool empty() = 0;
    };

    template<typename Mtype, typename QueueType, enabled<Mtype> = 0>
    class DerivedQueue: public BaseQueue<Mtype> {
    public:
        explicit DerivedQueue(QueueType queue_): queue{queue_} {}

        virtual void pop_front() final {
            queue.pop_front();
        }
        virtual void pop_back() final {
            queue.pop_back();
        }
        virtual void push(Mtype const& msg) final {
            queue.push_back(msg);
        }
        virtual Mtype& back() final {
            return queue.back();
        }
        virtual Mtype& front() final {
            return queue.front();
        }
        virtual std::size_t size() final {
            return queue.size();
        }
        virtual bool empty() final {
            return queue.empty();
        }
    private:
        QueueType queue;
    };

    template<typename Mtype, enabled<Mtype> = 0>
    class BaseQueueManipulator {
    public:
        virtual void pop(BaseQueue<Mtype>& messq) = 0;
        virtual Mtype const& get(BaseQueue<Mtype>& messq) const = 0;
        virtual void push(Mtype const& msg, BaseQueue<Mtype>& messq) {
            messq.push(msg);
        }
        virtual Mode get_mode() const noexcept { return qmode; }
        virtual ~BaseQueueManipulator() = default;
        BaseQueueManipulator(Mode qmode_): qmode{qmode_} {}
    private:
        Mode qmode;
    };

    template<typename Mtype, enabled<Mtype> = 0>
    class QueueManipulatorFIFO: public BaseQueueManipulator<Mtype> {
    public:
        QueueManipulatorFIFO(): BaseQueueManipulator<Mtype>{Mode::FIFO} {}

        virtual void pop(BaseQueue<Mtype>& messq) final {
            messq.pop_front();
        }
        virtual Mtype const& get(BaseQueue<Mtype>& messq) const final {
            return messq.front();
        }
    };


    template<typename Mtype, enabled<Mtype> = 0>
    class QueueManipulatorLIFO: public BaseQueueManipulator<Mtype> {
    public:
        QueueManipulatorLIFO(): BaseQueueManipulator<Mtype>{Mode::LIFO} {}
 
        virtual void pop(BaseQueue<Mtype>& messq) final {
            messq.pop_back();
        }
        virtual Mtype const& get(BaseQueue<Mtype>& messq) const final {
            return messq.back();
        }
    };


    template<typename Mtype, enabled<Mtype> = 0>
    class Queue {

    public:
        template<typename QueueType>
        explicit Queue(QueueType&& msg_queue_) {
            msg_queue = std::make_unique<
                DerivedQueue<Mtype, std::decay_t<QueueType>>>(
                std::move(msg_queue_)
            );
        }

        bool full() const noexcept { return msg_queue->size() == max_size; }
        bool empty() const noexcept { return msg_queue->empty(); }
        void pop() { queue_manipulator->pop(*msg_queue); }
        
        bool push(Mtype const& msg) {
            if (full()) return false;
            queue_manipulator->push(msg, *msg_queue);
            return true;
        }

        template<typename MessageReader>
        bool process(MessageReader&& reader) {
            std::lock_guard lck{mutex};
            if (msg_queue->empty()) return false;
            Mtype const& msg = queue_manipulator->get(*msg_queue);
            if (reader(msg)) {
                pop();
                return true;
            }
            return false;
        }
        bool load(Mtype const& msg) {
            std::lock_guard lck{mutex};
            return push(msg);
        }
        void set_size(std::size_t size) noexcept { max_size = size; }
        std::size_t size() const noexcept { return max_size; }
        std::size_t count() const noexcept { return msg_queue->size(); }
        void set_mode(Mode new_mode) noexcept {
            switch (new_mode) {
                case Mode::FIFO:
                    queue_manipulator.reset(new QueueManipulatorFIFO<Mtype>{});
                break;
                case Mode::LIFO:
                    queue_manipulator.reset(new QueueManipulatorLIFO<Mtype>{});
                break;
                default:
                break;
            }
        }
        Mode mode() const noexcept { return queue_manipulator->get_mode(); }
    private:
        std::unique_ptr<BaseQueueManipulator<Mtype>> queue_manipulator {
            new QueueManipulatorLIFO<Mtype>{}
        };
        std::unique_ptr<BaseQueue<Mtype>> msg_queue;
        std::mutex mutex{};
        std::size_t max_size{1000};
    };
    template<typename Mtype = void, typename QueueType>
    explicit Queue(QueueType&&) -> Queue<typename QueueType::value_type>;


    template<typename Mtype, enabled<Mtype> = 0>
    class Receiver {
    public:
        Receiver(Queue<Mtype>& q): queue{q} {}
        template<typename Reader,
            typename std::enable_if_t<std::is_invocable_r_v<bool, Reader, Mtype>, int> = 0>
        bool listen(Reader&& reader) {
            return queue.process(std::forward<Reader>(reader));
        }

    private:
        Queue<Mtype> &queue;
    };
    template<typename Mtype>
    Receiver(Queue<Mtype>&) -> Receiver<Mtype>;

    template<typename Mtype, enabled<Mtype> = 0>
    class BlockingReceiver: public Receiver<Mtype> {};


    template<typename Mtype, enabled<Mtype> = 0>
    class Producer {

    public:
        Producer(Queue<Mtype>& q): queue{q} {}
        bool send(Mtype const& msg) {
            return queue.load(msg);
        }

    private:
        Queue<Mtype> &queue;
    };
    template<typename Mtype>
    Producer(Queue<Mtype>&) -> Producer<Mtype>;
}

#endif
