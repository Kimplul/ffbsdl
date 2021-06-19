all:
	$(CC) -g ffbsdl.c -o ffbsdl $(shell sdl2-config --libs)

clean:
	rm ffbsdl
