DESTDIR:=
PREFIX:=/usr/local

CC:=gcc
CFLAGS=-Wall -g -O2 -Wl,-z,relro,-z,now -fstack-protector-strong
APP=persist_logger

.PHONY: all clean install uninstal

all: $(APP)

clean:
	rm -f $(APP)

install: persist_logger
	install -D -m 555 $(APP) $(DESTDIR)$(PREFIX)/bin/$(APP)

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(APP)
