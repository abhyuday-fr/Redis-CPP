# C++ Event-Loop TCP Server & Pipelined Client

## Overview
This project implements a single-threaded, non-blocking TCP server and a stress-testing client in modern C++. 

Instead of a traditional "wait-and-respond" or multi-threaded architecture, this server utilizes a **non-blocking event loop** powered by `poll()`. This is the exact same architectural foundation used by high-performance systems like Node.js and Redis, allowing it to juggle thousands of concurrent connections on a single thread without ever freezing.

---

## Part 1: The Client (`client.cpp`)
The client's job is to connect to the server, throw a batch of messages at it as fast as possible (including a massive 32 Megabyte message to stress-test the kernel buffers), and then wait to read all the replies.

### 1. The Setup & Helpers
* **`read_full()` and `write_all()`**: Networks are unpredictable. If you ask the OS to send 1,000 bytes, it might only send 400 because its internal pipes are full. These functions use a `while` loop to stubbornly insist: *"Keep trying until every single requested byte is sent or received."*
* **`buf_append()`**: A helper to attach new data to the end of a `std::vector` (a resizable array).
* **`k_max_msg`**: A hardcoded limit ensuring no single message exceeds 32 Megabytes.

### 2. Talking to the Server
* **`send_req(fd, text, len)`**: Packages the message. It takes the length of the text, puts it into a tiny 4-byte box (the header), glues the actual text right behind it, and fires the whole package over the network.
* **`read_res(fd)`**: Reads the server's reply in two steps:
  1. Reads exactly 4 bytes to find out the incoming message length.
  2. Prepares a buffer of that exact size and reads the rest of the message payload.
  3. Prints the message to the screen (truncated at 100 characters to prevent the 32MB message from flooding the terminal).

### 3. The Main Mission
* **Connection**: Grabs a socket, dials the server's address (`127.0.0.1:1234`), and connects.
* **The Pipelining Loop**: Instead of sending one message and waiting for a reply, it sprints through a list of requests and shoves *all* of them into the network pipe at once.
* **The Reading Loop**: Sits back and waits to receive all replies sequentially before closing the connection.

---

## Part 2: The Server (`server.cpp`)
The server's job is much harder. It must manage many clients concurrently on a **single thread** without ever getting stuck waiting for one slow client to send or receive data.

### 1. The Setup & The Client Folder
* **`fd_set_nb()`**: The non-blocking enabler. It tells the OS: *"If I try to read from a client and they haven't sent anything, don't freeze my program! Just give me an 'EAGAIN' error so I can move on."*
* **`struct Conn`**: Think of this as a manila folder for every connected client. It holds:
  * `fd`: Their unique network file descriptor.
  * `want_read` / `want_write`: "Sticky notes" telling the event loop what this client is waiting for.
  * `incoming`: An "Inbox" buffer for fragmented messages arriving over the network.
  * `outgoing`: An "Outbox" buffer for replies queued to be sent back.

### 2. The Callbacks (State Machine)
* **`handle_accept()`**: Triggered when the main listening socket rings. It accepts the connection, sets the new client's socket to non-blocking, creates a new `Conn` folder for them, and sets their sticky note to `want_read`.
* **`handle_read()`**: Triggered when a client has sent data.
  * Scoops up available data from the network pipe and dumps it into the client's `incoming` Inbox.
  * Calls `try_one_request()` repeatedly until the Inbox is drained.
  * If a reply is generated, swaps the sticky note from `want_read` to `want_write`.
* **`try_one_request()`**: Inspects the client's Inbox.
  * Checks for the 4-byte header. If missing, it stops and waits for more data.
  * Checks if the full payload has arrived. If missing, it stops and waits.
  * If a full message is present, it copies it into the `outgoing` Outbox (echo logic) and shreds the processed bytes from the Inbox.
* **`handle_write()`**: Triggered when the network pipe has room for outgoing data.
  * Tries to shove as much of the Outbox into the network as the OS allows.
  * If the OS pipe fills up midway, it stops. The remaining data stays in the Outbox for the next loop iteration.
  * Once the Outbox is entirely empty, it swaps the sticky note back to `want_read`.

### 3. The Event Loop
* **`fd2conn`**: A standard vector acting as a filing cabinet, mapping file descriptors to their respective `Conn` objects (managed safely via `std::unique_ptr`).
* **The `while(true)` Loop**: The beating heart of the server.
  * Prepares a checklist (`poll_args`) for the OS based on every client's `want_read`/`want_write` sticky notes.
  * Calls **`poll()`**: The server goes to sleep here. It does zero CPU work until the OS wakes it up to announce that a socket is ready.
  * Iterates through the returned checklist, routing ready sockets to `handle_accept`, `handle_read`, or `handle_write`.
  * Cleans up disconnected or errored clients by closing their file descriptors and deleting their `Conn` folders.