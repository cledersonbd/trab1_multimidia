CC=gcc
LDFLAGS=`pkg-config opencv --cflags --libs`

all: vWin face facept

vWin:   main.c 
	$(CC) -o $@ $^ $(LDFLAGS)

face:   new.c 
	$(CC) -o $@ $^ $(LDFLAGS)

facept:   facept.c 
	$(CC) -o $@ $^ $(LDFLAGS)


clean:
	rm -rf vWin

