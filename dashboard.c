#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "dashboard.h"

void enviar_alerta(const char *servicio, int total)
{
  printf(">>> ALERTA: Servicio %s tuvo %d eventos críticos.\n", servicio, total);
}

void analizar_logs(FILE *fp, Estadisticas *est, char logs[8][10][1024], int counts[8])
{
  char linea[1024];
  int prioridad_actual = -1;

  while (fgets(linea, sizeof(linea), fp))
  {
    if (strncmp(linea, "PRIORITY=", 9) == 0)
    {
      prioridad_actual = atoi(linea + 9);
      if (prioridad_actual >= 0 && prioridad_actual <= 7)
      {
        int *contador = &est->emerg + prioridad_actual;
        (*contador)++;
        if (counts[prioridad_actual] < 10)
        {
          logs[prioridad_actual][counts[prioridad_actual]][0] = '\0'; // limpia
        }
      }
    }
    else if (strncmp(linea, "MESSAGE=", 8) == 0 && prioridad_actual >= 0 && prioridad_actual <= 7)
    {
      if (counts[prioridad_actual] < 10)
      {
        strncpy(logs[prioridad_actual][counts[prioridad_actual]], linea + 8, 1023);
        logs[prioridad_actual][counts[prioridad_actual]][strcspn(logs[prioridad_actual][counts[prioridad_actual]], "\n")] = '\0';
        counts[prioridad_actual]++;
      }
    }
  }
}
