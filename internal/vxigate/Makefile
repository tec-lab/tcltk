## Parameters
LIB = libsicl.so
SOURCES.c = sicl.c vxi11core_xdr.c vxi11core_clnt.c
SOURCES.h = sicl.h vxi11core.h
OBJECTS   = $(SOURCES.c:%.c=%.o)
INCLUDES  = -I.
#LDLIBS  = -lnsl -ldl
LDFLAGS = -L.

## Compiler flags
CFLAGS += -fPIC -DLINUX
CC = gcc

## Targets
all:		$(LIB)

$(OBJECTS):	$(SOURCES.c)

%.o: %.c
		$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(LIB):		$(OBJECTS)
		$(CC) -shared $(OBJECTS) -o $(LIB) $(LDFLAGS) $(LDLIBS)

clean:
		$(RM) $(OBJECTS) $(LIB)
