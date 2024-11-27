#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>

#define NUM_LOGS 100000  // Número de logs a generar

int main() {
    // Abrir la conexión al demonio del sistema de log
    openlog("StressTestLogger", LOG_PID | LOG_CONS, LOG_USER);

    for (int i = 0; i < NUM_LOGS; i++) {
        syslog(LOG_INFO, "Log de información número %d", i);
        syslog(LOG_WARNING, "Log de advertencia número %d", i);
        syslog(LOG_ERR, "Log de error número %d", i);
        // Hacer una pausa breve para simular generación continua de logs
        usleep(100);
    }

    // Cerrar la conexión al demonio del sistema de log
    closelog();

    return 0;
}
