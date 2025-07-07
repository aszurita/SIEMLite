#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "dashboard.h"

void enviar_alerta(const char *servicio, int total)
{
  printf(">>> ALERTA: Servicio %s tuvo %d eventos críticos.\n", servicio, total);
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
      }else if (strncmp(linea, "_SOURCE_REALTIME_TIMESTAMP=", 27) == 0) { // Final

          if (prioridad_actual >= 0 && prioridad_actual <= 7 && counts[prioridad_actual] < 10) {
              int *contador = &est->emerg + prioridad_actual;
              (*contador)++;

              snprintf(logs[prioridad_actual][counts[prioridad_actual]++], 1024,
                      "%s %s %s[%s]: %s",
                      timestamp, hostname, ident, pid, mensaje);
          }

          // Reset para siguiente entrada
          prioridad_actual = -1;
          timestamp[0] = hostname[0] = ident[0] = pid[0] = mensaje[0] = '\0';
      }
  }
}
