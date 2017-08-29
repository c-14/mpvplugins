all: mvi.so

mvi.so: mvi.c
	gcc -o mvi.so mvi.c `pkg-config --cflags mpv` -shared -fPIC -lm

install: mvi.so
	install -D mvi.so $(HOME)/.config/mpv/scripts/mvi.so
