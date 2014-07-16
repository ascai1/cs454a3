#include "packet.h"
#include <string.h>
#include <sys/socket.h>

void setPacketLength(unsigned char * packet, unsigned int length) {
    memcpy(packet + MSG_LENGTH, &length, sizeof(unsigned int));
}

void setPacketType(unsigned char * packet, unsigned int type) {
    memcpy(packet + MSG_TYPE, &type, sizeof(unsigned int));
}

void setPacketHeader(unsigned char * packet, unsigned int length, unsigned int type) {
    setPacketLength(packet, length);
    setPacketType(packet, type);
}

unsigned int getPacketLength(unsigned char * packet) {
    unsigned int length;
    memcpy(&length, packet + MSG_LENGTH, sizeof(unsigned int));
    return length;
}

unsigned int getPacketType(unsigned char * packet) {
    unsigned int type;
    memcpy(&type, packet + MSG_TYPE, sizeof(unsigned int));
    return type;
}

void getPacketHeader(unsigned char * packet, unsigned int & length, unsigned int & type) {
    length = getPacketLength(packet);
    type = getPacketType(packet);
}

int sendPacket(int soc, unsigned char * packet, unsigned int length, unsigned int type) {
    setPacketHeader(packet, length, type);
    return send(soc, packet, MSG_HEADER_LEN + length, 0);
}

void clearPacket(unsigned char * packet) {
    memset(packet, 0, sizeof(packet));
}

void setPacketData(unsigned char * packet, unsigned int offset, const void * data, int length) {
    memcpy(packet + MSG_HEADER_LEN + offset, data, length);
}
