#include <algorithm>
#include <iostream>
#include <string.h>
#include <unistd.h>

#include "rpc.h"
#include "packet.h"
#include "socket.h"
#include "exception.h"

using namespace std;

int rpcCall(char * name, int * argTypes, void ** args) {
    int soc = 0;
    int result = -1;
    unsigned char * packet = NULL;

    try {
        if (strlen(name) > MAX_NAME_LENGTH) {
            throw RpcException(NAME_TOO_LONG);
        }

        int argc = 0;
        for (int * at = argTypes; *at; at++) {
            argc++;
        }
    
        int binderPacketLength = CLIENT_LOC_MSG_ARGS + sizeof(int) * (argc + 1);
        int serverPacketLength = CLIENT_EXEC_MSG_ARGS + sizeof(int) * (argc + 1) + getTotalArgLength(argTypes);
        int length = std::max(binderPacketLength, serverPacketLength);

        std::cerr << getTotalArgLength(argTypes) << std::endl;
        std::cerr << "length: " << length << std::endl;

        packet = new unsigned char[MSG_HEADER_LEN + length];
        if (!packet) {
            throw RpcException(SIG_TOO_LONG);
        }

        soc = getClientBinderSocket(); 
        if (soc < 0) {
            throw RpcException(BAD_BINDER_SOCK);
        }

        clearPacket(packet);
        setPacketData(packet, CLIENT_LOC_MSG_NAME, name, std::min(MAX_NAME_LENGTH, (int)strlen(name)));
        for (int i = 0; i < argc; i++) {
            setPacketData(packet, CLIENT_LOC_MSG_ARGS + sizeof(int) * i, argTypes + i, sizeof(int));
        }

        int sendBytes = sendPacket(soc, packet, binderPacketLength, LOC_REQUEST);
        if (!sendBytes) {
            throw RpcException(BINDER_UNAVAILABLE);
        } else if (sendBytes < binderPacketLength) {
            throw RpcException(BAD_SEND_BIND);
        }

        unsigned char response[MSG_HEADER_LEN + BINDER_LOC_MSG_LEN];
        int readBytes = myread(soc, response, sizeof(response));
        if (!readBytes) {
            throw RpcException(BINDER_UNAVAILABLE);
        } else if (readBytes < sizeof(response)) {
            throw RpcException(BAD_RECV_BIND);
        }

        close(soc);

        char name[MAX_HOST_LENGTH + 1] = {0};
        char port[MAX_PORT_LENGTH + 1] = {0};
        getPacketData(response, BINDER_LOC_MSG_HOST, name, MAX_HOST_LENGTH);
        getPacketData(response, BINDER_LOC_MSG_PORT, port, MAX_PORT_LENGTH);
        soc = getClientServerSocket(name, port);
        if (soc < 0) {
            throw RpcException(BAD_SERVER_SOCK);
        }

        if (CLIENT_LOC_MSG_NAME != CLIENT_EXEC_MSG_NAME || CLIENT_LOC_MSG_ARGS != CLIENT_EXEC_MSG_ARGS) {
            clearPacket(packet);
            setPacketData(packet, CLIENT_EXEC_MSG_NAME, name, std::min(MAX_NAME_LENGTH, (int)strlen(name)));
            for (int i = 0; i < argc; i++) {
                setPacketData(packet, CLIENT_EXEC_MSG_ARGS + sizeof(int) * i, argTypes + i, sizeof(int));
            }
        }

        setPacketArgData(packet, CLIENT_EXEC_MSG_ARGS + sizeof(int) * (argc + 1), argTypes, args, ARG_INPUT);

        sendBytes = sendPacket(soc, packet, serverPacketLength, EXECUTE);
        if (!sendBytes) {
            throw RpcException(SERVER_UNAVAILABLE);
        } else if (sendBytes < binderPacketLength) {
            throw RpcException(BAD_SEND_SERVER);
        }
        
        readBytes = myread(soc, packet, length);
        if (!readBytes) {
            throw RpcException(SERVER_UNAVAILABLE);
        } else if (readBytes < serverPacketLength) {
            throw RpcException(BAD_RECV_SERVER);
        }

        int failCause = 0;
        getPacketData(packet, SERVER_EXEC_MSG_CAUSE, &failCause, sizeof(int));
        if (failCause) {
            throw RpcException(failCause);
        }

        getPacketData(packet, SERVER_EXEC_MSG_RESULT, &result, sizeof(int));
        if (!result) {
            getPacketArgData(packet, CLIENT_EXEC_MSG_ARGS + sizeof(int) * (argc + 1), argTypes, args, ARG_OUTPUT);
        }
    } catch (const RpcException e) {
        result = e.getErrorCode();
    }

  //  if (packet) delete[] packet;
    if (soc) close(soc);
    return result;
}

int rpcTerminate() {
    int soc = getClientBinderSocket(); 
    if (soc < 0) {
        return BAD_BINDER_SOCK;
    }
    unsigned char packet[MSG_HEADER_LEN];
    sendPacket(soc, packet, 0, TERMINATE);
    close(soc);
    return 0;
}
