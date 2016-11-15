.phony: all
	
clean:
	rm -rf bin
all:
	g++ -o bin/conctrl        src/ConsolaControl.cpp   -pthread -std=c++11
	g++ -o bin/procesoctrl    src/ProcesoControl.cpp   -pthread -std=c++11
	g++ -o bin/ProcesoSuicida src/ProcesoSuicida.cpp   -std=c++11
init:
	mkdir bin
	cp FichCfg.txt bin/FichCfg.txt
	cp conctrl.cfg bin/conctrl.cfg