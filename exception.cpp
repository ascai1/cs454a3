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
    "No hostname found.",
    "No socket information found",
    "Method name too long",
    "Server has not registered any methods",
    "Server sockets not initialized",
    "Server failed to accept",
    "Server failed to select",
    "Could not create packet",
    "Could not read valid packet header",
    "Server failed to read",
    "Authentication failed",
    "Method not found",
    "Server failed to send",
    "Could not register method"
};
int lastError = -23;

RpcException::RpcException(int error): error(error) {}

std::string RpcException::getException() const {
    if (error >= 0) return "Success";
    if (error < lastError) return "Unknown error";
    return errorStrings[-error - 1];
}

int RpcException::getErrorCode() const {
    return error;
}
