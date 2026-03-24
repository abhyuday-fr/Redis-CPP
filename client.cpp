#include <iostream>
#include <cstdlib>
#include <string>
#include <cerrno>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

static void die(const std::string& msg){
    int err = errno;
    std::cerr << "[" << err << "] " << msg << '\n';
    std::abort();
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

    std::string msg = "hello";
    ::write(fd, msg.data(), msg.length());

    char rbuf[64] = {};
    ssize_t n = ::read(fd, rbuf, sizeof(rbuf) - 1);
    if(n < 0){
        die("read");
    }

    std::cout << "server says: " << rbuf << '\n';

    ::close(fd);

    return 0;
}