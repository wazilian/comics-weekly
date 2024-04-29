
EXE			= comics-weekly
CC			= gcc -D APP_NAME='"$(EXE)"'
CFLAGS	= -Wall -Wextra

# mbedTLS library, point TLS_TREE to the location of the Mbed TLS directory
TLS_TREE	+= ../mbedtls
CFLAGS		+= -I $(TLS_TREE)/include
LDFLAGS		+= -L $(TLS_TREE)/library -lmbedtls -lmbedcrypto -lmbedx509

SRC = src
OBJ = src/obj

SOURCES := $(wildcard $(SRC)/*.c)
OBJECTS := $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SOURCES))

all:	$(EXE)

$(EXE): $(OBJECTS)
	$(CC) $^ -o $@ $(LDFLAGS)

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-@rm -f $(EXE) $(EXE).gdb $(OBJ)/*.o
