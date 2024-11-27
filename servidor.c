#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUF_SIZE 4096
#define THRESHOLD 2  // Umbral de logs por prioridad para enviar alerta

void enviar_alerta(const char *mensaje) {
    printf("ALERTA: %s\n", mensaje);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUF_SIZE] = {0};

    // Crear el socket del servidor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Servidor esperando conexiones en el puerto %d...\n", PORT);

    while (1) {
        // Aceptar una nueva conexión
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // Leer los datos enviados por el cliente
        read(new_socket, buffer, BUF_SIZE);
        printf("Datos recibidos:\n%s\n", buffer);

        // Procesar los datos para buscar si se supera el threshold en algún servicio
        char *token = strtok(buffer, "\n");
        while (token != NULL) {
            char servicio[50];
            char prioridad[10];
            int conteo;
            sscanf(token, "%s %s %d", servicio, prioridad, &conteo);

            // Verificar si se ha superado el umbral
            if (conteo > THRESHOLD) {
                char alerta[100];
                snprintf(alerta, sizeof(alerta), "Servicio: %s Prioridad: %s supera el umbral con %d logs", servicio, prioridad, conteo);
                enviar_alerta(alerta);
            }

            token = strtok(NULL, "\n");
        }

        close(new_socket);
    }

    return 0;
}
