main: main.cpp listContents.cpp
	g++ -w main.cpp listContents.cpp commandMode.cpp -o a.out

clean: 
	rm a.out
