all: mine_entrance

mine_entrance: mine_entrance.cpp map_parser.o error_handler.o libmap.a goldchase.h libmap.a
	g++ -O0 -g -std=c++17 mine_entrance.cpp -o mine_entrance map_parser.o error_handler.o -L. -lmap -lpanel -lncurses -pthread -lrt

map_parser.o: map_parser.cpp
	g++ -std=c++17 -c map_parser.cpp 

error_handler.o: error_handler.cpp
	g++ -std=c++17 -c error_handler.cpp

libmap.a: Screen.o Map.o
	ar -r libmap.a Screen.o Map.o

Screen.o: Screen.cpp
	g++ -std=c++17 -c Screen.cpp

Map.o: Map.cpp
	g++ -std=c++17 -c Map.cpp

clean:
	rm -f Screen.o Map.o libmap.a mine_entrance error_handler.o map_parser.o
