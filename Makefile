CXX = g++
CXXFLAGS = -pthread
BINDER_OBJECTS = socket.o packet.o exception.o
RPC_OBJECTS = server.o client.o socket.o packet.o exception.o
EXECS = binder

all: librpc.a binder

librpc.a: $(RPC_OBJECTS) 
	ar rvs librpc.a $(RPC_OBJECTS)

binder: $(BINDER_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $@.cpp $(BINDER_OBJECTS)

clean:
	rm -rf $(EXECS) *.o *.a
