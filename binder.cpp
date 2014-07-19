#include <algorithm>
#include <iostream>
#include <map>
#include <queue>
#include <set>
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

using namespace std;

// Map method signature to hosts (round robin)
typedef std::map<Key, std::queue<ServerID> > SigServ;
// Map host to method signatures (for deduping and some server status checking)
typedef std::map<ServerID, std::set<Key> > ServSig;
struct proc_m {
    SigServ sigToServ;
    ServSig servToSig;
};

// Variables passed into each thread
struct HandlerArgs {
    pthread_t id;
    int soc;
    proc_m & procMap;
    pthread_mutex_t * procMapMutex;
    bool & term;
    pthread_mutex_t * terminateMutex;
    ServerID hostID;

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

// Given a received REGISTER packet, register the host with the given method signature
// and send a response to socket "soc". Also place the hostname and port into "hostID".
void registerProc(proc_m & procMap, pthread_mutex_t * procMapMutex, unsigned char * packet, int soc, ServerID & hostID) {
    unsigned char messageBuf[MSG_HEADER_LEN] = {0};
    unsigned int status = REGISTER_FAILURE;

    char name[MAX_NAME_LENGTH + 1] = {0};
    getPacketData(packet, SERVER_REG_MSG_NAME, name, MAX_NAME_LENGTH);
    int * argTypes = (int *)(packet + MSG_HEADER_LEN + SERVER_REG_MSG_ARGS);
    Key key(name, argTypes);

    char host[MAX_HOST_LENGTH + 1] = {0};
    getPacketData(packet, SERVER_REG_MSG_HOST, host, MAX_HOST_LENGTH);
    char port[MAX_PORT_LENGTH + 1] = {0};
    getPacketData(packet, SERVER_REG_MSG_PORT, port, MAX_PORT_LENGTH);
    ServerID id(host, port);

    pthread_mutex_lock(procMapMutex);
    if (procMap.servToSig[id].insert(key).second) {     // True if redundant map insertion was successful
        status = REGISTER_SUCCESS;
        hostID = id;
        procMap.sigToServ[key].push(id);    // Push into main map
    }
    pthread_mutex_unlock(procMapMutex);

    sendPacket(soc, messageBuf, 0, status);
}

// Given a received LOC_REQUEST packet, find an appropriate host
// for the requested method and send a response to socket "soc".
void getProcLoc(proc_m & procMap, pthread_mutex_t * procMapMutex, unsigned char * packet, int soc) {
    unsigned char messageBuf[MSG_HEADER_LEN + BINDER_LOC_MSG_LEN] = {0};
    unsigned int status = LOC_FAILURE;
    unsigned int length = 0;

    char name[MAX_NAME_LENGTH + 1] = {0};
    getPacketData(packet, CLIENT_LOC_MSG_NAME, name, MAX_NAME_LENGTH);
    int * argTypes = (int *)(packet + MSG_HEADER_LEN + CLIENT_LOC_MSG_ARGS);
    Key key(name, argTypes);

    pthread_mutex_lock(procMapMutex);
    SigServ::iterator proc = procMap.sigToServ.find(key);

    // Find the given method in the map, and look for an appropriate host
    while (proc != procMap.sigToServ.end() && !proc->second.empty()) {
        ServerID host(proc->second.front());
        proc->second.pop();

        ServSig::iterator servSigIt = procMap.servToSig.find(host);
        // Has the server gone down or unregistered this method?
        if (servSigIt != procMap.servToSig.end() && servSigIt->second.count(key)) {
            status = LOC_SUCCESS;
            length = BINDER_LOC_MSG_LEN;
            setPacketData(messageBuf, BINDER_LOC_MSG_HOST, host.getName(), MAX_HOST_LENGTH);
            setPacketData(messageBuf, BINDER_LOC_MSG_PORT, host.getPort(), MAX_PORT_LENGTH);
            proc->second.push(host);
            break;
        }
    }

    pthread_mutex_unlock(procMapMutex);

    sendPacket(soc, messageBuf, length, status);
}

// Handle an incoming packet differently based on packet type
void process(unsigned char * packet, HandlerArgs * args) {
    unsigned int length, type;
    getPacketHeader(packet, length, type);

    switch (type) {
        case REGISTER: {
            registerProc(args->procMap, args->procMapMutex, packet, args->soc, args->hostID);
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
    unsigned char header[MSG_HEADER_LEN];
    unsigned char * packet = NULL;
    unsigned int type = 0;
    unsigned int length = 0;

    while (!args->hasTerminated()) {
        int readSize = selectAndRead(args->soc, header, MSG_HEADER_LEN);
        if (readSize == -1) {       // This means that "select" returned zero
            continue;
        } else if (readSize != MSG_HEADER_LEN) {    // Socket is closed, or something went wrong
            break;
        }

        getPacketHeader(header, length, type);      // Retrieve header length and type
        packet = new unsigned char[MSG_HEADER_LEN + length];
        if (!packet) {
            continue;
        }
        setPacketHeader(packet, length, type);

        // Loop until the entire packet is received
        for (unsigned int offset = 0; readSize > 0 && offset < length; offset += readSize) {
            readSize = myread(args->soc, packet + MSG_HEADER_LEN + offset, length - offset);
        }
        if (readSize <= 0) {
            break;
        }

        process(packet, args);
        delete[] packet;
        packet = NULL;
    }

    if (packet) delete[] packet;

    // Server is going down, unregister this host from the redundant map
    pthread_mutex_lock(args->procMapMutex);
    args->procMap.servToSig.erase(args->hostID);
    pthread_mutex_unlock(args->procMapMutex);

    // Send terminate request to server (or client)
    sendPacket(args->soc, header, 0, TERMINATE);
    close(args->soc);
    return NULL;
}

int main() {
    proc_m procMap;
    pthread_mutex_t procMapMutex;
    pthread_mutex_init(&procMapMutex, NULL);

    int soc = getBinderSocket();
    if (soc < 0) {
        return -1;
    }

    bool terminate = false;
    pthread_mutex_t terminateMutex;
    pthread_mutex_init(&terminateMutex, NULL);

    // This is just for checking "terminate" in a thread-safe way
    HandlerArgs mainArgs(soc, procMap, &procMapMutex, terminate, &terminateMutex);
    std::vector<HandlerArgs *> threads;

    while (!mainArgs.hasTerminated()) {
        int connSoc = selectAndAccept(soc);
        if (connSoc == 0) {         // "select" returned zero
            continue;
        } else if (connSoc < 0) {   // Something went wrong
            break;
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

    // Join all threads before closing main thread
    for (std::vector<HandlerArgs *>::iterator it = threads.begin(); it != threads.end(); it++) {
        pthread_join((*it)->id, NULL);
        delete *it;
    }

    pthread_mutex_destroy(&procMapMutex);
    pthread_mutex_destroy(&terminateMutex);
    close(soc);
    return 0;
}

