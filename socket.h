#ifndef SOCKET_H
#define SOCKET_H

int getBinderSocket();

/**
 * If any data arrives at socket "soc" in the next 0.5 seconds, return 1
 * If no data arrived, return 0
 * If something broke, print error and return -1
 */
int myselect(int soc);

/**
 * If any connection requests arrive at socket "soc" in the next 0.5 seconds, 
 *     accept and return the new socket number
 * Otherwise return -1 (print error if something went wrong)
 */
int selectAndAccept(int soc);

/**
 * If any data arrives at socket "soc" in the next 0.5 seconds, read up to
 *     "size" bytes into the buffer "buf" and return the number of bytes read
 * If the other side has closed the connection, return 0
 * Otherwise return -1 (print error if something went wrong)
 */
int selectAndRead(int soc, unsigned char * buf, unsigned int size);

/**
 * Basically a wrapper for recv
 */
int myread(int soc, unsigned char * buf, unsigned int size);

#endif
