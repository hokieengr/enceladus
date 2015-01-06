#!/bin/sh
gcc -Wall -shared -o libencaludus.so -fPIC enceladus.c `pkg-config --cflags --libs libxfce4panel-1.0` `pkg-config --cflags --libs gtk+-3.0`

