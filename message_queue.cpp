#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <random>
#include <chrono>
#include <functional>
#include <array>


using system_clock = std::chrono::system_clock;
using duration = std::chrono::duration<double>;


enum class Action {
    ACTION_1 = 0,
    ACTION_2 = 1,
    ACTION_3 = 2,
    ACTION_4 = 3,
};



template<typename MessageType> class Producer;


template<typename MessageType>
class Listener {

    friend class Producer<MessageType>;

    std::queue<MessageType> *message_queue{nullptr};
    std::mutex *queue_rw{nullptr};

    MessageType pop_message() {
        MessageType message;
        std::unique_lock<std::mutex> unlk(*queue_rw);
        if(!message_queue->empty()) {
            message = message_queue->back();
            message_queue->pop();
        } else {
            unlk.unlock();
            throw std::exception{};
        }
        unlk.unlock();
        return message;
    }

    void set_message_queue(std::queue<MessageType>& producer_message_queue) {
        message_queue = &producer_message_queue;
    }

    void set_mutex(std::mutex& m) {
        queue_rw = &m;
    }

protected:
    virtual void process(MessageType const& message) = 0;

public:

    void listen() {
        MessageType message;
        try {
            message = pop_message();
        } catch(std::exception const& e) {
            return;
        }
        process(message);
    }

};


class SimpleListener: public Listener<Action> {
    virtual void process(Action const& message) {
        std::cout << "SimpleListener consume action: ";
        switch (message) {
        case Action::ACTION_1:
            std::cout << "ACTION_1\n";
            break;
        case Action::ACTION_2:
            std::cout << "ACTION_2\n";
            break;
        case Action::ACTION_3:
            std::cout << "ACTION_3\n";
            break;
        default:
            std::cout << "None\n";
            break;
        }
    }
};


class ListenerTask {
    std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<> dis{2, 6};

public:
    SimpleListener listener{};
    void operator()() {
        while(true) {
            listener.listen();
            // Simulate some time-consuming task.
            std::this_thread::sleep_for(duration(dis(gen)));
        }
    }

};


template<typename MessageType>
class Producer {

    unsigned int max_queue_size{30};
    std::queue<MessageType> message_queue;
    std::mutex queue_rw;  // * Cannot be copied

public:
    void set_max_len(unsigned int max_len) {
        max_queue_size = max_len;
    }

    void produce(MessageType const& message) {
        std::unique_lock<std::mutex> grd(queue_rw);
        if(message_queue.size() < max_queue_size)
            message_queue.emplace(message);
        grd.unlock();
    }

    void attach(Listener<MessageType>& listener) {
        listener.set_message_queue(message_queue);
        listener.set_mutex(queue_rw);
    }
};


class ProducerTask {
    std::array<Action, 4> actions{
        Action::ACTION_1,
        Action::ACTION_2,
        Action::ACTION_3,
        Action::ACTION_4
    };
    std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<> dis{1, 3};
    std::uniform_int_distribution<> enum_dis{0, static_cast<int>(actions.size()) - 1};


public:
    Producer<Action> producer{};
    void operator()() {
        while(true) {
            std::cout << "ProducerTask Produce\n";
            producer.produce(actions[enum_dis(gen)]);
            std::this_thread::sleep_for(duration(dis(gen)));
        }
    }
};

int main() {

    ProducerTask producer_task{};
    ListenerTask listener_task{};
    producer_task.producer.attach(listener_task.listener);
    producer_task.producer.set_max_len(25);

    // The arguments of the std:thread ctor are moved or copied by value.
    std::thread listener_thread{std::ref(producer_task)};
    std::thread producer_thread{std::ref(listener_task)};
    listener_thread.join();
    producer_thread.join();
    return 0;
}
