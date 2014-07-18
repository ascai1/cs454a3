#include <iostream>
#include "rpc.h"
#include "packet.h"

int main() {
    int result;
    int nums[] = {1,2,3,4};

    char name[] = "sum";
    int argTypes[] = {
        ARG_OUTPUT|ARG_INT,
        ARG_INPUT|ARG_INT|4,
        0
    };
    int * args[] = {&result, nums};
    int ret = rpcCall(name, argTypes, (void **)args);
    std::cout << "rpcCall returned " << ret << ", final result is " << result << std::endl;
    rpcTerminate();
    return 0;
}
