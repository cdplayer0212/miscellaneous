target_gtk = logs_collector_gtk
target_app = logs_collector

CFLAGS ?= -O2 -Werror -Wall -g -fstack-protector-strong -fPIE -Wl,-z,relro,-z,now
LDFLAGS ?= -z now

CFLAGS_GTK ?= $(shell pkg-config --cflags gtk+-3.0)
LDFLAGS_GTK ?= $(shell pkg-config --libs gtk+-3.0)

COMMON_OBJS = logger.o logs_collector_ver.o common_funcs.o mounts_funcs.o

GTK_OBJS = logs_collector_gtk.o

APP_OBJS = logs_collector.o logs_collector_conf.o

USE_TAR_BZ2_COMPRESS=true
USE_ZIP_COMPRESS=true

ARCHIVE_CFLAGS=
ARCHIVE_LDFLAGS=
ifeq ($(USE_TAR_BZ2_COMPRESS),true)
  ARCHIVE_CFLAGS += -DUSE_TAR_BZ2_COMPRESS
  ARCHIVE_LDFLAGS += -lbz2 -ltar
  APP_OBJS += tar_bz2_funcs.o
endif
ifeq ($(USE_ZIP_COMPRESS),true)
  ARCHIVE_CFLAGS += -DUSE_ZIP_COMPRESS
  ARCHIVE_LDFLAGS += -lzip
  APP_OBJS += zip_funcs.o
endif

all: $(target_gtk) $(target_app)

%.o: %.c
	$(CC) $(CFLAGS) $(CFLAGS_GTK) $(ARCHIVE_CFLAGS) -o $@ -c $<

$(target_gtk): $(GTK_OBJS) $(COMMON_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(LDFLAGS_GTK)

$(target_app): $(APP_OBJS) $(COMMON_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(ARCHIVE_LDFLAGS)

clean:
	rm -f *.o $(target_gtk) $(target_app)
