CXX = g++
CXXFLAGS = -pthread
BINDER_OBJECTS = socket.o packet.o
EXECS = binder

binder: $(BINDER_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $@.cpp $(BINDER_OBJECTS)

clean:
	rm -rf $(EXECS) *.o
