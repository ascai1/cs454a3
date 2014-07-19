#ifndef SOCKET_H
#define SOCKET_H

#include <netinet/in.h>

int getClientBinderSocket();
int getClientServerSocket(const char * name, const char * port);

int getServerBinderSocket(sockaddr_in & addr);
int getServerClientSocket();

int getBinderSocket();

/**
 * Get the current host identifier and place it in "hostname".
 * Return 0 on success.
 */
int getHost(char* hostname);

/**
 * Get the socket information bound to "soc" and place it in "addr".
 * Return 0 on success.
 */
int getSockName(int soc, sockaddr_in* addr);

/**
 * If any data arrives at the "numsocs" sockets in array "socs" in the next "tv" seconds,
 *   return the first available socket FD
 * If no data arrived, return 0
 * If something broke, print error and return -1
 */
int myselect(int * socs, int numsocs, timeval * tv);

/**
 * If any connection requests arrive at socket "soc" in the next 0.5 seconds, 
 *     accept and return the new socket number
 * If nothing arrived, return 0
 * If something broke, print error and return a negative number
 */
int selectAndAccept(int soc);

/**
 * If any data arrives at socket "soc" in the next 0.5 seconds, read up to
 *     "size" bytes into the buffer "buf" and return the number of bytes read
 * If the other side has closed the connection, return 0
 * If nothing arrived, return -1
 * Otherwise return something less than -1
 */
int selectAndRead(int soc, unsigned char * buf, unsigned int size);

/**
 * Basically a wrapper for recv
 */
int myread(int soc, unsigned char * buf, unsigned int size);

/**
 * Basically a wrapper for accept
 */
int myaccept(int soc);

#endif
