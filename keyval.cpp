#include <algorithm>
#include <utility>
#include "keyval.h"

bool Key::operator< (const Key& otherKey) const {
    if(name != otherKey.name){
        return name < otherKey.name;
    }

    if (argTypes.size() != otherKey.argTypes.size()) {
        return argTypes.size() < otherKey.argTypes.size();
    }
   
    std::pair<types_v::const_iterator, types_v::const_iterator> mis
        = mismatch(argTypes.begin(), argTypes.end(), otherKey.argTypes.begin());
    return *(mis.first) < *(mis.second);
}

bool Key::operator> (const Key& otherKey) const { return otherKey < *this; }
bool Key::operator<= (const Key& otherKey) const { return !(otherKey < *this); }
bool Key::operator>= (const Key& otherKey) const { return !(*this < otherKey); }
bool Key::operator== (const Key& otherKey) const {
    if(name != otherKey.name){
        return false;
    }

    if (argTypes.size() != otherKey.argTypes.size()) {
        return false;
    }
   
    return std::equal(argTypes.begin(), argTypes.end(), otherKey.argTypes.begin());
}
bool Key::operator!= (const Key& otherKey) const { return !(*this == otherKey); }

Key::Key(char* name, int* argTypes): name(name)
{
    for(int* at = argTypes; *at; at++){
        this->argTypes.push_back(*at);
    }
}

Key::Key(const Key& otherKey): name(otherKey.name), argTypes(otherKey.argTypes) {}


ServerID::ServerID(char* name, char * port): name(name), port(port) {}
ServerID::ServerID(const ServerID& otherID): name(otherID.name), port(otherID.port) {}

const char * ServerID::getName() {
    return name.c_str();
}

const char * ServerID::getPort() {
    return port.c_str();
}
