CXX = g++
CXXFLAGS = -pthread
BINDER_OBJECTS = binder.o socket.o packet.o exception.o keyval.o
RPC_OBJECTS = server.o client.o socket.o packet.o exception.o keyval.o
TARGETS = librpc.a binder test-server test-client

all: $(TARGETS)

test-server: test-server.o librpc.a
	g++ -o test-server test-server.o -L. -lrpc

test-client: test-client.o librpc.a
	g++ -o test-client test-client.o -L. -lrpc

librpc.a: $(RPC_OBJECTS) 
	ar rvs librpc.a $(RPC_OBJECTS)

binder: $(BINDER_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $(BINDER_OBJECTS)

clean:
	rm -rf $(TARGETS) *.o
