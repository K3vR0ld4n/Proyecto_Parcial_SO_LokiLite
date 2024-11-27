#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

void enviar_alerta(const char *mensaje) {
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if(curl) {
        // Configura la petición a Twilio 
        curl_easy_setopt(curl, CURLOPT_URL, "");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");

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
