target_camera_preview = camera_preview

CFLAGS ?= $(shell pkg-config --cflags gtk+-3.0) -O2 -Werror -g -fstack-protector-strong -fPIE -Wl,-z,relro,-z,now -D_FORTIFY_SOURCE=2
LDFLAGS ?= $(shell pkg-config --libs gtk+-3.0) -z now

CAMERA_TARGET_PREVIEW_OBJS = mfg_logger.o camera_util.o camera_preview.o

all: $(target) $(target_camera_preview) $(SUBDIR)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(target_camera_preview): $(CAMERA_TARGET_PREVIEW_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f \
		$(CAMERA_TARGET_PREVIEW_OBJS) \
		$(target_camera_preview)
