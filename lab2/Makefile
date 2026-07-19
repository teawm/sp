CXX = g++

BINS = server client
ZOMBIE = serverZ
all: $(BINS) $(ZOMBIE)

server: server.cpp
	$(CXX) -o server server.cpp
client: client.cpp
	$(CXX) -o client client.cpp
serverZ: serverZ.cpp
	$(CXX) -o serverZ serverZ.cpp

clean:
	rm -f $(BINS) $(ZOMBIE)