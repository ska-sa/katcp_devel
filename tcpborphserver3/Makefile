KATCP ?= ../katcp

include ../Makefile.inc

INC = -I$(KATCP)
#LIB = -L$(KATCP) -lkatcp -ldl -lz -lmagic
LIB = -L$(KATCP) -lkatcp -ldl -lz -rdynamic
CFLAGS += -fPIC
CFLAGS += -ggdb
CFLAGS += -DDEBUG
#CFLAGS += -DPROFILE
#CFLAGS += -DFAILFAST

SERVER = tcpborphserver3
SERVER_GITVER = $(SERVER)-$(GITVER)
SRC = main.c raw.c loadbof.c tg.c tapper.c hwmon.c upload.c subprocess.c ev.c plugin.c

OBJ = $(patsubst %.c,%.o,$(SRC))
all: $(SERVER_GITVER)

$(SERVER): $(SERVER_GITVER)
	cp $< $@

$(SERVER_GITVER): $(OBJ) $(KATCP)/libkatcp.a
	$(CC) -o $@ $(OBJ) $(LIB)

clean: 
	$(RM) -f ./$(SERVER) ./$(SERVER)-* *.o test-*

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ $(INC)

install: all
	$(INSTALL) $(SERVER) $(PREFIX)/sbin

test-bof: bof.c 
	$(CC) $(CFLAGS) -DSTANDALONE -o $@ $^ -I../katcp

