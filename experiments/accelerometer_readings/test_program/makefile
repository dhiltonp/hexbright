all: test.o hexbright.o
	g++ test.o hexbright.o -o test.bin

test.o: test.cpp ../../../libraries/hexbright/hexbright.h
	g++ -c test.cpp 

hexbright.o: ../../../libraries/hexbright/hexbright.cpp ../../../libraries/hexbright/hexbright.h ../../../libraries/hexbright/includes/NotArduino.h
	g++ -c ../../../libraries/hexbright/hexbright.cpp

../../../libraries/hexbright/hexbright.h:
	

clean:
	rm -rf *.o test.bin
