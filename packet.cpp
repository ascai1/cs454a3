#include <iostream>
#include "packet.h"
#include <string.h>
#include <sys/socket.h>

// sets the packet's length field to the passed in value
void setPacketLength(unsigned char * packet, unsigned int length) {
    memcpy(packet + MSG_LENGTH, &length, sizeof(unsigned int));
}

// sets the packet's type field to the passed in value
void setPacketType(unsigned char * packet, unsigned int type) {
    memcpy(packet + MSG_TYPE, &type, sizeof(unsigned int));
}

// sets the packet's header to the given value
void setPacketHeader(unsigned char * packet, unsigned int length, unsigned int type) {
    setPacketLength(packet, length);
    setPacketType(packet, type);
}

// retrieves the length of the packet from the packet 
unsigned int getPacketLength(unsigned char * packet) {
    unsigned int length;
    memcpy(&length, packet + MSG_LENGTH, sizeof(unsigned int));
    return length;
}

// retrieves the type of the packet from the packet
unsigned int getPacketType(unsigned char * packet) {
    unsigned int type;
    memcpy(&type, packet + MSG_TYPE, sizeof(unsigned int));
    return type;
}

// retrieves the packet header from the packet
void getPacketHeader(unsigned char * packet, unsigned int & length, unsigned int & type) {
    length = getPacketLength(packet);
    type = getPacketType(packet);
}

// sets the packet header to the given socket
// sends the packet using the socket
int sendPacket(int soc, unsigned char * packet, unsigned int length, unsigned int type) {
    setPacketHeader(packet, length, type);
    return send(soc, packet, MSG_HEADER_LEN + length, 0);
}

// clears a given packet
void clearPacket(unsigned char * packet) {
    memset(packet, 0, sizeof(packet));
}

// copies data into the packet starting at the offset
void setPacketData(unsigned char * packet, unsigned int offset, const void * data, int length) {
    if (!data) {
        memset(packet + MSG_HEADER_LEN + offset, 0, length);
    } else {
        memcpy(packet + MSG_HEADER_LEN + offset, data, length);
    }
}

// retrieves the packet's data starting at the offset into the data array
void getPacketData(const unsigned char * packet, unsigned int offset, void * data, int length){
    memcpy(data, packet + MSG_HEADER_LEN + offset, length);
}

// calculates the total length of all the arguments in the array given
unsigned int getTotalArgLength(int * argTypes){
    unsigned int totalArgLength = 0;

    for (int* at = argTypes; *at; at++) {
        unsigned int argLength = ((unsigned int)(*at) & ARG_ARR_LEN_MASK);
        if(argLength == 0){
            argLength = 1;
        }
        unsigned int dataType = (unsigned int)(*at) & ARG_TYPE_ID_MASK;

        if(dataType == ARG_CHAR){
            totalArgLength += sizeof(char) * argLength;    
        }
        else if(dataType == ARG_SHORT){
            totalArgLength += sizeof(short) * argLength;
        }
        else if(dataType == ARG_INT){
            totalArgLength += sizeof(int) * argLength;
        }
        else if(dataType == ARG_LONG){
            totalArgLength += sizeof(long) * argLength;
        }
        else if(dataType == ARG_DOUBLE){
            totalArgLength += sizeof(double) * argLength;
        }
        else if(dataType == ARG_FLOAT){
            totalArgLength += sizeof(float) * argLength;
        }
        // throw exception
    }  

    return totalArgLength;
}

// looks through a packet starting at the offset
// fills the packet's argument fields with values from args
void setPacketArgData(unsigned char * packet, unsigned int offset, const int * argTypes, const void * const* args, unsigned int iomask){
    int newOffset = offset + MSG_HEADER_LEN;

    int argc = 0;
    for(const int* at = argTypes; *at; at++){
        argc++;
    }

    unsigned int dataLength;
    for(int i = 0; i < argc; i++){
        unsigned int argLength = ((unsigned int)(argTypes[i]) & ARG_ARR_LEN_MASK);
        if(argLength == 0){
            argLength = 1;
        }
        unsigned int dataType = (unsigned int)(argTypes[i]) & ARG_TYPE_ID_MASK;
        const void* arg = args[i];

        if(dataType == ARG_CHAR){
            dataLength = sizeof(char);
        }
        else if(dataType == ARG_SHORT){
            dataLength = sizeof(short); 
        }
        else if(dataType == ARG_INT){  
            dataLength = sizeof(int);
        }
        else if(dataType == ARG_LONG){
            dataLength = sizeof(long);
        }
        else if(dataType == ARG_DOUBLE){
            dataLength = sizeof(double);
        }
        else if(dataType == ARG_FLOAT){
            dataLength = sizeof(float);
        }  

        if((unsigned int)(argTypes[i]) & iomask){
            memcpy(packet + newOffset, arg, argLength * dataLength);   
        }
        
        newOffset += dataLength * argLength;    
    }
}

// looks through a packet starting at the offset
// returns an array of void* with the arguments found in the packet
void getPacketArgData(unsigned char * packet, unsigned int offset, const int * argTypes, void ** args, unsigned int iomask){
    int newOffset = offset + MSG_HEADER_LEN;

    int argc = 0;
    for(const int* at = argTypes; *at; at++){
        argc++;
    }

    unsigned int dataLength = 0;
    for(int i = 0; i < argc; i++){
        unsigned int argLength = ((unsigned int)(argTypes[i]) & ARG_ARR_LEN_MASK);
        if(argLength == 0){
            argLength = 1;
        }
        unsigned int dataType = (unsigned int)(argTypes[i]) & ARG_TYPE_ID_MASK;
        void* arg = args[i];

        if(dataType == ARG_CHAR){
            dataLength = sizeof(char);
        }
        else if(dataType == ARG_SHORT){
            dataLength = sizeof(short); 
        }
        else if(dataType == ARG_INT){  
            dataLength = sizeof(int);
        }
        else if(dataType == ARG_LONG){
            dataLength = sizeof(long);
        }
        else if(dataType == ARG_DOUBLE){
            dataLength = sizeof(double);
        }
        else if(dataType == ARG_FLOAT){
            dataLength = sizeof(float);
        }  

        if((unsigned int)(argTypes[i]) & iomask){
            memcpy(arg, packet + newOffset, argLength * dataLength);    
        }

        newOffset += dataLength * argLength;
    }
}

// looks through the packet and returns an array of pointers
// each pointer points to an argument
void** getPacketArgPointers(unsigned char * packet, unsigned int offset, int * argTypes){
    int newOffset = offset + MSG_HEADER_LEN;

    int argc = 0;
    for(int* at = argTypes; *at; at++){
        argc++;
    }

    void** argPointers = new void*[argc];

    for(int i = 0; i < argc; i++){
        unsigned int argLength = ((unsigned int)(argTypes[i]) & ARG_ARR_LEN_MASK);
        if(argLength == 0){
            argLength = 1;
        }
        unsigned int dataType = (unsigned int)(argTypes[i]) & ARG_TYPE_ID_MASK;

        unsigned int dataLength;
        if(dataType == ARG_CHAR){
            dataLength = sizeof(char);
        }
        else if(dataType == ARG_SHORT){
            dataLength = sizeof(short); 
        }
        else if(dataType == ARG_INT){  
            dataLength = sizeof(int);
        }
        else if(dataType == ARG_LONG){
            dataLength = sizeof(long);
        }
        else if(dataType == ARG_DOUBLE){
            dataLength = sizeof(double);
        }
        else if(dataType == ARG_FLOAT){
            dataLength = sizeof(float);
        } 

        argPointers[i] = (packet + newOffset);
        newOffset += dataLength * argLength;
    } 

    return argPointers;
}
