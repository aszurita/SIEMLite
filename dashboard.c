#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <curl/curl.h>
#include "dashboard.h"

size_t discard_response(void *ptr, size_t size, size_t nmemb, void *userdata) {
    return size * nmemb;
}

void enviar_alerta(const char *servicio, int total)
{
    printf("\n\n**************************\n");
    printf("!!! ALERTA: El servicio '%s' ha superado el umbral con %d mensajes críticos !!!\n", servicio, total);
    printf("!!! INTENTANDO ENVIAR ALERTA POR WHATSAPP... !!!\n");
    printf("************************\n\n");

    const char *account_sid = "ACe6e033c1ba04ca9b69925eea526f2972";
    const char *auth_token = "ccaebea261491bf011a4aa320d753c8d";
    const char *from_whatsapp_number = "whatsapp:+14155238886";
    const char *to_whatsapp_number   = "whatsapp:+593988035770";

    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if (curl) {
        char url[256];
        sprintf(url, "https://api.twilio.com/2010-04-01/Accounts/%s/Messages.json", account_sid);

        char *escaped_to = curl_easy_escape(curl, to_whatsapp_number, 0);
        char *escaped_from = curl_easy_escape(curl, from_whatsapp_number, 0);

        char body_text[256];
        sprintf(body_text, "\n🚨 ALERTA CRÍTICA 🚨\n\nServicio: %s\nErrores detectados: %d\n\nPor favor, revisa el estado del servicio inmediatamente.\n", servicio, total);
        char *escaped_body = curl_easy_escape(curl, body_text, 0);

        char post_data[512];
        sprintf(post_data, "To=%s&From=%s&Body=%s", escaped_to, escaped_from, escaped_body);

        curl_free(escaped_to);
        curl_free(escaped_from);
        curl_free(escaped_body);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        char userpwd[256];
        sprintf(userpwd, "%s:%s", account_sid, auth_token);
        curl_easy_setopt(curl, CURLOPT_USERPWD, userpwd);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discard_response); // evita impresión del JSON

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "Error al enviar la alerta de WhatsApp: %s\n", curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
    }
}

void analizar_logs(FILE *fp, Estadisticas *est) {
    char linea[1024];

    while (fgets(linea, sizeof(linea), fp)) {
        if (strncmp(linea, "PRIORITY=", 9) == 0) {
            int prioridad = atoi(linea + 9);
            if (prioridad >= 0 && prioridad <= 7) {
                int *contador = &est->emerg + prioridad;
                (*contador)++;
            }
        }
    }
}

void obtener_logs_formateados(FILE *fp, char logs[10][1024], int *count) {
    char linea[1024];
    *count = 0;

    while (fgets(linea, sizeof(linea), fp)) {
        linea[strcspn(linea, "\n")] = '\0';
        if (*count < 10) {
            strncpy(logs[*count], linea, 1024);
            (*count)++;
        }
    }
}
