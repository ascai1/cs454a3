#include <algorithm>
#include <utility>

#include "packet.h"
#include "keyval.h"

// printing method
void Key::print() const{
    std::cerr << "Name: " << name << std::endl;

    for(int i = 0; i < argTypes.size(); i++){
        std::cerr << "ArgType " << i << ": " << argTypes[i] << std::endl;
    }
}

// overwritten for map/other stdlib use
bool Key::operator< (const Key& otherKey) const {
    if(name != otherKey.name){
        return name < otherKey.name;
    }

    if (argTypes.size() != otherKey.argTypes.size()) {
        return argTypes.size() < otherKey.argTypes.size();
    }
   
    std::pair<types_v::const_iterator, types_v::const_iterator> mis
        = std::mismatch(argTypes.begin(), argTypes.end(), otherKey.argTypes.begin());
    
   return mis.first != argTypes.end() && *(mis.first) < *(mis.second);
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

// constructor
// puts method name into name
// puts argument types into argType vector
Key::Key(char* name, int* argTypes): name(name)
{
    for(int* at = argTypes; *at; at++){
        unsigned int argType = *at;
        unsigned int isArray = argType & ARG_ARR_LEN_MASK;
        argType = (argType & ~ARG_ARR_LEN_MASK) | (isArray ? 1 : 0);

        this->argTypes.push_back(argType);
    }
}

Key::Key(const Key& otherKey): name(otherKey.name), argTypes(otherKey.argTypes) {}

// "value" 
// the server which has the method being registered
bool ServerID::operator< (const ServerID & otherID) const {
    return name < otherID.name || port < otherID.port;
}
bool ServerID::operator> (const ServerID & otherID) const { return otherID < *this; }
bool ServerID::operator<= (const ServerID & otherID) const { return !(otherID < *this); }
bool ServerID::operator>= (const ServerID & otherID) const { return !(*this < otherID); }
bool ServerID::operator== (const ServerID & otherID) const {
    return name == otherID.name && port == otherID.port;
}
bool ServerID::operator!= (const ServerID & otherID) const { return !(*this == otherID); }

ServerID::ServerID() {}
ServerID::ServerID(char* name, char * port): name(name), port(port) {}
ServerID::ServerID(const ServerID& otherID): name(otherID.name), port(otherID.port) {}
ServerID & ServerID::operator=(const ServerID & otherID) {
    name = otherID.name;
    port = otherID.port;
    return *this;
}

const char * ServerID::getName() {
    return name.c_str();
}

const char * ServerID::getPort() {
    return port.c_str();
}
