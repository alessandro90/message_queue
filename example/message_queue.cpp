/*

    A simple example of usage.
    Two communicating tasks are simulated.

*/

#include <iostream>
#include <cstdio>
#include <cstddef>
#include <thread>
#include <random>
#include <chrono>
#include <functional>
#include <array>
#include <type_traits>
#include <deque>
#include <algorithm>
#include "../messageQueue.hpp"

using seconds = std::chrono::duration<double>;


class RandomElementGetter {
private:
    std::random_device rd{};
    std::mt19937 gen{rd()};
    std::uniform_int_distribution<> dis;
public:
    RandomElementGetter(std::size_t max, std::size_t min = 0): dis(min, max) {};
    
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
    ACTION_1,
    ACTION_2,
    ACTION_3,
    ACTION_4,
    ACTION_5,
    ACTION_6,
    ACTION_7,
};

std::ostream& operator<<(std::ostream& os, Action const& action) {
    os << static_cast<int>(action) + 1;
    return os;
}

template<int N>
struct MessageReader {
    MessageReader(std::array<Action, N> actions_, int id_): actions{actions_}, id{id_} {}
    std::array<Action, N> actions;
    int id;

    void process(Action const& message) const {
        std::cout << "ListenerTask " << id << " received ";
        std::cout << message << '\n';
    }

    bool operator()(Action const& message) const {
        process(message);
        return std::find(
            std::begin(actions), std::end(actions), message
        ) != std::end(actions);
    }
};


class ListenerTask {

    RandomElementGetter r{9, 1};
    mq::Queue<Action>& q;
public:
    ListenerTask(mq::Queue<Action> &queue): q{queue} {}
    void operator()() {
        mq::Receiver receiver{q};
        auto reader = MessageReader<3>{std::array{
            Action::ACTION_1,
            Action::ACTION_2,
            Action::ACTION_3,
        }, 1};
        while (true) {
            receiver.listen(reader);
            // Simulate some time-consuming task.
            std::this_thread::sleep_for(seconds(r.get()));
        }
    }
};

class ListenerTaskTwo {

    RandomElementGetter r{8, 3};
    mq::Queue<Action>& q;
public:
    ListenerTaskTwo(mq::Queue<Action> &queue): q{queue} {}
    void operator()() {
        mq::Receiver receiver{q};
        auto reader = MessageReader<4>{std::array{
            Action::ACTION_4,
            Action::ACTION_5,
            Action::ACTION_6,
            Action::ACTION_7,
        }, 2};
        while (true) {
            receiver.listen(reader);
            // Simulate some time-consuming task.
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
    RandomElementGetter r{3, 1};
    RandomElementGetter r_element{actions.size()};
    mq::Queue<Action>& q;
public:
    ProducerTask(mq::Queue<Action> &queue): q{queue} {}
    void operator()() {
        mq::Producer producer{q};
        while (true) {
            producer.send(r_element.get(actions));
            std::this_thread::sleep_for(seconds(r.get()));
        }
    }
};

int main() {

    mq::Queue queue{std::deque<Action>{}, 10};
    ProducerTask producer_task{queue};
    ListenerTask listener_task{queue};
    ListenerTaskTwo listener_task2{queue};

    // The arguments of the std:thread ctor are moved or copied by value.
    std::thread producer_thread{std::ref(producer_task)};
    std::thread listener_thread{std::ref(listener_task)};
    std::thread listener2_thread{std::ref(listener_task2)};
    listener_thread.join();
    listener2_thread.join();
    producer_thread.join();

    return 0;
}
