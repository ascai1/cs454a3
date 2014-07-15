#ifndef PACKET_H
#define PACKET_H

#define ARG_LENGTH 4

#define ARG_CHAR (1 << 16)
#define ARG_SHORT (2 << 16)
#define ARG_INT (3 << 16)
#define ARG_LONG (4 << 16)
#define ARG_DOUBLE (5 << 16)
#define ARG_FLOAT (6 << 16)

#define ARG_INPUT (1 << 31)
#define ARG_OUTPUT (1 << 30)


#define MSG_LENGTH 0
#define MSG_TYPE 4
#define MSG_START 8

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

#define BINDER_MSG_REG_HOST MSG_START
#define BINDER_MSG_REG_PORT (BINDER_MSG_REG_HOST + MAX_HOST_LENGTH)
#define BINDER_MSG_REG_NAME (BINDER_MSG_REG_PORT + MAX_PORT_LENGTH)
#define BINDER_MSG_REG_ARGS (BINDER_MSG_REG_NAME + MAX_NAME_LENGTH)

#define BINDER_MSG_LOC_NAME MSG_START
#define BINDER_MSG_LOC_ARGS (BINDER_MSG_LOC_NAME + MAX_NAME_LENGTH)


void setPacketLength(unsigned char * packet, unsigned int length);
void setPacketType(unsigned char * packet, unsigned int type);
void setPacketHeader(unsigned char * packet, unsigned int length, unsigned int type);

unsigned int getPacketLength(unsigned char * packet);
unsigned int getPacketType(unsigned char * packet);
void getPacketHeader(unsigned char * packet, unsigned int & length, unsigned int & type);

/**
 * Populate the header and send the packet, returning the results
 */
int sendPacket(int soc, unsigned char * packet, unsigned int length, unsigned int type);

#endif
