#include <iostream>
#include "string.h"
#include "packet.h"
#include "rpc.h"

int sum(long num1, short num2) {
    int result = (int)(num1 + num2);

    std::cout << "testserver: sum: result: " << result << std::endl;

    return result;
}

int sumSkeleton(int* argTypes, void ** args) {
    int argc = argTypes[1] & 0x0000ffff;
    int s = sum(*(long *)args[1], *(short *)args[2]);
    memcpy(args[0], &s, sizeof(int));
    return 0;
}

int main() {
    rpcInit();

    char name[] = "sum";
    int argTypes[] = {
        ARG_OUTPUT|ARG_INT,
        ARG_INPUT|ARG_LONG,
        ARG_INPUT|ARG_SHORT,
        0
    };
    rpcRegister(name, argTypes, sumSkeleton);

    rpcExecute();
    return 0;
}
