.phony: all
	
clean:
	rm -rf bin
all:
	g++ -o bin/conctrl        src/ConsolaControl.cpp -Wall  -pthread -std=c++11
	g++ -o bin/procesoctrl    src/ProcesoControl.cpp -Wall  -pthread -std=c++11
	g++ -o bin/procesoSuicida src/ProcesoSuicida.cpp -Wall  -std=c++11
init:
	mkdir bin
