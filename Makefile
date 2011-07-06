#
# create libraries
#

CFLAGS = -g -Wall -pthread -fPIC -I.
LDFLAGS = -shared -lpthread -pthread -lcap -lrt -lpcl

PCL_PATH:=/usr/local/lib
PCL_OBJS:=$(shell ar t $(PCL_PATH)/libpcl.a)



OBJS = buffer.o mailbox.o modimpl/monitoring.o scheduler.o stream.o \
       streamset.o task.o taskqueue.o lpel_main.o worker.o \
       #ctx/ctx_amd64.o


LIB_ST = liblpel.a
LIB_DYN = liblpel.so

.PHONY: all clean static dynamic

all: static dynamic

static: $(LIB_ST)

dynamic: $(LIB_DYN)


$(LIB_ST): $(OBJS) $(PCL_OBJS)
	ar ru $@ $^

$(LIB_DYN): $(OBJS)
	gcc $(LDFLAGS) -o $@ $(OBJS)

$(PCL_OBJS):
	ar x $(PCL_PATH)/libpcl.a $@

ctx/%.o: ctx/%.S
	gcc -c $(CFLAGS) $(FPIC) $< -o $@

%.o: %.c
	gcc -c $(CFLAGS) $(FPIC) $< -o $@

clean:
	rm -fr $(OBJS) $(PCL_OBJS) $(LIB_ST) $(LIB_DYN)