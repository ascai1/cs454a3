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

    char hostname[128] = {0};
    if (gethostname(hostname, 127) < 0) {
        perror("GetHostName:");
        close(soc);
        return -1;
    }

    sockaddr_in sin;
    socklen_t addr_len = sizeof(sockaddr_in);
    if (getsockname(soc, (sockaddr *)(&sin), &addr_len) < 0) {
        perror("GetSockName:");
        close(soc);
        return -1;
    }

    std::cout << "SERVER_ADDRESS " << hostname << std::endl;
    std::cout << "SERVER_PORT " << ntohs(sin.sin_port) << std::endl;

    return soc;
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
        ret = recv(soc, buf, size, 0);
        if (ret < 0) {
            perror("Read:");
        }
        return ret;
    }
    return -1;
}
