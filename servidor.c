#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <curl/curl.h>

#define PORT 8080
#define BUF_SIZE 4096
#define INITIAL_THRESHOLD 50
#define MAX_ALERTS 5

typedef struct {
    char service_name[50];
    int total_logs;
    int threshold;
    int alert_count;
} ServiceData;

ServiceData *servicios = NULL;
int num_servicios = 0;

size_t no_op_callback(void *ptr, size_t size, size_t nmemb, void *data) {
    return size * nmemb; // No hacer nada con la respuesta.
}

void enviar_alerta(const char *service_name, int total_logs, int threshold, int alerta_numero) {
    CURL *curl;
    CURLcode res;

    // Obtener las variables sensibles necesarias para TWILIO desde el entorno
    const char *account_sid = getenv("TWILIO_ACCOUNT_SID");
    const char *auth_token = getenv("TWILIO_AUTH_TOKEN");
    const char *twilio_whatsapp_number = getenv("TWILIO_WHATSAPP_NUMBER");
    const char *recipient_whatsapp_number = getenv("TWILIO_RECIPIENT_WHATSAPP_NUMBER");

    if (!account_sid) {
        fprintf(stderr, "Error: Falta la variable de entorno TWILIO_ACCOUNT_SID\n");
        return;
    }
    if (!auth_token) {
        fprintf(stderr, "Error: Falta la variable de entorno TWILIO_AUTH_TOKEN\n");
        return;
    }
    if (!twilio_whatsapp_number) {
        fprintf(stderr, "Error: Falta la variable de entorno TWILIO_WHATSAPP_NUMBER\n");
        return;
    }
    if (!recipient_whatsapp_number) {
        fprintf(stderr, "Error: Falta la variable de entorno TWILIO_RECIPIENT_WHATSAPP_NUMBER\n");
        return;
    }

    // Inicializar libcurl
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl) {
        char *encoded_from = curl_easy_escape(curl, twilio_whatsapp_number, 0);
        char *encoded_to = curl_easy_escape(curl, recipient_whatsapp_number, 0);
        char url[256];
        snprintf(url, sizeof(url), "https://api.twilio.com/2010-04-01/Accounts/%s/Messages.json", account_sid);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        // Construir el cuerpo del mensaje
        char message[1024];
        snprintf(message, sizeof(message), "ALERTA #%d: El Servicio '%s' con un Total de Logs de %d ha superado el Umbral actual de %d", alerta_numero, service_name, total_logs, threshold);
        char postfields[2048];
        snprintf(postfields, sizeof(postfields), "From=%s&To=%s&Body=%s", encoded_from, encoded_to, message);

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields);

        char userpwd[512];
        snprintf(userpwd, sizeof(userpwd), "%s:%s", account_sid, auth_token);
        curl_easy_setopt(curl, CURLOPT_USERPWD, userpwd);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, no_op_callback);
        printf("%s\n",message);
        // Realizar la solicitud
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        // Limpiar
        curl_easy_cleanup(curl);
        curl_free(encoded_from);
        curl_free(encoded_to);
    } else {
        fprintf(stderr, "curl_easy_init() failed\n");
    }
    curl_global_cleanup();
}

int encontrar_indice_servicio(const char *service_name) {
    for (int i = 0; i < num_servicios; i++) {
        if (strcmp(servicios[i].service_name, service_name) == 0) {
            return i;
        }
    }
    return -1; 
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
