CXX = g++
CXXFLAGS = -pthread
BINDER_OBJECTS = binder.o socket.o packet.o exception.o keyval.o
RPC_OBJECTS = server.o client.o socket.o packet.o exception.o keyval.o
EXECS = binder

all: librpc.a binder test-client

test-client: test-client.o librpc.a
	g++ -o test-client test-client.o -L. -lrpc


librpc.a: $(RPC_OBJECTS) 
	ar rvs librpc.a $(RPC_OBJECTS)

binder: $(BINDER_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $(BINDER_OBJECTS)

clean:
	rm -rf $(EXECS) *.o *.a
