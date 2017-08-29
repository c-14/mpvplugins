all: mvi.so xscrnsaver.so

mvi.so: mvi.c
	gcc -o mvi.so mvi.c `pkg-config --cflags mpv` -shared -fPIC -lm

xscrnsaver.so: xscrnsaver.c
	gcc -o xscrnsaver.so xscrnsaver.c `pkg-config --cflags mpv` -shared -fPIC `pkg-config --libs --cflags xscrnsaver`

install: mvi.so xscrnsaver.so
	install -D mvi.so $(HOME)/.config/mpv/scripts/mvi.so
	install -D xscrnsaver.so $(HOME)/.config/mpv/scripts/xscrnsaver.so
