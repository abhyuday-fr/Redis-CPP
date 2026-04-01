## The need for input buffers
Since reads are now non-blocking, we cannot just wait for n bytes while parsing the protocol; the read_full() function is now irrelevant. We’ll do this instead:

At each loop iteration, if the socket is ready to read:

- Do a non-blocking read.
- Add new data to the Conn::incoming buffer.
- Try to parse the accumulated buffer.
- If there is not enough data, do nothing in that iteration.
- Process the parsed message.
- Remove the message from Conn::incoming.

## Why buffer output data?

Since writes are now non-blocking, we cannot write to sockets at will; data is written iff the socket is ready to write. A large response may take multiple loop iterations to complete. So the response data must be stored in a buffer (Conn::outgoing).

## Map from fd to connection state
poll() returns a fd list. We need to map each fd to the Conn object.

```
    // a map of all client connections, keyed by fd
    std::vector<Conn *> fd2conn;
```

On Unix, an fd is allocated as the smallest available non-negative integer, so the mapping from fd to Conn can be a flat array indexed by fd, and the array will be densely packed. Nothing can be more efficient. Sometimes simple arrays can replace complex data structures like hashtables.