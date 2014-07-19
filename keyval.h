#ifndef KEYVAL_H
#define KEYVAL_H

#include <string>
#include <vector>
#include <iostream>

// used as the key in rpc system maps
// contains information regarding the method name and parameters it uses
struct Key {
    std::string name;
    typedef std::vector<unsigned int> types_v;
    types_v argTypes;

    bool operator< (const Key & otherKey) const;
    bool operator> (const Key & otherKey) const;
    bool operator<= (const Key & otherKey) const;
    bool operator>= (const Key & otherKey) const;
    bool operator== (const Key & otherKey) const;
    bool operator!= (const Key & otherKey) const;

    Key(char* name, int* argTypes);
    Key(const Key& otherKey);

    void print() const;
};

// used as the value in rpc system maps
// contains information regarding the server name and port of the method
struct ServerID {
    std::string name;
    std::string port;

    bool operator< (const ServerID & otherID) const;
    bool operator> (const ServerID & otherID) const;
    bool operator<= (const ServerID & otherID) const;
    bool operator>= (const ServerID & otherID) const;
    bool operator== (const ServerID & otherID) const;
    bool operator!= (const ServerID & otherID) const;

    ServerID();
    ServerID(char* name, char * port);
    ServerID(const ServerID& otherID);
    ServerID & operator=(const ServerID & otherID);

    const char * getName();
    const char * getPort();
};

#endif
