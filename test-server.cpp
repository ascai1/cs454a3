#include <iostream>
#include "string.h"
#include "packet.h"
#include "rpc.h"

int sum(int argc, int* argv) {
    int result = 0;
    for (int i = 0; i < argc; i++) {
        result += argv[i];
    }
    return result;
}

int sumSkeleton(int* argTypes, void ** args) {
    int argc = argTypes[1] & 0x0000ffff;
    int s = sum(argc, (int *)args[1]);
    memcpy(args[0], &s, sizeof(int));
    return 0;
}

int main() {
    rpcInit();

    char name[] = "sum";
    int argTypes[] = {
        ARG_OUTPUT|ARG_INT,
        ARG_INPUT|ARG_INT|1,
        0
    };
    rpcRegister(name, argTypes, sumSkeleton);

    rpcExecute();
    return 0;
}
