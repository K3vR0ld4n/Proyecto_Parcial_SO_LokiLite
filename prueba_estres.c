#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <time.h>

#define NUM_THREADS 10

// Función para crear el archivo de servicio systemd temporalmente
void crear_servicio_systemd() {
    const char *service_content = 
    "[Unit]\n"
    "Description=Servicio de prueba de estrés para generar logs\n"
    "\n"
    "[Service]\n"
    "ExecStart=/bin/bash -c '/usr/bin/logger -t prueba_estres -p local0.alert \"Log generado con prioridad ALERT\"; /usr/bin/logger -t prueba_estres -p local0.crit \"Log generado con prioridad CRIT\"; /usr/bin/logger -t prueba_estres -p local0.err \"Log generado con prioridad ERR\"; /usr/bin/logger -t prueba_estres -p local0.warning \"Log generado con prioridad WARNING\"; /usr/bin/logger -t prueba_estres -p local0.notice \"Log generado con prioridad NOTICE\"; /usr/bin/logger -t prueba_estres -p local0.info \"Log generado con prioridad INFO\"; /usr/bin/logger -t prueba_estres -p local0.debug \"Log generado con prioridad DEBUG\"'\n"
    "Restart=always\n"
    "RestartSec=1\n"
    "User=root\n"
    "StandardOutput=journal\n"
    "StandardError=journal\n"
    "StartLimitInterval=0\n"
    "\n"
    "[Install]\n"
    "WantedBy=multi-user.target\n";

    // Crear el archivo de servicio en /etc/systemd/system
    FILE *service_file = fopen("/etc/systemd/system/prueba_estres.service", "w");
    if (!service_file) {
        perror("No se pudo crear el archivo de servicio");
        exit(EXIT_FAILURE);
    }

    fprintf(service_file, "%s", service_content);
    fclose(service_file);

    system("systemctl daemon-reload");
    system("systemctl enable prueba_estres.service");
    system("systemctl start prueba_estres.service");
}

// Función para generar logs con niveles de prioridad aleatorios
void *generar_logs(void *arg) {
    const char *priorities[] = {
        "alert", "crit", "err", "warning", "notice", "info", "debug"
    };

    for (int i = 0; i < 50; i++) {  // Cada hilo genera 50 logs
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char message[256];
        strftime(message, sizeof(message), "%Y-%m-%d %H:%M:%S", tm_info);
        int priority_index = rand() % 7;
        char command[512];
        snprintf(command, sizeof(command), "logger -t prueba_estres -p local0.%s \"%s Generando log de prueba\"", priorities[priority_index], message);
        system(command);  

        usleep(500000);  // Pausa de 500ms entre logs para evitar generar logs demasiado rápido
    }

    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    int i;

    printf("Iniciando prueba de estrés...\n");

    // Crear el archivo de servicio de systemd temporalmente
    crear_servicio_systemd();

    // Crear NUM_THREADS hilos que generarán logs en paralelo
    for (i = 0; i < NUM_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, generar_logs, NULL) != 0) {
            perror("Error creando hilo");
            exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    sleep(5);

    // Detener y deshabilitar el servicio después de la prueba
    system("systemctl stop prueba_estres.service");
    system("systemctl disable prueba_estres.service");

    printf("Prueba de estrés completada. Logs generados.\n");

    return 0;
}
