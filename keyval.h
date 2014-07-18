#ifndef KEYVAL_H
#define KEYVAL_H

#include <string>
#include <vector>
#include <iostream>

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

struct ServerID {
    std::string name;
    std::string port;

    ServerID(char* name, char * port);
    ServerID(const ServerID& otherID);
    const char * getName();
    const char * getPort();
};

#endif
