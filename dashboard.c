#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <curl/curl.h>
#include "dashboard.h"

void enviar_alerta(const char *servicio, int total)
{
    printf("\n\n**************************\n");
    printf("!!! ALERTA: El servicio '%s' ha superado el umbral con %d mensajes críticos !!!\n", servicio, total);
    printf("!!! INTENTANDO ENVIAR ALERTA POR WHATSAPP... !!!\n");
    printf("************************\n\n");

    const char *account_sid = "ACe6e033c1ba04ca9b69925eea526f2972";
    const char *auth_token = "ccaebea261491bf011a4aa320d753c8d";
    const char *from_whatsapp_number = "whatsapp:+14155238886";
    const char *to_whatsapp_number   = "whatsapp:+5939888035770";
    
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if (curl) {
        char url[256];
        sprintf(url, "https://api.twilio.com/2010-04-01/Accounts/%s/Messages.json", account_sid);

        // Escapar los valores
        char *escaped_to = curl_easy_escape(curl, to_whatsapp_number, 0);
        char *escaped_from = curl_easy_escape(curl, from_whatsapp_number, 0);

        char body_text[256];
        sprintf(body_text, "ALERTA SIEMLite: El servicio '%s' ha generado %d errores críticos.", servicio, total);
        char *escaped_body = curl_easy_escape(curl, body_text, 0);

        char post_data[512];
        sprintf(post_data, "To=%s&From=%s&Body=%s", escaped_to, escaped_from, escaped_body);

        // Limpiar cadenas escapadas
        curl_free(escaped_to);
        curl_free(escaped_from);
        curl_free(escaped_body);

        // Configuración CURL
        curl_easy_setopt(curl, CURLOPT_URL, url);

        char userpwd[256];
        sprintf(userpwd, "%s:%s", account_sid, auth_token);
        curl_easy_setopt(curl, CURLOPT_USERPWD, userpwd);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);

        // Ejecutar
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "Error al enviar la alerta de WhatsApp: %s\n", curl_easy_strerror(res));
        } else {
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            printf("Alerta de WhatsApp enviada. Código de respuesta HTTP: %ld\n", http_code);
        }

        curl_easy_cleanup(curl);
    }
}


void analizar_logs(FILE *fp, Estadisticas *est, char logs[8][10][1024], int counts[8]) {

  char linea[1024];
  int prioridad_actual = -1;
  char timestamp[128] = "", hostname[128] = "", ident[128] = "", pid[32] = "", mensaje[1024] = "";

  while (fgets(linea, sizeof(linea), fp)) {
      if (strncmp(linea, "PRIORITY=", 9) == 0) {
        prioridad_actual = atoi(linea + 9);
      } else if (strncmp(linea, "_HOSTNAME=", 10) == 0) {
        strncpy(hostname, linea + 10, sizeof(hostname));
        hostname[strcspn(hostname, "\n")] = '\0';
      } else if (strncmp(linea, "SYSLOG_IDENTIFIER=", 18) == 0) {
        strncpy(ident, linea + 18, sizeof(ident));
        ident[strcspn(ident, "\n")] = '\0';
      } else if (strncmp(linea, "MESSAGE=", 8) == 0) {
        strncpy(mensaje, linea + 8, sizeof(mensaje));
        mensaje[strcspn(mensaje, "\n")] = '\0';
      }else if (strncmp(linea, "SYSLOG_TIMESTAMP=", 17) == 0) {
        strncpy(timestamp, linea + 17, sizeof(timestamp));
        timestamp[strcspn(timestamp, "\n")] = '\0';
      }else if (strncmp(linea, "SYSLOG_PID=", 11) == 0) {
        strncpy(pid, linea + 11, sizeof(pid));
        pid[strcspn(pid, "\n")] = '\0';
      }else if (strncmp(linea, "_SOURCE_REALTIME_TIMESTAMP=", 27) == 0) { 
          /* Todo log termina con esa linea _SOURCE_REALTIME_TIMESTAMP= */

          if (prioridad_actual >= 0 && prioridad_actual <= 7 && counts[prioridad_actual] < 10) {
              int *contador = &est->emerg + prioridad_actual;
              (*contador)++;

              snprintf(logs[prioridad_actual][counts[prioridad_actual]++], 1024,
                      "%s %s %s[%s]: %s",
                      timestamp, hostname, ident, pid, mensaje);
          }
          /*Inicio de un nuevo log*/
          prioridad_actual = -1;
          timestamp[0] = hostname[0] = ident[0] = pid[0] = mensaje[0] = '\0';
      }
  }
}
