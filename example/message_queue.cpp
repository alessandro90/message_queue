/*

    A simple example of usage.
    Three communicating tasks are simulated.

*/

#include "../messageQueue.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <deque>
#include <functional>
#include <iostream>
#include <random>
#include <thread>
#include <type_traits>

using seconds = std::chrono::duration<double>;

class RandomElementGetter {
private:
    std::random_device rd {};
    std::mt19937 gen { rd() };
    std::uniform_int_distribution<> dis;

public:
    explicit RandomElementGetter(std::size_t max, std::size_t min = 0)
        : dis(min, max) {};

    auto get()
    {
        return dis(gen);
    }

    template <typename T>
    auto get(T const& container) requires requires { std::declval<T>()[std::declval<std::size_t>()]; }
    {
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

std::ostream& operator<<(std::ostream& os, Action const& action)
{
    os << static_cast<int>(action) + 1;
    return os;
}

class ListenerTask {

    RandomElementGetter r { 9, 1 };
    mq::Queue<Action>& q;

public:
    explicit ListenerTask(mq::Queue<Action>& queue)
        : q { queue }
    {
    }
    void operator()()
    {
        mq::Receiver receiver { q };
        std::array supported {
            Action::ACTION_1,
            Action::ACTION_2,
            Action::ACTION_3,
        };
        while (true) {
            auto m = receiver.dequeue_if([&supported](Action const& a) {
                return std::find(std::begin(supported),
                           std::end(supported), a)
                    != std::end(supported);
            });
            if (m) {
                std::cout << "ListenerTask "
                          << 1
                          << " received "
                          << m.value()
                          << '\n';
            }
            // Simulate some time-consuming task.
            std::this_thread::sleep_for(seconds(r.get()));
        }
    }
};

class ListenerTaskTwo {

    RandomElementGetter r { 8, 3 };
    mq::Queue<Action>& q;

public:
    explicit ListenerTaskTwo(mq::Queue<Action>& queue)
        : q { queue }
    {
    }
    void operator()()
    {
        mq::Receiver receiver { q };
        std::array supported {
            Action::ACTION_4,
            Action::ACTION_5,
            Action::ACTION_6,
            Action::ACTION_7,
        };
        while (true) {
            auto m = receiver.dequeue_if([&supported](Action const& a) {
                return std::find(std::begin(supported),
                           std::end(supported), a)
                    != std::end(supported);
            });
            if (m) {
                std::cout << "ListenerTask "
                          << 2
                          << " received "
                          << m.value()
                          << '\n';
            }
            // Simulate some time-consuming task.
            std::this_thread::sleep_for(seconds(r.get()));
        }
    }
};

class ProducerTask {
    std::array<Action, 7> actions {
        Action::ACTION_1,
        Action::ACTION_2,
        Action::ACTION_3,
        Action::ACTION_4,
        Action::ACTION_5,
        Action::ACTION_6,
        Action::ACTION_7,
    };
    RandomElementGetter r { 3, 1 };
    RandomElementGetter r_element { actions.size() };
    mq::Queue<Action>& q;

public:
    explicit ProducerTask(mq::Queue<Action>& queue)
        : q { queue }
    {
    }
    void operator()()
    {
        mq::Producer producer { q };
        while (true) {
            producer.enqueue(r_element.get(actions));
            std::this_thread::sleep_for(seconds(r.get()));
        }
    }
};

int main()
{

    mq::Queue queue { std::deque<Action> {}, 10 };
    ProducerTask producer_task { queue };
    ListenerTask listener_task { queue };
    ListenerTaskTwo listener_task2 { queue };

    std::jthread producer_thread { std::ref(producer_task) };
    std::jthread listener_thread { std::ref(listener_task) };
    std::jthread listener2_thread { std::ref(listener_task2) };
}
