CC=gcc
LDFLAGS=`pkg-config opencv --cflags --libs`

vWin:   main.c
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf vWin

