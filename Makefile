CC=gcc
LDFLAGS=`pkg-config opencv --cflags --libs`

all: vWin face

vWin:   main.c 
	$(CC) -o $@ $^ $(LDFLAGS)

face:   new.c 
	$(CC) -o $@ $^ $(LDFLAGS)


clean:
	rm -rf vWin

