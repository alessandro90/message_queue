# C++17 Template MEssage Queue

This is a simple `C++17 ` header file which implements a message queue. All the classes are templates, so that a message can be basically any copyable type.

Documentation can be read at [docs/html/index.html](docs/html/index.html).

In the `example` folder there is a basic example of usage. The library is aimed at allowing the comunication between tasks in different threads.

The example can be compiled (with the GNU compiler for example) with (the -std=c++17 flag is needed only if your compiler does not use the C++17 standard by default):

`g++ message_queue.cpp -std=c++17 -Wall -Wextra -Wpedantic`

Possible output:

```
Producer task queue size: 0
ListenerTaskOne received an unhandled message.
ListenerTaskTwo received ACTION_4
Producer task queue size: 1
ListenerTaskTwo received an unhandled message.
Producer task queue size: 2
ListenerTaskOne received ACTION_3
Producer task queue size: 2
Producer task queue size: 3
Producer task queue size: 4
ListenerTaskTwo received an unhandled message.
Producer task queue size: 5
Producer task queue size: 5
ListenerTaskOne received ACTION_1
ListenerTaskTwo received ACTION_7
Producer task queue size: 5
Producer task queue size: 6
Producer task queue size: 7
ListenerTaskOne received an unhandled message.
Producer task queue size: 8
Producer task queue size: 9
ListenerTaskTwo received ACTION_7
Producer task queue size: 9
ListenerTaskTwo received an unhandled message.
Producer task queue size: 10
Producer task queue size: 10
Producer task queue size: 9
ListenerTaskOne received ACTION_2
ListenerTaskTwo received an unhandled message.
```