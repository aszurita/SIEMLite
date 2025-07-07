#ifndef DASHBOARD_H
#define DASHBOARD_H
#include <stdio.h>
typedef struct
{
    int emerg;
    int alert;
    int crit;
    int err;
    int warning;
    int notice;
    int info;
    int debug;
} Estadisticas;

void analizar_logs(FILE *fp, Estadisticas *est, char logs[8][10][1024], int counts[8]);
void enviar_alerta(const char *servicio, int total_alertas);

#endif
