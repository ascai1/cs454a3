#ifndef SERVER_H
#define SERVER_H

int rpcInit(void);

int rpcRegister(char* name, int* argTypes, skeleton f);

int rpcExecute(void);

#endif