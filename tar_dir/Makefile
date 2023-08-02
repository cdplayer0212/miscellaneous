target = z_compress

CFLAGS ?= -O2 -Werror -g -fstack-protector-strong -fPIE -Wl,-z,relro,-z,now
LDFLAGS ?= -z now -lbz2 -ltar

OBJS = z_compress.o

all: $(target)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(target): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -fr $(target) $(OBJS)
