ifeq ($(OS),Windows_NT)
	CONSOLE := -mconsole
endif

all:
	$(CC) -g ffbsdl.c -o ffbsdl $(shell sdl2-config --libs) $(CONSOLE)

clean:
	rm ffbsdl
