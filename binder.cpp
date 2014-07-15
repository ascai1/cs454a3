#include <algorithm>
#include <iostream>
#include <map>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "socket.h"

#define MAX_HOST_LENGTH 64
#define MAX_PORT_LENGTH 8
#define MAX_NAME_LENGTH 64

#define REGISTER 0x10
#define REGISTER_SUCCESS 0x11
#define REGISTER_FAILURE 0x12

#define LOC_REQUEST 0x20
#define LOC_SUCCESS 0x21
#define LOC_FAILURE 0x22

#define TERMINATE 0xff

#define MSG_LENGTH 0
#define MSG_TYPE 4
#define MSG_START 8
#define MSG_REG_HOST MSG_START
#define MSG_REG_PORT (MSG_REG_HOST + MAX_HOST_LENGTH)
#define MSG_REG_NAME (MSG_REG_PORT + MAX_PORT_LENGTH)
#define MSG_REG_ARGS (MSG_REG_NAME + MAX_NAME_LENGTH)

#define BUF_SIZE 256
#define SELECT_WAIT_SECS 0
#define SELECT_WAIT_MICROSECS 500000

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
    bool & terminate;
    pthread_mutex_t * terminateMutex;
    HandlerArgs(int soc, proc_m & procMap, pthread_mutex_t * procMapMutex,
            bool & terminate, pthread_mutex_t * terminateMutex):
        id(0),
        soc(soc),
        procMap(procMap),
        procMapMutex(procMapMutex),
        terminate(terminate),
        terminateMutex(terminateMutex) {}
};


void registerProc(proc_m & procMap, pthread_mutex_t * procMapMutex, char_v & message, int soc) {
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

    unsigned char messageBuf[MSG_START + 1] = {0};
    unsigned int length = 1;
    unsigned int status = REGISTER_SUCCESS;
    memcpy(messageBuf + MSG_LENGTH, &length, sizeof(unsigned int));
    memcpy(messageBuf + MSG_TYPE, &status, sizeof(unsigned int));
    send(soc, messageBuf, MSG_START + 1, 0);
}

void getProcLoc(proc_m & procMap, pthread_mutex_t * procMapMutex, char_v & message, int soc) {
    unsigned char messageBuf[MSG_START + MAX_HOST_LENGTH + MAX_NAME_LENGTH] = {0};
    unsigned int status = LOC_FAILURE;
    unsigned int length = 1;

    pthread_mutex_lock(procMapMutex);
    proc_m::iterator proc = procMap.find(message);
    if (proc != procMap.end()) {
        status = LOC_SUCCESS;
        length = MAX_HOST_LENGTH + MAX_NAME_LENGTH;
        char_v & host = proc->second.front();
        std::copy(host.begin(), host.end(), messageBuf + 1);
        proc->second.push(host);
        proc->second.pop();
    }
    pthread_mutex_unlock(procMapMutex);

    memcpy(messageBuf + MSG_LENGTH, &length, sizeof(unsigned int));
    memcpy(messageBuf + MSG_TYPE, &status, sizeof(unsigned int));
    send(soc, messageBuf, MSG_START + length, 0);
}

void terminate(bool & terminate, pthread_mutex_t * terminateMutex) {
    pthread_mutex_lock(terminateMutex);
    terminate = true;
    pthread_mutex_unlock(terminateMutex);
}

void process(unsigned char type, char_v & message, HandlerArgs * args) {
    switch (type) {
        case REGISTER: {
            registerProc(args->procMap, args->procMapMutex, message, args->soc);
            break;
        } case LOC_REQUEST: {
            getProcLoc(args->procMap, args->procMapMutex, message, args->soc);
            break;
        } case TERMINATE: {
            terminate(args->terminate, args->terminateMutex);
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

    while (1) {
        pthread_mutex_lock(args->terminateMutex);
        bool _terminate = args->terminate;
        pthread_mutex_unlock(args->terminateMutex);
        if (_terminate) break;

		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(args->soc, &fds);
		timeval tv = {SELECT_WAIT_SECS, SELECT_WAIT_MICROSECS};

		int retval = select(args->soc + 1, &fds, NULL, NULL, &tv);
		if (retval < 0) {
			perror("Select:");
			continue;
		} else if (retval == 0) {
			continue;
		}

        int readSize = recv(args->soc, buf, BUF_SIZE, 0);
        if (!readSize) break;

        int offset = 0;
        if (readSize >= MSG_START && !type && !length) {
            memcpy(&length, buf + MSG_LENGTH, sizeof(unsigned int));
            memcpy(&type, buf + MSG_TYPE, sizeof(unsigned int));
            offset = MSG_START;
        }
        for (unsigned char * _buf = buf + offset; _buf - buf < readSize; _buf++) {
            message.push_back(*_buf);
        }
        if (message.size() >= length) {
            process(type, message, args);
            message.clear();
            if (type == TERMINATE) break;
            type = 0;
        }
    }
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

    while (1) {
        pthread_mutex_lock(&terminateMutex);
        bool _terminate = terminate;
        pthread_mutex_unlock(&terminateMutex);
        if (_terminate) break;

		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(soc, &fds);
		timeval tv = {SELECT_WAIT_SECS, SELECT_WAIT_MICROSECS};

		int retval = select(soc + 1, &fds, NULL, NULL, &tv);
		if (retval < 0) {
			perror("Select:");
			continue;
		} else if (retval == 0) {
			continue;
		}

        int connSoc = accept(soc, NULL, NULL);
		if (connSoc < 0) {
			perror("Accept:");
			continue;
		}

        HandlerArgs * args = new HandlerArgs(connSoc, procMap, &procMapMutex,
                terminate, &terminateMutex);

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

