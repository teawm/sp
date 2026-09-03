CXX = g++

BINS = game

all: $(BINS)

game: game.cpp
	$(CXX) -o game game.cpp -pthread

clean:
	rm -f $(BINS)