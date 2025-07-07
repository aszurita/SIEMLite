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
  while (fgets(linea, sizeof(linea), fp))
  {
    char temp[1024];
    strcpy(temp, linea);
    for (int i = 0; temp[i]; i++)
      temp[i] = tolower(temp[i]);

    if (strstr(temp, "emerg"))
    {
      est->emerg++;
      if (counts[0] < 10)
        strcpy(logs[0][counts[0]++], linea);
    }
    else if (strstr(temp, "alert"))
    {
      est->alert++;
      if (counts[1] < 10)
        strcpy(logs[1][counts[1]++], linea);
    }
    else if (strstr(temp, "crit"))
    {
      est->crit++;
      if (counts[2] < 10)
        strcpy(logs[2][counts[2]++], linea);
    }
    else if (strstr(temp, "err"))
    {
      est->err++;
      if (counts[3] < 10)
        strcpy(logs[3][counts[3]++], linea);
    }
    else if (strstr(temp, "warn"))
    {
      est->warning++;
      if (counts[4] < 10)
        strcpy(logs[4][counts[4]++], linea);
    }
    else if (strstr(temp, "notice"))
    {
      est->notice++;
      if (counts[5] < 10)
        strcpy(logs[5][counts[5]++], linea);
    }
    else if (strstr(temp, "info"))
    {
      est->info++;
      if (counts[6] < 10)
        strcpy(logs[6][counts[6]++], linea);
    }
    else if (strstr(temp, "debug"))
    {
      est->debug++;
      if (counts[7] < 10)
        strcpy(logs[7][counts[7]++], linea);
    }
  }
}
