#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUF_SIZE 4096
#define INITIAL_THRESHOLD 1000
#define MAX_ALERTS 5

typedef struct {
    char service_name[50];
    int total_logs;
    int threshold;
    int alert_count;
} ServiceData;

ServiceData *servicios = NULL;
int num_servicios = 0;

void enviar_alerta(const char *service_name, int total_logs, int threshold, int alerta_numero) {
    printf("ALERTA #%d: Servicio: %s | Total Logs: %d | Umbral Superado: %d\n", alerta_numero, service_name, total_logs, threshold);
}

int encontrar_indice_servicio(const char *service_name) {
    for (int i = 0; i < num_servicios; i++) {
        if (strcmp(servicios[i].service_name, service_name) == 0) {
            return i;
        }
    }
    return -1; // No encontrado
}

void procesar_dashboard(char *dashboard) {
    char *linea = strtok(dashboard, "\n");
    int line_number = 0;

    while (linea != NULL) {
        if (line_number >= 2) { // Ignorar las primeras dos líneas
            char service_name[50];
            int logs[7];
            int total_logs = 0;

            // Parsear la línea del dashboard
            if (sscanf(linea, "%49s | %d | %d | %d | %d | %d | %d | %d", 
                       service_name, &logs[0], &logs[1], &logs[2], &logs[3], &logs[4], &logs[5], &logs[6]) == 8) {
                for (int i = 0; i < 7; i++) {
                    total_logs += logs[i];
                }

                int indice_servicio = encontrar_indice_servicio(service_name);
                if (indice_servicio == -1) { // Nuevo servicio
                    num_servicios++;
                    servicios = realloc(servicios, num_servicios * sizeof(ServiceData));
                    if (servicios == NULL) {
                        perror("Error al realocar memoria");
                        exit(EXIT_FAILURE);
                    }
                    strncpy(servicios[num_servicios - 1].service_name, service_name, 50);
                    servicios[num_servicios - 1].total_logs = total_logs;
                    servicios[num_servicios - 1].threshold = INITIAL_THRESHOLD;
                    servicios[num_servicios - 1].alert_count = 0;
                } else { // Servicio existente
                    servicios[indice_servicio].total_logs = total_logs;
                }
            }
        }
        linea = strtok(NULL, "\n");
        line_number++;
    }

    for (int i = 0; i < num_servicios; i++) {
        if (servicios[i].total_logs > servicios[i].threshold) {
            if (servicios[i].alert_count < MAX_ALERTS) {
                servicios[i].alert_count++;
                enviar_alerta(servicios[i].service_name, servicios[i].total_logs, servicios[i].threshold, servicios[i].alert_count);
            } else {
                servicios[i].threshold *= 2; // Duplicar el umbral
                servicios[i].alert_count = 0; // Resetear el conteo de alertas
            }
        }
    }
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    while (1) {
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("accept failed");
            continue;
        }

        char buffer[BUF_SIZE] = {0};
        int bytes_read = read(client_socket, buffer, BUF_SIZE - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            printf("Dashboard recibido:\n%s\n", buffer);
            procesar_dashboard(buffer);
        }

        close(client_socket);
    }

    free(servicios);
    close(server_fd);
    return 0;
}
