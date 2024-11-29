#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <regex.h>
#include <pthread.h>
#include <semaphore.h>

#define MAX_PRIORITY_LEVELS 7  
#define BUF_SIZE 4096

// Estructura para almacenar los datos de cada servicio
typedef struct {
    char service_name[50];
    int priority_count[MAX_PRIORITY_LEVELS];  // Contador para cada nivel de prioridad
} ServiceData;

const char *priority_levels[] = { "alert", "crit", "err", "warning", "notice", "info", "debug" };

// Mutex para proteger los contadores de logs
pthread_mutex_t mutex;

// Semáforo para controlar el número de procesos hijos concurrentes
sem_t sem;

// Detectar líneas de logs válidas usando regex
int es_log_valido(const char *linea) {
    regex_t regex;
    int reti;

    if (regcomp(&regex, "^[[:alpha:]]{3} [[:digit:]]{2} [[:digit:]]{2}:[[:digit:]]{2}:[[:digit:]]{2}", REG_EXTENDED)) {
        fprintf(stderr, "Could not compile regex\n");
        exit(EXIT_FAILURE);
    }

    reti = regexec(&regex, linea, 0, NULL, 0);
    regfree(&regex);

    return (reti == 0) && (strncmp(linea, "--", 2) != 0) &&
           (strncmp(linea, "Hint:", 5) != 0) &&
           (strncmp(linea, "Logs begin at", 13) != 0) &&
           (strncmp(linea, "Logs end at", 11) != 0);
}

// Función que ejecuta journalctl para un servicio y prioridad, y cuenta los logs
void contar_logs_por_prioridad(ServiceData *service_data, int priority_level) {
    int pipefd[2];
    pid_t pid;
    char buffer[BUF_SIZE];

    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {  // Proceso hijo
        close(pipefd[0]);  

        dup2(pipefd[1], STDOUT_FILENO);  // Redirigir la salida estándar al pipe
        close(pipefd[1]);

        execlp("journalctl", "journalctl", "-u", service_data->service_name, "-p", priority_levels[priority_level], "--no-pager", NULL);
        perror("exec failed");
        exit(EXIT_FAILURE);
    } else {  // Proceso padre
        close(pipefd[1]);  
        sem_wait(&sem);

        FILE *stream = fdopen(pipefd[0], "r");
        int log_count = 0;

        while (fgets(buffer, sizeof(buffer), stream) != NULL) {
            if (es_log_valido(buffer)) {
                log_count++;
            }
        }

        pthread_mutex_lock(&mutex);
        service_data->priority_count[priority_level] = log_count;
        pthread_mutex_unlock(&mutex);

        fclose(stream);
        close(pipefd[0]);

        wait(NULL);  // Esperar a que el proceso hijo termine

        sem_post(&sem);
    }
}

// Función que imprime el dashboard en la terminal
void print_dashboard(ServiceData *servicios, int num_servicios) {
   // printf("\033[2J"); // Limpiar la pantalla
   // printf("\033[H");  // Mover el cursor al inicio
    printf("========= Dashboard de Monitoreo =========\n");
    printf("%-20s | %5s | %5s | %5s | %5s | %5s | %5s | %5s\n", 
        "Servicio", "ALERT", "CRIT", "ERR", "WARN", "NOTE", "INFO", "DEBUG");
    printf("-------------------------------------------------------------------------------\n");

    // Imprimir los datos de los logs
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
    printf("=========================================\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <servicio1> <servicio2> ... <servicioN>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int num_servicios = argc - 1;  // Número de servicios que se pasan como argumento

    // Crear memoria compartida para los datos de los servicios
    ServiceData *servicios = mmap(NULL, sizeof(ServiceData) * num_servicios, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (servicios == MAP_FAILED) {
        perror("mmap failed");
        return EXIT_FAILURE;
    }

    // Inicializar la estructura de datos de los servicios
    for (int i = 0; i < num_servicios; i++) {
        strncpy(servicios[i].service_name, argv[i + 1], 50);
        memset(servicios[i].priority_count, 0, sizeof(int) * MAX_PRIORITY_LEVELS);
    }

    pthread_mutex_init(&mutex, NULL);

    sem_init(&sem, 0, 3);  

    // Monitorear cada servicio y contar logs por prioridad
    for (int i = 0; i < num_servicios; i++) {
        for (int priority_level = 0; priority_level < MAX_PRIORITY_LEVELS; priority_level++) {
            contar_logs_por_prioridad(&servicios[i], priority_level);
        }
    }

    print_dashboard(servicios, num_servicios);

    munmap(servicios, sizeof(ServiceData) * num_servicios);

    pthread_mutex_destroy(&mutex);
    sem_destroy(&sem);

    return 0;
}
