#include "rpc.h"
#include "packet.h"
#include "socket.h"
#include "exception.h"

#define BUF_SIZE 256

using namespace std;

typedef int (*skeleton)(int*, void**);

int bindSocket = 0;
int clientSocket = 0;
sockaddr_in binderAddr;

struct Key{
    string name;
    vector<unsigned int> argTypes;

    bool operator< (const Key& otherkey){
        if(name != otherkey.name){
            return name < otherkey.name;
        }

        if (argTypes.size() != otherKey.argTypes.size()) {
            return argTypes.size() < otherKey.argTypes.size();
        }
       
        pair<vector<unsigned int>::iterator, vector<unsigned int>::iterator> mis
            = mismatch(argTypes.begin(), argTypes.end(), otherkey.argTypes.begin());
        return *(mis.first) < *(mis.second);
    }

    Key(char* name, int* argTypes):
    name(name)
    {
        for(int* at = argTypes; *at; at++){
            argTypes.push_back(*at);
        }
    }
};

std::map<Key, skeleton> serverSkeletonMap;

int rpcInit(){
    bindSocket = getServerBinderSocket(binderAddr);
    clientSocket = getServerClientSocket();

    if(bindSocket == 0 && clientSocket == 0){
        return 0;
    }
    else{
        return SOCKETS_NOT_INITIALIZED;
    }
}

int rpcRegister(char* name, int* argTypes, skeleton f){
    unsigned char* packet = NULL;

    try{
        if (strlen(name) > MAX_NAME_LENGTH) {
            throw RpcException(NAME_TOO_LONG);
        }

        if(bindSocket != 0 || clientSocket != 0){
            throw RpcException(SOCKETS_NOT_INITIALIZED);
        }

        int argc = 0;
        for (int* at = argTypes; *at; at++) {
            argc++;
        }  

        int registerPacketLength = SERVER_REG_MSG_ARGS + sizeof(int) * argc;
        packet = new unsigned char[MSG_HEADER_LEN + registerPacketLength];
        if(!packet){
            throw RpcException(SIG_TOO_LONG);
        }

        clearPacket(packet);
        setPacketData(packet, SERVER_REG_MSG_NAME, name, std::min(MAX_NAME_LENGTH, strlen(name)));
        for(int i = 0; i < argc; i++){
            setPacketData(packet, SERVER_REG_MSG_ARGS + sizeof(int) * i, argTypes + i, sizeof(int));
        }

        char hostname[MAX_HOST_LENGTH];
        if(getHost(hostname) < 0){
            throw RpcException(NO_HOST_NAME);
        }

        sockaddr_in addr;
        if(getSockName(bindSocket, &addr)){
            throw RpcException(NO_PORT_NUMBER);
        }

        setPacketData(packet, SERVER_REG_MSG_HOST, hostname, MAX_HOST_LENGTH);
        setPacketData(packet, SERVER_REG_MSG_PORT, &addr.sin_port, sizeof(unsigned short));

        int sendBytes = sendPacket(bindSocket, packet, registerPacketLength, REGISTER);
        if (!sendBytes) {
            throw RpcException(BINDER_UNAVAILABLE);
        } else if (sendBytes < registerPacketLength) {
            throw RpcException(BAD_SEND_BIND);
        }

        unsigned char response[MSG_HEADER_LEN];
        int readBytes = myread(bindSocket, response, sizeof(response));
        if (!readBytes) {
            throw RpcException(BINDER_UNAVAILABLE);
        } else if (readBytes < sizeof(response)) {
            throw RpcException(BAD_RECV_BIND);
        }        
   
        Key key(name, argTypes);
        serverSkeletonMap[key] = f;
    }
    catch{
        std::cerr << "rpcRegister: " << e.getException() << std::endl;
        if (registerPacket) delete[] packet;
       // if (bindSocket) close(bindSocket);
        return e.getErrorCode();        
    }

    return 0;
}

int rpcExecute(){
    if (socketSkeletonMap.empty()) {
        return NO_METHODS_REGISTERED;
    }
    if (bindSocket != 0 || clientSocket != 0){
        return SOCKETS_NOT_INITIALIZED;
    }

    while(true){
        unsigned char* packet = NULL;
        int selectedSocket = 0;
        int readSocket = 0;

        try {
            unsigned char header[MSG_HEADER_LEN];
            unsigned int length = 0;
            unsigned int type = 0;   

            sockaddr_in addr;
            socklen_t socklen = sizeof(sockaddr_in);

            int socs[2] = {bindSocket, clientSocket};
            selectedSocket = myselect(socs, 2, NULL);
            readSocket = selectedSocket;
            if (selectedSocket == clientSocket) {
                readSocket = myaccept(clientSocket);
                if (readSocket < 0) {
                    throw RpcException(SERVER_ACCEPT_FAILED);
                }
            } else if (selectedSocket != bindSocket) {
                throw RpcException(SERVER_SELECT_FAILED);
            }

            int readBytes = myread(readSocket, header, sizeof(header));        
            if (readBytes >= MSG_HEADER_LEN) {
                getPacketHeader(header, length, type);
                packet = new unsigned char[MSG_HEADER_LEN + length];
                if (!packet) {
                    throw RpcException(PACKET_ALLOC_FAILED);
                }
                setPacketHeader(packet, length, type);
            } else {
                throw RpcException(BAD_PACKET_HEADER);
            }

            for (unsigned int offset = 0; readBytes > 0 && offset < length; offset += readBytes) {
                readBytes = myread(clientSocket, packet + MSG_HEADER_LEN + offset, length - offset);
            }
            if (readBytes <= 0) {
                throw RpcException(SERVER_READ_FAILED);
            }

            if(type == TERMINATE){
                if(memcmp(&binderAddr, &addr, sizeof(sockaddr_in)) == 0){
                    break;
                }
                throw RpcException(AUTHENTICATION_FAILED);
            }

            char methodName[MAX_NAME_LENGTH + 1] = {0};
            getPacketData(packet, CLIENT_EXEC_MSG_NAME, methodName, MAX_NAME_LENGTH);

            int* argTypes = (int*)(packet + MSG_HEADER_LEN + CLIENT_EXEC_MSG_ARGS);   

            int argTypeCount = 0;
            for (int* at = argTypes; *at; at++) {
                argTypeCount++;
            }

            Key key(methodName, argTypes);
            map<Key, skeleton>::iterator it = serverSkeletonMap.find(key);
            if (it == serverSkeletonMap.end()) {
                throw RpcException(METHOD_NOT_FOUND);
            }

            unsigned int argsLength = getTotalArgLength(argTypes);
            unsigned int argsOffset = packet + MSG_HEADER_LEN + CLIENT_EXEC_MSG_ARGS + sizeof(int) * argTypeCount;
            void** args = getPacketArgPointers(packet, argsOffset, argTypes);
 
            int result = (it->second)(argTypes, args);
            if(result == 0){
                setPacketArgData(packet, argsOffset, argTypes, args);
                if (sendPacket(readSocket, packet, sizeof(packet), EXECUTE_SUCCESS) <= 0) {
                    throw RpcException(SERVER_SEND_FAILED);
                }
            }
            else{
                setPacketData(packet, SERVER_FAIL_MSG_CAUSE, NULL, sizeof(int));
                setPacketData(packet, SERVER_FAIL_MSG_RESULT, &result, sizeof(int));
                if(sendPacket(readSocket, packet, SERVER_FAIL_MSG_LEN, EXECUTE_FAILURE) <= 0) {
                    throw RpcException(SERVER_SEND_FAILED);
                }
            }
        } catch (const RpcException e) {
            std::cerr << "rpcExecute: " << e.getException() << std::endl;

            int err = e.getErrorCode();
            if (readSocket && (e.getErrorCode() != SERVER_SEND_FAILED)) {
                char failPacket[MSG_HEADER_LEN + SERVER_FAIL_MSG_LEN] = {0};
                setPacketData(failPacket, SERVER_FAIL_MSG_CAUSE, &err, sizeof(int));
                sendPacket(readSocket, failPacket, SERVER_FAIL_MSG_LEN, EXECUTE_FAILURE);
            }
        }

        if (packet) delete[] packet;
        if (selectedSocket == clientSocket) {
            close(readSocket);
        }
    }

    close(bindSocket);
    close(clientSocket);
    bindSocket = 0;
    clientSocket = 0;

    return 0;  
}