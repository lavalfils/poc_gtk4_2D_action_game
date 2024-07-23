all:
	gcc game.c -o game `pkg-config --cflags --libs gtk4`
