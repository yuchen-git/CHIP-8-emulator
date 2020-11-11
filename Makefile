LDFLAGS = -lsfml-graphics -lsfml-audio -lsfml-window -lsfml-system
CXXFLAGS = -O2 -std=c++17

all:
	g++ $(CXXFLAGS) -o ./main.o -c ./main.cc
	g++ -o ./chip8 ./main.o $(LDFLAGS)

clean:
	rm -f ./chip8 ./*.o
