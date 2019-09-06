default: dir bin/gerador bin/parque

dir:
	mkdir -p bin/
	mkdir -p obj/
bin/gerador: obj/gerador.o
	cc obj/gerador.o -o bin/gerador -lpthread -lrt
bin/parque: obj/parque.o
	cc obj/parque.o -o bin/parque -lpthread -lrt
obj/gerador.o: src/gerador.c
	cc -c src/gerador.c -o obj/gerador.o
obj/parque.o: src/parque.c
	cc -c src/parque.c -o obj/parque.o

clean:
	rm -f bin/*
	rm -f obj/*
