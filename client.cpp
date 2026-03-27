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

static void msg(const std::string_view& msg){
    std::cerr << msg << '\n';
}

static void die(const std::string_view& msg){
    int err = errno;
    std::cerr << "[" << err << "] " << msg << '\n';
    std::abort();
}

static int32_t read_full(int fd, char *buf, size_t n){
    while(n > 0){
        ssize_t rv = ::read(fd, buf, n);
        if(rv <= 0){
            return -1; // error, EOF
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
            return -1; // error
        }
        assert(static_cast<size_t>(rv) <= n);
        n -= static_cast<size_t>(rv);
        buf += rv;

    }
    return 0;
}

constexpr size_t k_max_msg = 4096;

static int32_t query(int fd, std::string_view text){
    uint32_t len = static_cast<uint32_t>(text.length());
    if(len > k_max_msg){
        return -1;
    }

    char wbuf[4 + k_max_msg];
    std::memcpy(wbuf, &len, 4); // assume little endian
    std::memcpy(&wbuf[4], text.data(), len);
    if(int32_t err = write_all(fd, wbuf, 4 + len)){
        return err;
    }

    // 4 bytes header
    char rbuf[4 + k_max_msg];
    errno = 0;
    int32_t err = read_full(fd, rbuf, 4);
    if(err){
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }

    std::memcpy(&len, rbuf, 4); // assume little endian
    if(len > k_max_msg){
        msg("too long");
        return -1;
    }

    // reply body
    err = read_full(fd, &rbuf[4], len);
    if(err){
        msg("read() error");
        return err;
    }

    // do something
    std::cout << "server says: " << std::string_view(&rbuf[4], len) << '\n';

    return 0;

}

int main(){
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0){
        die("socket()");
    }

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK); // 127.0.0.1

    int rv = ::connect(fd, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
    if(rv){
        die("connect");
    }

    // multiple requests
    int32_t err = query(fd, "hello1");
    if(err){
        goto L_DONE;
    }

    err = query(fd, "hello2");
    if(err){
        goto L_DONE;
    }

    err = query(fd, "hello3");
    if(err){
        goto L_DONE;
    }

    L_DONE:
        ::close(fd);
    
    return 0;
}