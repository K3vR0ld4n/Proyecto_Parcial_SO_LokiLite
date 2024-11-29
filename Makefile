CC = gcc
CFLAGS = -Wall -lpthread  

TARGETS = agente cliente servidor prueba_estres

SRC_AGENTE = agente.c
SRC_CLIENTE = cliente.c
SRC_SERVIDOR = servidor.c
SRC_PRUEBA_ESTRES = prueba_estres.c

all: $(TARGETS)

agente: $(SRC_AGENTE)
	$(CC) $(CFLAGS) -o agente $(SRC_AGENTE)

cliente: $(SRC_CLIENTE)
	$(CC) $(CFLAGS) -o cliente $(SRC_CLIENTE)

servidor: $(SRC_SERVIDOR)
	$(CC) $(CFLAGS) -o servidor $(SRC_SERVIDOR) -lcurl

prueba_estres: $(SRC_PRUEBA_ESTRES)
	$(CC) $(CFLAGS) -o prueba_estres $(SRC_PRUEBA_ESTRES)

clean:
	rm -f $(TARGETS)
