CC = gcc
CFLAGS = -Wall -lpthread  

TARGETS = agente cliente servidor

SRC_AGENTE = agente.c
SRC_CLIENTE = cliente.c
SRC_SERVIDOR = servidor.c

all: $(TARGETS)

agente: $(SRC_AGENTE)
	$(CC) $(CFLAGS) -o agente $(SRC_AGENTE)

cliente: $(SRC_CLIENTE)
	$(CC) $(CFLAGS) -o cliente $(SRC_CLIENTE)

servidor: $(SRC_SERVIDOR)
	$(CC) $(CFLAGS) -o servidor $(SRC_SERVIDOR)

clean:
	rm -f $(TARGETS)
