test_new_send: canopenmsg.o test_new_send.o
	g++ canopenmsg.o test_new_send.o -o test_new_send -lpthread -lpcan

test_new_send.o: test_new_send.cpp
	g++ -c test_new_send.cpp -std=c++0x

canopenmsg.o: canopenmsg.cpp canopenmsg.h 
	g++ -c canopenmsg.cpp -std=c++0x

clean:
	\rm *.o *~ test_new_send test_read test_send

include $(shell rospack find mk)/cmake.mk