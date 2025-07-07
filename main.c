#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include "dashboard.h"

#define MAX_SERVICIOS 10
#define INTERVALO 60
#define THRESHOLD_ALERTAS 3

void mostrar_logs(const char *servicio, const char *prioridad, char logs[10][1024], int count)
{
    if (count == 0)
        return;
    printf("> %s | %s:\n", servicio, prioridad);
    for (int i = 0; i < count; i++)
    {
        printf("  - %s", logs[i]);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Uso: %s servicio1 servicio2 ...\n", argv[0]);
        return 1;
    }

    int num_servicios = argc - 1;
    char *servicios[MAX_SERVICIOS];

    for (int i = 0; i < num_servicios; i++)
    {
        servicios[i] = argv[i + 1];
    }

    while (1)
    {
        time_t t = time(NULL);
        struct tm *tm = localtime(&t);
        printf("\n--- DASHBOARD SIEMLite (%ds) --- %02d/%02d/%04d %02d:%02d:%02d ---\n",
               INTERVALO, tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900,
               tm->tm_hour, tm->tm_min, tm->tm_sec);
        printf("%-12s EMERG ALERT CRIT ERR WARN NOTICE INFO DEBUG\n", "Servicio");
        printf("--------------------------------------------------------------\n");

        for (int i = 0; i < num_servicios; i++)
        {
            int pipefd[2];
            pipe(pipefd);
            pid_t pid = fork();

            if (pid == 0)
            {
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[0]);
                close(pipefd[1]);

                char intervalo[16];
                snprintf(intervalo, sizeof(intervalo), "-%d", INTERVALO);

                execlp("journalctl", "journalctl",
                       "-u", servicios[i],
                       "--since", intervalo,
                       "--no-pager",
                       (char *)NULL);
                perror("execlp");
                exit(1);
            }
            else
            {
                close(pipefd[1]);
                FILE *fp = fdopen(pipefd[0], "r");
                if (!fp)
                {
                    perror("fdopen");
                    continue;
                }

                Estadisticas est = {0};
                char logs[8][10][1024];
                int counts[8] = {0};

                analizar_logs(fp, &est, logs, counts);
                fclose(fp);
                wait(NULL);

                printf("%-12s %5d %5d %4d %3d %4d %6d %4d %5d\n", servicios[i],
                       est.emerg, est.alert, est.crit, est.err,
                       est.warning, est.notice, est.info, est.debug);

                const char *nombres[] = {"emerg", "alert", "crit", "err", "warn", "notice", "info", "debug"};
                for (int p = 0; p < 8; p++)
                {
                    mostrar_logs(servicios[i], nombres[p], logs[p], counts[p]);
                }

                if (est.emerg || est.alert + est.crit > THRESHOLD_ALERTAS)
                {
                    enviar_alerta(servicios[i], est.emerg + est.alert + est.crit);
                }
            }
        }

        sleep(INTERVALO);
    }

    return 0;
}
