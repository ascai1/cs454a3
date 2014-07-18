#ifndef RPC_H
#define RPC_H

typedef int (*skeleton)(int*, void**);

int rpcCall(char * name, int * argTypes, void ** args);
int rpcInit();
int rpcRegister(char* name, int* argTypes, skeleton f);
int rpcExecute();
int rpcTerminate();

#endif
