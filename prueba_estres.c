#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_PROCESOS 100

void generar_stress() {
    for (int i = 0; i < NUM_PROCESOS; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Proceso hijo simula una tarea de estrés (por ejemplo, un cálculo intensivo)
            for (long j = 0; j < 1000000000; j++);
            exit(0);
        }
    }
}

int main() {
    printf("Iniciando prueba de estrés...\n");
    generar_stress();
    printf("Prueba de estrés finalizada.\n");
    return 0;
}
