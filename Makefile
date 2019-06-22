PKG_CONFIG ?= pkg-config
CFLAGS += -Wall
CFLAGS += $(shell $(PKG_CONFIG) --cflags libnotify)
LDFLAGS += -lX11
LDFLAGS += $(shell $(PKG_CONFIG) --libs libnotify)

notifypaste: main.o
	$(CC) -o $@ $(LDFLAGS) $^
main.o: main.c
	$(CC) -c -o $@ $(CFLAGS) $<
