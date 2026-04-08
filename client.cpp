#include <iostream>
#include <string>
#include <vector>
#include <string_view>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cerrno>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

static void msg(const std::string_view& msg) {
    std::cerr << msg << '\n';
}

static void die(const std::string_view& msg) {
    int err = errno;
    std::cerr << "[" << err << "] " << msg << '\n';
    std::abort();
}

static int32_t read_full(int fd, char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = ::read(fd, buf, n);
        if (rv <= 0) {
            return -1;  // error, or unexpected EOF
        }
        assert(static_cast<size_t>(rv) <= n);
        n -= static_cast<size_t>(rv);
        buf += rv;
    }
    return 0;
}

static int32_t write_all(int fd, const char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = ::write(fd, buf, n);
        if (rv <= 0) {
            return -1;  // error
        }
        assert(static_cast<size_t>(rv) <= n);
        n -= static_cast<size_t>(rv);
        buf += rv;
    }
    return 0;
}

constexpr size_t k_max_msg = 4096;

static int32_t send_req(int fd, const std::vector<std::string> &cmd) {
    uint32_t len = 4;
    for (const std::string &s : cmd) {
        len += 4 + static_cast<uint32_t>(s.size());
    }
    if (len > k_max_msg) {
        return -1;
    }

    char wbuf[4 + k_max_msg];
    std::memcpy(&wbuf[0], &len, 4);  // assume little endian
    
    uint32_t n = static_cast<uint32_t>(cmd.size());
    std::memcpy(&wbuf[4], &n, 4);
    
    size_t cur = 8;
    for (const std::string &s : cmd) {
        uint32_t p = static_cast<uint32_t>(s.size());
        std::memcpy(&wbuf[cur], &p, 4);
        std::memcpy(&wbuf[cur + 4], s.data(), s.size());
        cur += 4 + s.size();
    }
    
    return write_all(fd, wbuf, 4 + len);
}

static int32_t read_res(int fd) {
    // 4 bytes header
    char rbuf[4 + k_max_msg];
    errno = 0;
    int32_t err = read_full(fd, rbuf, 4);
    if (err) {
        if (errno == 0) {
            msg("EOF");
        } else {
            msg("read() error");
        }
        return err;
    }

    uint32_t len = 0;
    std::memcpy(&len, rbuf, 4);  // assume little endian
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }

    // reply body
    err = read_full(fd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }

    // print the result
    uint32_t rescode = 0;
    if (len < 4) {
        msg("bad response");
        return -1;
    }
    std::memcpy(&rescode, &rbuf[4], 4);
    
    // Safely print the payload without relying on null-terminators
    std::string_view reply_text(&rbuf[8], len - 4);
    std::cout << "server says: [" << rescode << "] " << reply_text << '\n';
    
    return 0;
}

int main(int argc, char **argv) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234); // Changed ntohs to htons (Host to Network Short)
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  // 127.0.0.1
    
    int rv = ::connect(fd, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
    if (rv) {
        die("connect");
    }

    // Parse command line arguments into a C++ vector
    std::vector<std::string> cmd;
    for (int i = 1; i < argc; ++i) {
        cmd.push_back(argv[i]);
    }

    int32_t err = send_req(fd, cmd);
    if (err) {
        goto L_DONE;
    }
    
    err = read_res(fd);
    if (err) {
        goto L_DONE;
    }

L_DONE:
    ::close(fd);
    return 0;
}