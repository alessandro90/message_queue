/*

    A simple example of usage.
    Two communicating tasks are simulated.

*/

#include <iostream>
#include <cstddef>
#include <thread>
#include <random>
#include <chrono>
#include <functional>
#include <array>
#include <type_traits>
#include "../messageQueue.hpp"

using seconds = std::chrono::duration<double>;


class RandomElementGetter {
private:
    std::random_device rd{};
    std::mt19937 gen{rd()};
    std::uniform_int_distribution<> dis;
public:
    RandomElementGetter(std::size_t min, std::size_t max): dis(min, max) {};
    RandomElementGetter(std::size_t max): RandomElementGetter(0, max) {}
    
    auto get() { return dis(gen); }

    template<
        typename T,
        typename = std::void_t<decltype(std::declval<T>()[std::declval<std::size_t>()])>
    >
    auto& get(T const& container) {
        return container[dis(gen)];
    }
};


enum class Action {
    ACTION_NONE,
    ACTION_1,
    ACTION_2,
    ACTION_3,
    ACTION_4,
    ACTION_5,
    ACTION_6,
    ACTION_7,
};

std::ostream& operator<<(std::ostream& os, Action const& action) {
    os << static_cast<int>(action);
    return os;
}

template<std::size_t N>
bool message_consumable(std::array<Action, N> const &actions, Action const& message) noexcept {
    for (Action const &a : actions)
        if (a == message) return true;
    return false;
}

class ListenerOne: public mq::Receiver<Action> {
    std::array<Action, 3> actions {
        Action::ACTION_1,
        Action::ACTION_2,
        Action::ACTION_3,
    };
    virtual bool consumed(Action const& message) const noexcept override {
        return message_consumable(actions, message);
    }
};

class ListenerTwo: public mq::Receiver<Action> {
    std::array<Action, 4> actions {
        Action::ACTION_4,
        Action::ACTION_5,
        Action::ACTION_6,
        Action::ACTION_7,
    };

    virtual bool consumed(Action const& message) const noexcept override {
        return message_consumable(actions, message);
    }
};


class ListenerTaskOne {

    RandomElementGetter r{6, 10};

    void process(Action const& message) {
        std::cerr << "ListenerTaskOne received ";
        switch (message) {
        case Action::ACTION_1:
            std::cerr << "ACTION_1\n";
            break;
        case Action::ACTION_2:
            std::cerr << "ACTION_2\n";
            break;
        case Action::ACTION_3:
            std::cerr << "ACTION_3\n";
            break;
        default:
            std::cerr << "an unhandled message.\n";
            break;
        }
    }

public:
    ListenerOne receiver{};
    void operator()() {
        while (true) {
            Action message{Action::ACTION_NONE};
            try {
                if (receiver.listen(message)) process(message);
            } catch (mq::BaseMessageQueueException const& e) {
                std::cerr << e.what() << "\n";
            }
            // Simulate some time-consuming task.
            std::this_thread::sleep_for(seconds(r.get()));
        }
    }
};

class ListenerTaskTwo {

    RandomElementGetter r{3, 8};

    void process(Action const& message) {
        std::cerr << "ListenerTaskTwo received ";
        switch (message) {
        case Action::ACTION_4:
            std::cerr << "ACTION_4\n";
            break;
        case Action::ACTION_5:
            std::cerr << "ACTION_4\n";
            break;
        case Action::ACTION_6:
            std::cerr << "ACTION_6\n";
            break;
        case Action::ACTION_7:
            std::cerr << "ACTION_7\n";
            break;
        default:
            std::cerr << "an unhandled message.\n";
            break;
        }
    }

public:
    ListenerTwo receiver{};
    void operator()() {
        while (true) {
            Action message{Action::ACTION_NONE};
            try{
                if (receiver.listen(message)) process(message);
            } catch (mq::BaseMessageQueueException const& e) {}
            // Simulate some time-consuming task.
            std::cerr << "Detached: " << receiver.detached();
            std::this_thread::sleep_for(seconds(r.get()));
        }
    }
};


class ProducerTask {
    std::array<Action, 7> actions{
        Action::ACTION_1,
        Action::ACTION_2,
        Action::ACTION_3,
        Action::ACTION_4,
        Action::ACTION_5,
        Action::ACTION_6,
        Action::ACTION_7,
    };
    RandomElementGetter r{1, 3};
    RandomElementGetter r_element{actions.size()};
public:
    mq::Producer<Action> producer{};
    void operator()() {
        while (true) {
            producer.send(r_element.get(actions));
            std::cerr << "Producer task queue size: " << producer.queue_size() << "\n";
            std::this_thread::sleep_for(seconds(r.get()));
        }
    }
};

int main() {
    ProducerTask producer_task{};
    ListenerTaskOne listener_task{};
    ListenerTaskTwo listener_task_two{};
    producer_task.producer.attach(listener_task.receiver);
    producer_task.producer.attach(listener_task_two.receiver);
    producer_task.producer.set_max_len(10);
    listener_task.receiver.set_blocking(true, 30);

    // The arguments of the std:thread ctor are moved or copied by value.
    std::thread producer_thread{std::ref(producer_task)};
    std::thread listener_thread{std::ref(listener_task)};
    std::thread listener_two_thread{std::ref(listener_task_two)};
    listener_thread.join();
    listener_two_thread.join();
    producer_thread.join();

    return 0;
}
