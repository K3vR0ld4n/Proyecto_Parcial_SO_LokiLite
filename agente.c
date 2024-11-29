#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>  // Para O_NONBLOCK
#include <errno.h>  // Para errno y EAGAIN

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
    ssize_t bytes_read;

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

            // Configura la lectura no bloqueante de la tubería
            int flags = fcntl(pipefd[0], F_GETFL, 0);
            fcntl(pipefd[0], F_SETFL, flags | O_NONBLOCK);

            while (1) {
                bytes_read = read(pipefd[0], buffer, sizeof(buffer) - 1);
                if (bytes_read > 0) {
                    pthread_mutex_lock(&mutex);  // Bloquear acceso a los contadores
                    service_data->priority_count[i]++; 
                    pthread_mutex_unlock(&mutex);  // Desbloquear acceso a los contadores

                    // Para depuración, muestra los datos leídos
                    buffer[bytes_read] = '\0';  // Asegurar terminación de cadena
                    printf("Leído (%s, %s): %s\n", service_data->service_name, priority_levels[i], buffer);
                } else if (bytes_read == -1 && errno != EAGAIN) {
                    perror("read");
                    break;
                }

                usleep(500000); // Reducimos la espera para dar tiempo a los procesos a generar datos
            }

            close(pipefd[0]);  
            wait(NULL);  // Espera a que el proceso hijo termine
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
        servicios[i].service_name[49] = '\0'; // Asegurar la terminación en nulo
        memset(servicios[i].priority_count, 0, sizeof(int) * MAX_PRIORITY_LEVELS);
    }

    pthread_mutex_init(&mutex, NULL);  // Inicializar el mutex

    for (int i = 0; i < num_servicios; i++) {
        pthread_create(&threads[i], NULL, service_handler, (void *)&servicios[i]);
    }

    // En lugar de `sleep(3)`, usamos un mecanismo de sincronización
    // Esperamos a que los hilos se inicien antes de imprimir el dashboard

    int all_threads_ready = 0;
    while (!all_threads_ready) {
        all_threads_ready = 1;
        for (int i = 0; i < num_servicios; i++) {
            if (servicios[i].priority_count[0] == 0 && servicios[i].priority_count[1] == 0 && 
                servicios[i].priority_count[2] == 0 && servicios[i].priority_count[3] == 0 && 
                servicios[i].priority_count[4] == 0 && servicios[i].priority_count[5] == 0 && 
                servicios[i].priority_count[6] == 0) {
                all_threads_ready = 0;
                break;
            }
        }
        usleep(100000);  // Espera breve para no consumir muchos recursos
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
