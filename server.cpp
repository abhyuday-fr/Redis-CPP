#include <iostream>
#include <cstdlib>
#include <string>
#include <cerrno>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

static void msg(const std::string& msg) {
    std::cerr << msg << '\n';
}

static void die(const std::string& msg) {
    int err = errno;
    std::cerr << "[" << err << "] " << msg << '\n';
    std::abort();
}

static void do_something(int connfd) {
    char rbuf[64] = {};
    ssize_t n = ::read(connfd, rbuf, sizeof(rbuf) - 1);
    if (n < 0) {
        msg("read() error");
        return;
    }
    
    std::cerr << "client says: " << rbuf << '\n';
    
    std::string wbuf = "world";
    ::write(connfd, wbuf.data(), wbuf.length());
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

        do_something(connfd);
        ::close(connfd);
    }

    return 0;
}