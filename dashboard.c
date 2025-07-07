#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "dashboard.h"

#define INTERVALO_ACTUALIZACION 60

void ejecutar_journalctl(const char *servicio, Estadisticas *est)
{
  char comando[256];
  snprintf(comando, sizeof(comando),
           "journalctl -u %s.service --since '-%d seconds' --no-pager",
           servicio, INTERVALO_ACTUALIZACION);

  FILE *fp = popen(comando, "r");
  if (!fp)
  {
    perror("popen");
    exit(1);
  }

  char linea[1024];
  while (fgets(linea, sizeof(linea), fp))
  {
    // Convertir a minúsculas para comparación más robusta
    char linea_lower[1024];
    strcpy(linea_lower, linea);
    for (int i = 0; linea_lower[i]; i++)
    {
      linea_lower[i] = tolower(linea_lower[i]);
    }

    // Detectar todos los niveles de prioridad
    if (strstr(linea_lower, "emerg") || strstr(linea_lower, "emergency"))
      est->emerg++;
    else if (strstr(linea_lower, "alert"))
      est->alert++;
    else if (strstr(linea_lower, "crit") || strstr(linea_lower, "critical"))
      est->crit++;
    else if (strstr(linea_lower, "err") || strstr(linea_lower, "error"))
      est->err++;
    else if (strstr(linea_lower, "warn") || strstr(linea_lower, "warning"))
      est->warning++;
    else if (strstr(linea_lower, "notice"))
      est->notice++;
    else if (strstr(linea_lower, "info"))
      est->info++;
    else if (strstr(linea_lower, "debug"))
      est->debug++;
  }
  pclose(fp);
}

void enviar_alerta(const char *servicio, int alertas)
{
  char mensaje[256];
  snprintf(mensaje, sizeof(mensaje),
           "./enviar_alerta.py '¡ALERTA! %s tuvo %d alertas en %ds'",
           servicio, alertas, INTERVALO_ACTUALIZACION);
  system(mensaje);
}
