#include <iostream>

#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "socket.h"

#define SOCKET_QUEUE 32
#define SELECT_WAIT_SECS 0
#define SELECT_WAIT_MICROSECS 500000

int getClientSocket() {
    int soc = socket(AF_INET, SOCK_STREAM, 6);
    if (soc < 0) {
        perror("Socket: ");
        return -1;
    }

    addrinfo hints;
    addrinfo * addr;
    memset(&hints, 0, sizeof(addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 6; // TCP
    int gai = getaddrinfo(std::getenv("BINDER_ADDRESS"), std::getenv("BINDER_PORT"), &hints, &addr) ;
    if (gai != 0) {
        std::cerr << "GetAddrInfo failed, please check your environment variables BINDER_ADDRESS and BINDER_PORT" << std::endl;
        std::cerr << gai_strerror(gai) << std::endl;
        close(soc);
        return -1;
    }

    addrinfo * _addr;
    for (_addr = addr; _addr; _addr = _addr->ai_next) {
        if (connect(soc, _addr->ai_addr, _addr->ai_addrlen) >= 0) break;
    }
    if (!_addr)
        perror("Connect: ");
        close(soc);
        return -1;
    }
    freeaddrinfo(addr);

    return soc;
}

int getServerBinderSocket(sockaddr_in & binderAddr){
    int soc = socket(AF_INET, SOCK_STREAM, 6);
    if(soc < 0){
        perror("Socket: ");
        return -1;
    }

    addrinfo hints;
    addrinfo* addr;
    memset(&hints, 0, sizeof(addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 6; // TCP
    int gai = getaddrinfo(std::getenv("BINDER_ADDRESS"), std::getenv("BINDER_PORT"), &hints, &addr);
    if(gai != 0){
        std::cerr << "GetAddrInfo failed, please check your environment variables BINDER_ADDRESS and BINDER_PORT" << std::endl;
        std::cerr << gai_strerror(gai) << std::endl;
        close(soc);
        return -1;
    }

    addrinfo * _addr;
    for (_addr = addr; _addr; _addr = _addr->ai_next) {
        if (connect(soc, _addr->ai_addr, _addr->ai_addrlen) >= 0) {
            binderAddr = *(addr->ai_addr);
            break;
        }
    }
    if (!_addr)
        perror("Connect: ");
        close(soc);
        return -1;
    }
    freeaddrinfo(addr);

    return soc;
}

int getBinderSocket() {
    int soc = socket(AF_INET, SOCK_STREAM, 6);
    if (soc < 0) {
        perror("Socket: ");
        return -1;
    }

    sockaddr_in addr = {AF_INET, 0, INADDR_ANY};
    if (bind(soc, (sockaddr *)&addr, sizeof(sockaddr_in)) < 0) {
        perror("Bind: ");
        close(soc);
        return -1;
    }

    if (listen(soc, SOCKET_QUEUE) < 0) {
        perror("Listen: ");
        close(soc);
        return -1;
    }

    char hostname[128];
    if(getHost(hostname) < 0){
        close(soc);
        return -1;
    }

    if (getSockName(soc, &addr) < 0){
        close(soc);
        return -1;
    }   

    std::cout << "SERVER_ADDRESS " << hostname << std::endl;
    std::cout << "SERVER_PORT " << ntohs(addr.sin_port) << std::endl;

    return soc;
}

int getHost(char* hostname){
    if (gethostname(hostname, MAX_HOST_LENGTH) < 0) {
        perror("GetHostName:");
        return -1;
    }

    return 0;
}

int getSockName(int soc, sockaddr_in* addr){
    socklen_t addr_len = sizeof(sockaddr_in);
    if (getsockname(soc, (sockaddr *)(&addr), &addr_len) < 0) {
        perror("GetSockName:");
        return -1;
    }

    return 0;
}


int myselect(int soc) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(soc, &fds);
    timeval tv = {SELECT_WAIT_SECS, SELECT_WAIT_MICROSECS};

    int retval = select(soc + 1, &fds, NULL, NULL, &tv);
    if (retval < 0) {
        perror("Select:");
    }

    return retval;
}

int selectAndAccept(int soc) {
    int ret = myselect(soc);
    if (ret > 0) {
        ret = accept(soc, NULL, NULL);
        if (ret < 0) {
            perror("Accept:");
        }
    }

    return -1;
}

int selectAndRead(int soc, unsigned char * buf, unsigned int size) {
    int ret = myselect(soc);
    if (ret > 0) {
        return myread(soc, buf, size);
    }

    return -1;
}

int myread(int soc, unsigned char * buf, unsigned int size) {
    int ret = recv(soc, buf, size, 0);
    if (ret < 0) {
        perror("Read:");
    }

    return ret;
}
