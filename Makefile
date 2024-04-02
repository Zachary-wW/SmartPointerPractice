SRC = smart_pointer
$(SRC) :
	g++ -std=c++14 -o $(SRC) $(SRC).cpp -g -Wall
clean:
	rm $(SRC)