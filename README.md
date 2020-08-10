# C++17 Template Message Queue

This is a simple `C++20 ` header file which implements a message queue. All the classes are templates, so that a message can be basically any copyable type.

In the `example` folder there is a basic example of usage. The library is aimed at allowing the comunication between tasks in different threads.

The example can be compiled (with the GNU compiler for example) with (the -std=c++2a flag:

`g++ message_queue.cpp ../semaphore.cpp ../synchronizer.cpp -std=c++2a -lpthread -Wall -Wextra -Wpedantic -DDEBUG`

This is just an exercise to use some "advanced" features of C++.

Possible output:

```
Queue size after push: 1
ListenerTask 2 received 5
Queue size after push: 1
ListenerTask 1 received 6
Queue size after push: 2
ListenerTask 2 received 3
Queue size after push: 3
ListenerTask 1 received 1
Queue size after push: 3
ListenerTask 2 received 1
ListenerTask 1 received 1
Queue size after push: 3
ListenerTask 2 received 5
Queue size after push: 3
ListenerTask 1 received 1
```