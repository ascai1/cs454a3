#include <iostream>
#include "rpc.h"
#include "packet.h"

int main() {
    int result;
    long num1 = 1;
    short num2 = 1;

    char name[] = "sum";
    int argTypes[] = {
        ARG_OUTPUT|ARG_INT,
        ARG_INPUT|ARG_LONG,
        ARG_INPUT|ARG_SHORT,
        0
    };
    void* args[] = {&result, &num1, &num2};
    int ret = rpcCall(name, argTypes, (void **)args);
    std::cout << "rpcCall returned " << ret << ", final result is " << result << std::endl;
    rpcTerminate();
    return 0;
}
