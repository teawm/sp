CXX = g++

BINS = server client
all: $(BINS)

server: server.cpp
	$(CXX) -o server server.cpp
client: client.cpp
	$(CXX) -o client client.cpp

clean:
	rm -f $(BINS)