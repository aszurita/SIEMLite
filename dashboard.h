#ifndef DASHBOARD_H
#define DASHBOARD_H

typedef struct {
    int emerg, alert, crit, err, warning, notice, info, debug;
} Estadisticas;

void enviar_alerta(const char *servicio, int total);
void analizar_logs(FILE *fp, Estadisticas *est);
void obtener_logs_formateados(FILE *fp, char logs[10][1024], int *count);

#endif
