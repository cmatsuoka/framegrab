
CC	= gcc
CFLAGS	= -Wall -O2 -g -DOLD_GIFLIB_API
LD	= gcc
LDFLAGS	=
AR	= ar
RANLIB	= ranlib
OBJS	= capture.o capture-v4l2.o convert.o convert-yuv.o write-jpeg.o \
	  write-gif.o
STATIC	= libframegrab.a

.c.o:
	$(CC) -c $(CFLAGS) -o $*.o $<

all: $(STATIC) test

$(STATIC): $(OBJS)
	$(AR) r $@ $(OBJS)
	$(RANLIB) $@

clean:
	rm -f core *~ *.o *.a

test: $(STATIC) test.o
	$(LD) -o $@ $(LDFLAGS) test.o $(STATIC) -ljpeg -lgif
