#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define PORT 8080
#define BUF_SIZE 4096
#define INTERVALO_ACTUALIZACION 5 // Intervalo de actualización en segundos

// Función que ejecuta al agente, captura su salida (dashboard) y la retorna como cadena
void ejecutar_agente_y_obtener_dashboard(char *dashboard, char *argv[], int argc) {
    int pipefd[2];  // Pipe para capturar la salida del agente
    pid_t pid;
    char buffer[BUF_SIZE] = {0};
    ssize_t bytes_read;

    // Crear el pipe
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {  // Proceso hijo: Ejecuta al agente
        close(pipefd[0]);  

        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        // Construir los argumentos para el agente
        char *agente_args[argc + 2];  
        agente_args[0] = "./agente";  

        // Pasar los servicios desde argv al agente
        for (int i = 1; i < argc; i++) {
            agente_args[i] = argv[i];
        }
        agente_args[argc] = NULL;  

        execvp(agente_args[0], agente_args);

        perror("exec failed");
        exit(EXIT_FAILURE);
    } else {  // Proceso padre: Captura la salida del agente
        close(pipefd[1]);  
        // Leer la salida del agente del pipe
        bytes_read = read(pipefd[0], buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';  
            strncpy(dashboard, buffer, BUF_SIZE);
        }

        close(pipefd[0]);  
        wait(NULL);  
}}

// Función que envía el dashboard al servidor
void enviar_dashboard_al_servidor(char *dashboard) {
    int sock = 0;
    struct sockaddr_in serv_addr;

    // Crear el socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Error al crear el socket \n");
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convertir dirección IP a formato binario
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\n Dirección no válida o no soportada \n");
        return;
    }

    // Conectar al servidor
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\n Error en la conexión \n");
        return;
    }

    // Enviar el dashboard al servidor
    send(sock, dashboard, strlen(dashboard), 0);
    printf("Dashboard enviado:\n%s\n", dashboard);

    close(sock);
}

int main(int argc, char *argv[]) {
    char dashboard[BUF_SIZE] = {0};

    // Verificar que el usuario haya pasado al menos un servicio como argumento
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <servicio1> <servicio2> ... <servicioN>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    while (1) {
        ejecutar_agente_y_obtener_dashboard(dashboard, argv, argc);

        enviar_dashboard_al_servidor(dashboard);

        sleep(INTERVALO_ACTUALIZACION);
    }

    return 0;
}
