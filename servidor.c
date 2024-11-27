#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUF_SIZE 1024

int main() {
    int pipefd[2];
    pid_t pid;
    char buffer[BUF_SIZE];

    // Crear pipe
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {  // Proceso hijo (agente)
        close(pipefd[0]);  // Cierra el lado de lectura
        write(pipefd[1], "Log generado por el agente", strlen("Log generado por el agente"));
        close(pipefd[1]);  // Cerrar el lado de escritura
        exit(EXIT_SUCCESS);
    } else {  // Proceso padre (servidor)
        close(pipefd[1]);  // Cierra el lado de escritura
        read(pipefd[0], buffer, BUF_SIZE);
        printf("Servidor recibió: %s\n", buffer);
        close(pipefd[0]);
    }

    return 0;
}
