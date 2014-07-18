#ifndef EXCEPTION_H
#define EXCEPTION_H

#define SIG_TOO_LONG -1
#define BAD_SEND_BIND -2
#define BAD_RECV_BIND -3
#define BINDER_UNAVAILABLE -4
#define BAD_CLIENT_SOCK -5
#define BAD_SERVER_SOCK -6
#define SERVER_UNAVAILABLE -7
#define BAD_SEND_SERVER -8
#define BAD_RECV_SERVER -9
#define NO_HOST_NAME -10
#define NO_PORT_NUMBER -11
#define NAME_TOO_LONG -12
#define NO_METHODS_REGISTERED -13
#define SOCKETS_NOT_INITIALIZED -14
#define SERVER_ACCEPT_FAILED -15
#define SERVER_SELECT_FAILED -16
#define PACKET_ALLOC_FAILED -17
#define BAD_PACKET_HEADER -18
#define SERVER_READ_FAILED -19
#define AUTHENTICATION_FAILED -20
#define METHOD_NOT_FOUND -21
#define SERVER_SEND_FAILED -22

class RpcException: public std::exception {
    int error;

    public:
        RpcException(int error);
        std::string getException();
        int getErrorCode();
};

#endif
