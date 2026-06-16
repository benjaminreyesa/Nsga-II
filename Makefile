# Compilacion del NSGA-II adaptado (Entrega 3)
CXX      = g++
CXXFLAGS = -O2 -std=c++17 -Wall
OBJ      = instancia.o nsga2.o main.o
BIN      = nsga2

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(BIN) $(OBJ)

instancia.o: instancia.cpp instancia.hpp
	$(CXX) $(CXXFLAGS) -c instancia.cpp

nsga2.o: nsga2.cpp nsga2.hpp instancia.hpp
	$(CXX) $(CXXFLAGS) -c nsga2.cpp

main.o: main.cpp nsga2.hpp instancia.hpp
	$(CXX) $(CXXFLAGS) -c main.cpp

clean:
	rm -f $(OBJ) $(BIN) $(BIN).exe

.PHONY: all clean
