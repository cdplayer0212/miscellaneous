DESTDIR:=
PREFIX:=/usr/local

CC:=gcc
STRIP=strip
CFLAGS=-Wall -g -O2 -Wl,-z,relro,-z,now -fstack-protector-strong
APP=persist_logger

.PHONY: all clean install uninstal

all: $(APP)

clean:
	rm -f $(APP) $(APP).o

install: persist_logger
	install -D -m 555 $(APP) $(DESTDIR)$(PREFIX)/bin/$(APP)

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(APP)
