# Redis server CPP implementation 
As much as I love the C++ language, I am making my own Redis server in it.

## To build and run
- open bash/terminal and run this:
``` make server ``` and then
```./server ```

- Then open a new terminal window (without closing the original one) amd run this:
``` make client ``` and then 

    - To set a Key-Value pair in the server
    ``` ./client set <Key> <Value>```
    - To get a Key-Value pair in the server
    ``` ./client get```
    - To delete a Key-Value pair existing in the server
    ``` ./client del <Key> <Value> ```

## Concepts used in this project
1. TCP/IP and Socket Programming
2. Concurrency and Multithrerading
3. Mutex and Sync..
4. Data Structures -> hash tables, vectors
5. Parsing and RESP protocol
6. File I/O Presitance
7. Signal Handling
8. Command Processing and Response Formatting
9. Singleton Pattern
10. Bitwise Operators
11. std::

## What is Redis?
Redis is the most popular in-memory key-value store, used primarily for caching as no storage is faster than memory. Caching servers are inevitable because it’s the easiest way to scale. A cache can take slow DB queries out of the equation.
A cache server is a map<string,string> over a network. But in Redis, the “value” part is not just a string, it can be any of the data structures: hash, list, or sorted set, enabling complex use cases such as ranking, list pagination, etc. This is why Redis is called a data structure server.

## Why C/C++?
- I love C++
- High-performance software requires low-level control, which requires C/C++.

### Why didn't I do this project in some of the other programming languages I have worked in?
- Go is more high-level, but it’s still good for coding data structures and manipulating bits and bytes. It has a mature, built-in networking library, so I won’t learn the full networking lessons.
- Python is too high-level for coding data structures like hashtables, because it’s built-in. Production stuff is mostly C with Python glue. Python only has a thin wrapper for socket, so the networking lessons are still suitable.
- JavaScript is as high-level as Python, but with an invisible event loop and built-in networking. To fully understand JS or Go, you need to know more than JS or Go.
