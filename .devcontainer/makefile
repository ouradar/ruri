# I know, I know. Using a makefile for a single command is weird but this way I can "make clean", "make watch", etc.

VERSION = 4.6.0
CC = docker
CFLAGS = -t arrc/uhd-docker:${VERSION} -t arrc/uhd-docker:latest

all:
	$(CC) compose up --build -d

attach:
	$(CC) exec -it uhd-ruri bash

build:
	$(CC) build $(CFLAGS) --no-cache .

clean:
	$(CC) container prune -f

down:
	$(CC) compose down

stop:
	$(CC) compose down

up:
	$(CC) compose up -d --no-build
