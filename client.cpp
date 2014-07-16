#include "rpc.h"
#include "packet.h"
#include "socket.h"
#include "exception.h"


int rpcCall(char * name, int * argTypes, void ** args) {
    int argc = 0;
    for (int * at = argTypes; *at; at++) {
        argc++;
    }

    int soc = 0;
    int binderPacketLength = CLIENT_LOC_MSG_ARGS + sizeof(int) * argc;
    int serverPacketLength = CLIENT_EXEC_MSG_ARGS + sizeof(int) * argc + getTotalArgLength(argTypes);
    unsigned char * packet = NULL;

    try {
        soc = getClientBindSoc(); 
        if (soc < 0) {
            throw RpcException(BAD_BINDER_SOCK);
        }

        packet = new unsigned char[MSG_HEADER_LEN + std::max(binderPacketLength, serverPacketLength)];
        if (!packet) {
            throw RpcException(SIG_TOO_LONG);
        }

        clearPacket(packet);
        setPacketData(packet, CLIENT_LOC_MSG_NAME, name, std::min(MAX_NAME_LENGTH, strlen(name)));
        for (int i = 0; i < argc; i++) {
            setPacketData(packet, CLIENT_LOC_MSG_ARGS + sizeof(int) * i, argTypes + i, sizeof(int));
        }

        int sendBytes = sendPacket(soc, packet, binderPacketLength, LOC_REQUEST);
        if (!sendBytes) {
            throw RpcException(BINDER_UNAVAILABLE);
        } else if (sendBytes < binderPacketLength) {
            throw RpcException(BAD_SEND_BIND);
        }

        int readBytes;
        { 
            unsigned char response[MSG_HEADER_LEN + BINDER_LOC_MSG_LEN];
            readBytes = myread(soc, response, sizeof(response));
            if (!readBytes) {
                throw RpcException(BINDER_UNAVAILABLE);
            } else if (readBytes < sizeof(response)) {
                throw RpcException(BAD_RECV_BIND);
            }

            close(soc);

            unsigned char name[MAX_HOST_LENGTH + 1] = {0};
            unsigned char port[MAX_PORT_LENGTH + 1] = {0};
            getPacketData(response, BINDER_LOC_MSG_HOST, name, MAX_HOST_LENGTH);
            getPacketData(response, BINDER_LOC_MSG_PORT, name, MAX_PORT_LENGTH);
            soc = getClientServerSoc(name, port);
            if (soc < 0) {
                throw RpcException(BAD_SERVER_SOCK);
            }
        }

        if (CLIENT_LOC_MSG_NAME != CLIENT_EXEC_MSG_NAME) {
            setPacketData(packet, CLIENT_EXEC_MSG_NAME, name, std::min(MAX_NAME_LENGTH, strlen(name)));
        }
        if (CLIENT_LOC_MSG_ARGS != CLIENT_EXEC_MSG_ARGS) {
            for (int i = 0; i < argc; i++) {
                setPacketData(packet, CLIENT_EXEC_MSG_ARGS + sizeof(int) * i, argTypes + i, sizeof(int));
            }
        }

        setPacketArgData(packet, CLIENT_EXEC_MSG_ARGS + sizeof(int) * argc, argTypes, args);

        sendBytes = sendPacket(soc, packet, serverPacketLength, EXECUTE);
        if (!sendBytes) {
            throw RpcException(SERVER_UNAVAILABLE);
        } else if (sendBytes < binderPacketLength) {
            throw RpcException(BAD_SEND_SERVER);
        }
        
        readBytes = myread(soc, packet, sizeof(response));
        if (!readBytes) {
            throw RpcException(BINDER_UNAVAILABLE);
        } else if (readBytes < serverPacketLength) {
            throw RpcException(BAD_RECV_BIND);
        }

        getPacketArgData(packet, CLIENT_EXEC_MSG_ARGS + sizeof(int) * argc, argTypes, args);
        delete[] packet;
        close(soc);
        return 0;
    } catch (const RpcException e) {
        std::cerr << "rpcCall: " << e.getException() << std::endl;
        if (packet) delete[] packet;
        if (soc) close(soc);
        return e.getErrorCode();
    }
}

int rpcTerminate() {
    int soc = getClientBindSoc(); 
    if (soc < 0) {
        return BAD_BINDER_SOC;
    }
    unsigned char packet[MSG_HEADER_LEN];
    sendPacket(soc, packet, 0, TERMINATE);
    close(soc);
    return 0;
}
