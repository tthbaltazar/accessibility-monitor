#!/bin/sh

gcc $(pkg-config --cflags --libs dbus-1) monitor.c -o monitor
