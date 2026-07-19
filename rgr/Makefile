CXX = g++

BINS = server client

all: $(BINS)

server: server.cpp
	$(CXX) -o server server.cpp -pthread
client: client.cpp
	$(CXX) -o client client.cpp -pthread

clean:
	rm -f $(BINS)