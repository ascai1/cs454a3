#include <iostream>

#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "socket.h"

#define SOCKET_QUEUE 32

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
