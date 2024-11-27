#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/mman.h>

#define INTERVALO_ACTUALIZACION 5  
#define MAX_PRIORITY_LEVELS 7   

pthread_mutex_t mutex;

// Estructura para almacenar los datos de cada servicio
typedef struct {
    char service_name[50];
    int priority_count[MAX_PRIORITY_LEVELS];  
} ServiceData;

const char *priority_levels[] = { "alert", "crit", "err", "warning", "notice", "info", "debug" };

// Función que imprime el dashboard en la terminal
void print_dashboard(ServiceData *servicios, int num_servicios) {
    printf("\033[2J"); // Limpiar la pantalla
    printf("\033[H");  // Mover el cursor al inicio
    printf("========= Dashboard de Monitoreo =========\n");
    printf("%-20s | %5s | %5s | %5s | %5s | %5s | %5s | %5s\n", "Servicio", "ALERT", "CRIT", "ERR", "WARN", "NOTE", "INFO", "DEBUG");
    printf("-------------------------------------------------------------------------------\n");

    pthread_mutex_lock(&mutex);  // Bloqueo del mutex para lectura segura
    for (int i = 0; i < num_servicios; i++) {
        printf("%-20s | %5d | %5d | %5d | %5d | %5d | %5d | %5d\n", 
            servicios[i].service_name, 
            servicios[i].priority_count[0], 
            servicios[i].priority_count[1], 
            servicios[i].priority_count[2], 
            servicios[i].priority_count[3], 
            servicios[i].priority_count[4], 
            servicios[i].priority_count[5], 
            servicios[i].priority_count[6]);
    }
    pthread_mutex_unlock(&mutex);  // Desbloqueo del mutex
    printf("=========================================\n");
}

// Función ejecutada por los procesos hijos para monitorear los logs con `journalctl`
void monitor_service(int fd, const char *service, const char *priority) {
    dup2(fd, STDOUT_FILENO);
    execlp("journalctl", "journalctl", "-u", service, "-p", priority, "-f", "--no-pager", NULL);
    perror("exec failed");
    exit(EXIT_FAILURE);
}

// Función del hilo principal que coordina la ejecución de procesos
void* service_handler(void* arg) {
    ServiceData *service_data = (ServiceData*)arg;
    int pipefd[2];  
    pid_t pid;
    char buffer[1024];

    for (int i = 0; i < MAX_PRIORITY_LEVELS; i++) {
        if (pipe(pipefd) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        pid = fork();
        if (pid == -1) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {  // Proceso hijo
            close(pipefd[0]);  
            monitor_service(pipefd[1], service_data->service_name, priority_levels[i]);
            exit(EXIT_SUCCESS);
        } else {  // Proceso padre
            close(pipefd[1]);  
            while (read(pipefd[0], buffer, sizeof(buffer)) > 0) {
                pthread_mutex_lock(&mutex);  // Bloquear acceso a los contadores
                service_data->priority_count[i]++; 
                pthread_mutex_unlock(&mutex);  // Desbloquear acceso a los contadores
            }

            close(pipefd[0]);  
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <servicio1> <servicio2> ... <servicioN>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int num_servicios = argc - 1;  
    pthread_t threads[num_servicios];  

    ServiceData *servicios = mmap(NULL, sizeof(ServiceData) * num_servicios, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (servicios == MAP_FAILED) {
        perror("mmap failed");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < num_servicios; i++) {
        strncpy(servicios[i].service_name, argv[i + 1], 50);
        memset(servicios[i].priority_count, 0, sizeof(int) * MAX_PRIORITY_LEVELS);
    }

    pthread_mutex_init(&mutex, NULL);  // Inicializar el mutex

    for (int i = 0; i < num_servicios; i++) {
        pthread_create(&threads[i], NULL, service_handler, (void *)&servicios[i]);
    }

    // Actualizar el dashboard cada INTERVALO_ACTUALIZACION segundos
    while (1) {
        sleep(INTERVALO_ACTUALIZACION);
        print_dashboard(servicios, num_servicios);
    }

    // Esperar a que los hilos terminen
    for (int i = 0; i < num_servicios; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&mutex);  // Destruir el mutex
    munmap(servicios, sizeof(ServiceData) * num_servicios);  // Liberar memoria compartida

    return 0;
}
