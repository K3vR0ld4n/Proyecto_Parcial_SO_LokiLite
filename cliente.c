#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

// Simula el envío de una alerta a través de la API de Twilio (o puedes modificarlo para usar otra API de mensajes).
void enviar_alerta(const char *mensaje) {
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if(curl) {
        // Configura la petición a Twilio (aquí deberás insertar los valores adecuados)
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.twilio.com/2010-04-01/Accounts/ACXXXXX/Messages.json");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "To=+1234567890&From=+1987654321&Body=Alerta:%20Threshold%20superado!");

        res = curl_easy_perform(curl);
        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

        curl_easy_cleanup(curl);
    }
}

int main() {
    // Simula que el cliente ha recibido una alerta del servidor
    printf("Cliente: Threshold superado, enviando alerta...\n");
    enviar_alerta("Threshold superado");
    return 0;
}
