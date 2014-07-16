#include <algorithm>
#include <iostream>
#include <map>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "packet.h"
#include "socket.h"

#define BUF_SIZE 256

typedef std::vector<unsigned char> char_v;
typedef std::queue<char_v> host_v;

struct procMapComp {
    bool operator()(const char_v & a, const char_v & b) {
        if (a.size() != b.size()) return a.size() < b.size();
        std::pair<char_v::const_iterator, char_v::const_iterator> mismatch = std::mismatch(a.begin(), a.end(), b.begin());
        return *(mismatch.first) < *(mismatch.second);
    }
};

typedef std::map<char_v, host_v, procMapComp> proc_m;

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

void maskArgs(char_v & message, unsigned int offset) {
    for (; offset + ARG_LENGTH <= message.size(); offset += ARG_LENGTH) {
        message[offset + 2] = 0;
        message[offset + 3] = 0;
    }
}

void registerProc(proc_m & procMap, pthread_mutex_t * procMapMutex, char_v & message, int soc) {
    maskArgs(message, SERVER_REG_MSG_ARGS);

    char_v key;
    char_v val;

    char_v::const_iterator begin = message.begin();
    char_v::const_iterator end = message.end();
    for (char_v::const_iterator it = begin; it != end; it++) {
        if (it - begin >= MAX_HOST_LENGTH + MAX_PORT_LENGTH) {
            val.push_back(*it);
        } else {
            key.push_back(*it);
        }
    }

    pthread_mutex_lock(procMapMutex);
    procMap[key].push(val);
    pthread_mutex_unlock(procMapMutex);

    unsigned char messageBuf[MSG_HEADER_LEN + 1] = {0};
    sendPacket(soc, messageBuf, 1, REGISTER_SUCCESS);
}

void getProcLoc(proc_m & procMap, pthread_mutex_t * procMapMutex, char_v & message, int soc) {
    maskArgs(message, CLIENT_LOC_MSG_ARGS);

    unsigned char messageBuf[MSG_HEADER_LEN + BINDER_LOC_MSG_LEN] = {0};
    unsigned int status = LOC_FAILURE;
    unsigned int length = 1;

    pthread_mutex_lock(procMapMutex);
    proc_m::iterator proc = procMap.find(message);
    if (proc != procMap.end()) {
        status = LOC_SUCCESS;
        length = BINDER_LOC_MSG_LEN;
        char_v & host = proc->second.front();
        std::copy(host.begin(), host.end(), messageBuf + 1);
        proc->second.push(host);
        proc->second.pop();
    }
    pthread_mutex_unlock(procMapMutex);

    sendPacket(soc, messageBuf, length, status);
}

// What if send is unsuccessful due to other side closing? have to return a code
void process(unsigned char type, char_v & message, HandlerArgs * args) {
    switch (type) {
        case REGISTER: {
            registerProc(args->procMap, args->procMapMutex, message, args->soc);
            break;
        } case LOC_REQUEST: {
            getProcLoc(args->procMap, args->procMapMutex, message, args->soc);
            break;
        } case TERMINATE: {
            args->terminate();
            break;
        }
    }
}

void * handler(void * a) {
    HandlerArgs * args = (HandlerArgs *)a;
    char_v message;
    unsigned char buf[BUF_SIZE];
    unsigned int type = 0;
    unsigned int length = 0;

    while (!args->hasTerminated()) {
        int readSize = selectAndRead(args->soc, buf, BUF_SIZE);
        if (readSize < 0) {
            continue;
        } else if (readSize == 0) {
            break;
        }

        // This code assumes that the header will always arrive intact
        int offset = 0;
        if (readSize >= MSG_HEADER_LEN && !type && !length) {
            getPacketHeader(buf, length, type);
            offset = MSG_HEADER_LEN;
        }
        for (unsigned char * _buf = buf + offset; _buf - buf < readSize; _buf++) {
            message.push_back(*_buf);
        }
        if (message.size() >= length) {
            process(type, message, args);
            message.clear();
            if (type == TERMINATE) break;
            type = 0;
            length = 0;
        }
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

