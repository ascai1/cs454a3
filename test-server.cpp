#include <iostream>
#include "string.h"
#include "packet.h"
#include "rpc.h"

int sum(int argc, int* argv) {
    int result = 0;
    for (int i = 0; i < argc; i++) {
        std::cout << argv[i] << std::endl;
        result += argv[i];
    }

    std::cout << "testserver: sum: result: " << result << std::endl;

    return result;
}

int sumSkeleton(int* argTypes, void ** args) {
    int argc = argTypes[1] & 0x0000ffff;
    int s = sum(argc, (int *)args[1]);
    memcpy(args[0], &s, sizeof(int));
    return 0;
}

int mult(int argc, int* argv) {
    int result = 1;
    for (int i = 0; i < argc; i++) {
        std::cout << argv[i] << std::endl;
        result *= argv[i];
    }

    std::cout << "testserver: mult: result: " << result << std::endl;

    return result;
}

int multSkeleton(int* argTypes, void ** args) {
    int argc = argTypes[1] & 0x0000ffff;
    int s = mult(argc, (int *)args[1]);
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

    char name2[] = "mult";
    int argTypes2[] = {
        ARG_OUTPUT|ARG_INT,
        ARG_INPUT|ARG_INT|1,
        0
    };
    rpcRegister(name2, argTypes2, multSkeleton);

    rpcExecute();
    return 0;
}
