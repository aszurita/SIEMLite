#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "dashboard.h"

#define MAX_SERVICIOS 10
#define INTERVALO_ACTUALIZACION 5
#define THRESHOLD_ALERTAS 3

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Uso: %s servicio1 servicio2 ...\n", argv[0]);
        exit(1);
    }

    int num_servicios = argc - 1;
    char *servicios[MAX_SERVICIOS];

    for (int i = 0; i < num_servicios; i++)
    {
        servicios[i] = argv[i + 1];
    }

    while (1)
    {
        printf("====== DASHBOARD (%ds) ======\n", INTERVALO_ACTUALIZACION);

        for (int i = 0; i < num_servicios; i++)
        {
            int pipefd[2];
            pipe(pipefd);
            pid_t pid = fork();

            if (pid == 0)
            {
                close(pipefd[0]);
                Estadisticas est = {0, 0, 0, 0, 0, 0, 0, 0};
                ejecutar_journalctl(servicios[i], &est);
                write(pipefd[1], &est, sizeof(est));
                close(pipefd[1]);
                exit(0);
            }
            else
            {
                close(pipefd[1]);
                Estadisticas est;
                read(pipefd[0], &est, sizeof(est));
                close(pipefd[0]);
                wait(NULL);

                printf("Servicio: %s\n", servicios[i]);
                printf("[EMERG] %d\n", est.emerg);
                printf("[ALERT] %d\n", est.alert);
                printf("[CRIT ] %d\n", est.crit);
                printf("[ERR  ] %d\n", est.err);
                printf("[WARN ] %d\n", est.warning);
                printf("[NOTIC] %d\n", est.notice);
                printf("[INFO ] %d\n", est.info);
                printf("[DEBUG] %d\n", est.debug);
                printf("------------------------\n");

                // Alerta si hay emergencias, alertas o críticos
                if (est.emerg > 0 || est.alert >= THRESHOLD_ALERTAS || est.crit > 0)
                {
                    enviar_alerta(servicios[i], est.alert + est.emerg + est.crit);
                }
            }
        }

        printf("==============================\n");
        sleep(INTERVALO_ACTUALIZACION);
    }

    return 0;
}
