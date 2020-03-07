# C++17 Template Message Queue

This is a simple `C++17 ` header file which implements a message queue. All the classes are templates, so that a message can be basically any copyable type.

In the `example` folder there is a basic example of usage. The library is aimed at allowing the comunication between tasks in different threads.

The example can be compiled (with the GNU compiler for example) with (the -std=c++17 flag is needed only if your compiler does not use the C++17 standard by default):

`g++ message_queue.cpp -std=c++17 -lpthread -Wall -Wextra -Wpedantic`

This is just an exercise to use some "advanced" features of C++.

Possible output:

```
ListenerTask 1 received 5
ListenerTask 2 received 5
ListenerTask 1 received 3
ListenerTask 2 received 6
ListenerTask 1 received 5
ListenerTask 2 received 5
ListenerTask 1 received 5
ListenerTask 2 received 5
ListenerTask 2 received 6
ListenerTask 1 received 2
ListenerTask 1 received 1
ListenerTask 2 received 5
```