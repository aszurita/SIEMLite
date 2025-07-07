#ifndef DASHBOARD_H
#define DASHBOARD_H

typedef struct
{
    int emerg;   // 0 - Emergency
    int alert;   // 1 - Alert
    int crit;    // 2 - Critical
    int err;     // 3 - Error
    int warning; // 4 - Warning
    int notice;  // 5 - Notice
    int info;    // 6 - Informational
    int debug;   // 7 - Debug
} Estadisticas;

void ejecutar_journalctl(const char *servicio, Estadisticas *est);
void enviar_alerta(const char *servicio, int alertas);

#endif
