NAME=gorecover
R2_PLUGIN_PATH=$(shell r2 -hh|grep LIBR_PLUGINS|awk '{print $$2}')
CFLAGS=-g -fPIC $(shell pkg-config --cflags r_asm) -I ../include
LDFLAGS=-shared $(shell pkg-config --libs r_asm)
SO_EXT=$(shell uname|grep -q Darwin && echo dylib || echo so)

LIBS=gorecover.$(SO_EXT)

all: $(LIBS)

$(NAME).$(SO_EXT): gorecover.o
	$(CC) $(LDFLAGS) $^ -shared -o $@

clean:
	rm -f *.o
	rm -f *.so

install:
	cp -f *.$(SO_EXT) $(R2_PLUGIN_PATH)/

uninstall:
	rm -f $(R2_PLUGIN_PATH)/$(NAME).$(SO_EXT)
