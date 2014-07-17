#include "exception.h"

std::string errorStrings[] = {
    "Method signature too long",
    "Could not send packet to binder",
    "Could not receive packet from binder",
    "Binder is unavailable",
    "Could not create socket to binder",
    "Could not create socket to server",
    "Server is unavailable",
    "Could not send packet to server",
    "Could not receive packet from server",
    "No hostname found."
};
unsigned int lastError = 9;

RpcException::RpcException(int error): error(error) {}

std::string RpcException::getException() {
    if (error >= 0) return "Success";
    if (-error - 1 > lastError) return "Unknown error";
    return errorStrings[-error - 1];
}

int RpcException::getErrorCode() {
    return error;
}
