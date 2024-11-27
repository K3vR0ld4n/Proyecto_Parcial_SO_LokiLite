#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/wait.h>

#define NUM_SERVICIOS 2
#define INTERVALO_ACTUALIZACION 5 // Intervalo de actualización en segundos

// Función que monitorea un servicio con journalctl
void monitor_service(const char* service) {
    pid_t pid = fork();
    if (pid == 0) {
        // Proceso hijo ejecuta journalctl
        execlp("journalctl", "journalctl", "-u", service, "-f", NULL);
        perror("exec failed");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // Proceso padre espera a que el hijo termine
        wait(NULL);
    } else {
        perror("fork failed");
    }
}

int main(int argc, char *argv[]) {
    if (argc < NUM_SERVICIOS + 1) {
        fprintf(stderr, "Uso: %s <servicio1> <servicio2>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Crear hilos para cada servicio a monitorear
    pthread_t threads[NUM_SERVICIOS];
    for (int i = 0; i < NUM_SERVICIOS; i++) {
        pthread_create(&threads[i], NULL, (void *) monitor_service, (void *) argv[i + 1]);
    }

    // Esperar a que los hilos terminen
    for (int i = 0; i < NUM_SERVICIOS; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
