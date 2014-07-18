#include <algorithm>
#include <iostream>
#include <map>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "packet.h"
#include "socket.h"
#include "keyval.h"

#define BUF_SIZE 256

typedef std::map<Key, std::queue<ServerID> > proc_m;

struct HandlerArgs {
    pthread_t id;
    int soc;
    proc_m & procMap;
    pthread_mutex_t * procMapMutex;
    bool & term;
    pthread_mutex_t * terminateMutex;

    HandlerArgs(int soc, proc_m & procMap, pthread_mutex_t * procMapMutex,
            bool & terminate, pthread_mutex_t * terminateMutex):
        id(0),
        soc(soc),
        procMap(procMap),
        procMapMutex(procMapMutex),
        term(terminate),
        terminateMutex(terminateMutex) {}

    bool hasTerminated() {
        pthread_mutex_lock(terminateMutex);
        bool _terminate = term;
        pthread_mutex_unlock(terminateMutex);
        return _terminate;
    }

    void terminate() {
        pthread_mutex_lock(terminateMutex);
        term = true;
        pthread_mutex_unlock(terminateMutex);
    }
};

void maskArgs(unsigned char * packet, unsigned int offset) {
    unsigned int * argTypes = (unsigned int *)(packet + offset);
    for (unsigned int * at = argTypes; *at; ++at) {
        unsigned int isArray = *at & ARG_ARR_LEN_MASK;
        *at = (*at & ~ARG_ARR_LEN_MASK) | (isArray ? 1 : 0);
    }
}

void registerProc(proc_m & procMap, pthread_mutex_t * procMapMutex, unsigned char * packet, int soc) {
    maskArgs(packet, SERVER_REG_MSG_ARGS);

    char name[MAX_NAME_LENGTH + 1] = {0};
    getPacketData(packet, SERVER_REG_MSG_NAME, name, MAX_NAME_LENGTH);
    int * argTypes = (int *)(packet + MSG_HEADER_LEN + CLIENT_LOC_MSG_ARGS);
    Key key(name, argTypes);

    char host[MAX_HOST_LENGTH + 1] = {0};
    getPacketData(packet, SERVER_REG_MSG_HOST, host, MAX_HOST_LENGTH);
    char port[MAX_PORT_LENGTH + 1] = {0};
    getPacketData(packet, SERVER_REG_MSG_PORT, port, MAX_PORT_LENGTH);
    ServerID id(host, port);

    pthread_mutex_lock(procMapMutex);
    procMap[key].push(id);
    pthread_mutex_unlock(procMapMutex);

    // Return fail if already registered
    unsigned char messageBuf[MSG_HEADER_LEN] = {0};
    sendPacket(soc, messageBuf, 0, REGISTER_SUCCESS);
}

void getProcLoc(proc_m & procMap, pthread_mutex_t * procMapMutex, unsigned char * packet, int soc) {
    maskArgs(packet, CLIENT_LOC_MSG_ARGS);

    unsigned char messageBuf[MSG_HEADER_LEN + BINDER_LOC_MSG_LEN] = {0};
    unsigned int status = LOC_FAILURE;
    unsigned int length = 1;

    char name[MAX_NAME_LENGTH + 1] = {0};
    getPacketData(packet, CLIENT_LOC_MSG_NAME, name, MAX_NAME_LENGTH);
    int * argTypes = (int *)(packet + MSG_HEADER_LEN + CLIENT_LOC_MSG_ARGS);
    Key key(name, argTypes);

    pthread_mutex_lock(procMapMutex);
    proc_m::iterator proc = procMap.find(key);
    if (proc != procMap.end()) {
        status = LOC_SUCCESS;
        length = BINDER_LOC_MSG_LEN;
        ServerID & host = proc->second.front();
        setPacketData(messageBuf, BINDER_LOC_MSG_HOST, host.getName(), MAX_HOST_LENGTH);
        setPacketData(messageBuf, BINDER_LOC_MSG_PORT, host.getPort(), MAX_PORT_LENGTH);
        proc->second.push(host);
        proc->second.pop();
    }
    pthread_mutex_unlock(procMapMutex);

    sendPacket(soc, messageBuf, length, status);
}

// What if send is unsuccessful due to other side closing? have to return a code
void process(unsigned char * packet, HandlerArgs * args) {
    unsigned int length, type;
    getPacketHeader(packet, length, type);

    switch (type) {
        case REGISTER: {
            registerProc(args->procMap, args->procMapMutex, packet, args->soc);
            break;
        } case LOC_REQUEST: {
            getProcLoc(args->procMap, args->procMapMutex, packet, args->soc);
            break;
        } case TERMINATE: {
            args->terminate();
            break;
        }
    }
}

void * handler(void * a) {
    HandlerArgs * args = (HandlerArgs *)a;

    while (!args->hasTerminated()) {
        unsigned char header[MSG_HEADER_LEN];
        unsigned int type = 0;
        unsigned int length = 0;
        unsigned char * packet = NULL;

        int readSize = selectAndRead(args->soc, header, MSG_HEADER_LEN);
        if (readSize == 0) {
            break;
        } else if (readSize != MSG_HEADER_LEN) {
            continue;
        }

        getPacketHeader(header, length, type);
        packet = new unsigned char[MSG_HEADER_LEN + length];
        if (!packet) {
            continue;
        }
        setPacketHeader(packet, length, type);

        for (unsigned int offset = 0; readSize > 0 && offset < length; offset += readSize) {
            readSize = myread(args->soc, packet + MSG_HEADER_LEN + offset, length - offset);
        }
        if (readSize <= 0) {
            continue;
        }

        process(packet, args);
        if (type == TERMINATE) break;
        delete[] packet;
    }
    close(args->soc);
}

int main() {
    proc_m procMap;
    pthread_mutex_t procMapMutex;
    pthread_mutex_init(&procMapMutex, NULL);

    //populateProcMap(procMap, PROC_FILE);

    int soc = getBinderSocket();
    if (soc < 0) {
        return -1;
    }

    bool terminate = false;
    pthread_mutex_t terminateMutex;
    pthread_mutex_init(&terminateMutex, NULL);

    std::vector<HandlerArgs *> threads;
    HandlerArgs mainArgs(soc, procMap, &procMapMutex, terminate, &terminateMutex);

    while (!mainArgs.hasTerminated()) {
        int connSoc = selectAndAccept(soc);
        if (connSoc < 0) {
            continue;
        }

        HandlerArgs * args = new HandlerArgs(connSoc, procMap, &procMapMutex, terminate, &terminateMutex);

        if (pthread_create(&args->id, NULL, handler, args) != 0) {
            std::cerr << "Failed to create thread" << std::endl;
            close(args->id);
            delete args;
        } else {
            threads.push_back(args);
        }
    }

    for (std::vector<HandlerArgs *>::iterator it = threads.begin(); it != threads.end(); it++) {
        pthread_join((*it)->id, NULL);
        delete *it;
    }

    pthread_mutex_destroy(&procMapMutex);
    pthread_mutex_destroy(&terminateMutex);
    close(soc);
    return 0;
}

