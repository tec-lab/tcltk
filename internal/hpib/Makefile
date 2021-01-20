## Parameters
LIB = libhpib.so
SOURCES.c = tclHPIB.c tclAppInit.c tclConfig.c tclSRQ.c
SOURCES.h = tclHPIB.h tclConfig.h
OBJECTS   = $(SOURCES.c:%.c=%.o)
INCLUDES  = -I"$(INSTALLDIR)/include"
LDLIBS  = -ltcl8.6 -lsicl
LDFLAGS = -L"$(INSTALLDIR)/lib"

## Compiler flags
CFLAGS += -fPIC -DLINUX -D_WINDOWS
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
