ssi: ssi.o
	gcc ssi.o -o ssi -lreadline
ssi.o: ssi.c
	gcc -c ssi.c
clean:
	rm -r ssi.o ssi
