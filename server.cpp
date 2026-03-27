#include <iostream>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cerrno>
#include <cassert>
#include <string_view>

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

constexpr size_t k_max_msg = 4096;

static int32_t read_full(int fd, char *buf, size_t n){
    while(n > 0){
        ssize_t rv = ::read(fd, buf, n);
        if(rv <= 0){
            return -1;
        }
        assert(static_cast<size_t>(rv) <= n);
        n -= static_cast<size_t>(rv);
        buf += rv;
    }
    return 0;
}

static int32_t write_all(int fd, const char *buf, size_t n){
    while(n > 0){
        ssize_t rv = ::write(fd, buf, n);
        if(rv <= 0){
            return -1;
        }
        assert(static_cast<size_t>(rv) <= n);
        n -= static_cast<size_t>(rv);
        buf += rv;
    }
    return 0;
}

static int32_t one_request(int connfd){
    // 4 bytes header
    char rbuf[4 + k_max_msg];
    errno = 0;
    int32_t err = read_full(connfd, rbuf, 4);
    if(err){
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }

    uint32_t len = 0;
    std::memcpy(&len, rbuf, 4); // assume little endian
    if(len > k_max_msg){
        msg("too long");
        return -1;
    }

    // request body
    err = read_full(connfd, &rbuf[4], len);
    if(err){
        msg("read() error");
        return err;
    }

    // do something
    std::cerr << "client says: " << std::string_view(&rbuf[4], len) << '\n';

    // reply using the same protocol
    constexpr std::string_view reply = "world";
    constexpr size_t reply_len = reply.length();

    char wbuf[4 + reply_len];

    len = static_cast<uint32_t>(reply_len);
    std::memcpy(wbuf, &len, 4);
    std::memcpy(&wbuf[4], reply.data(), len);

    return write_all(connfd, wbuf, 4 + len);
}

int main(){
    
    std::cout << "Server is Running\n";

    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0){
        die("socket()");
    }

    // this is needed for most server applications
    int val = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // bind
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(0); // wildcard address 0.0.0.0

    int rv = ::bind(fd, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
    if(rv){
        die("bind()");
    }

    // listen
    rv = ::listen(fd, SOMAXCONN);
    if(rv){
        die("listen()");
    }
    
    while(true){
        // accept
        sockaddr_in client_addr = {};
        socklen_t addrlen = sizeof(client_addr);

        int connfd = ::accept(fd, reinterpret_cast<sockaddr*>(&client_addr), &addrlen);
        if(connfd < 0){
            continue; //error
        }

        while(true){
            // here the server serves only one client connection once
            int32_t err = one_request(connfd);
            if(err){
                break;
            }
        }
        ::close(connfd);
    }

    return 0;
}