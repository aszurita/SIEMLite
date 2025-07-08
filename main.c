#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>
#include "dashboard.h"

#define MAX_SERVICIOS 10

void mostrar_logs(const char *servicio, const char *prioridad, char logs[10][1024], int count)
{
    if (count == 0)
        return;
    printf("-> %s [ %s ]:\n", servicio, prioridad);
    for (int i = 0; i < count; i++)
    {
        printf("  - %s\n", logs[i]);
    }
}
void printHelp(FILE *stream, const char *programa)
{
    fprintf(stream, "Uso: %s [-t intervalo] [-u threshold] servicio1 servicio2 ...\n", programa);
    fprintf(stream, "Opciones:\n");
    fprintf(stream, " -t	Permite pasar el tiempo de actualización de la información en segundos\n");
    fprintf(stream, " -u	Permite indicar un threshold para alertas\n");
    fprintf(stream, " -h	Muestra ayuda\n");
    fprintf(stream, "En caso de no pasar ninguna opción el tiempo por defecto es 60 segundos y el threshold es 5.\n");
}

int main(int argc, char *argv[])
{
    /*Por defecto cada minuto*/
    int tiempo_intervalo = 60;
    int threshold = 50;
    int opt;

    while ((opt = getopt(argc, argv, "t:u:h")) != -1)
    {
        switch (opt)
        {
        case 't':
        {
            char *endptr;
            errno = 0;
            long temp = strtol(optarg, &endptr, 10);
            if (errno != 0 || *endptr != '\0' || temp <= 0)
            {
                fprintf(stderr, "Error: Intervalo inválido '%s'. Debe ser un número entero positivo.\n", optarg);
                return 1;
            }
            tiempo_intervalo = (int)temp;
            break;
        }
        case 'u':
        {
            char *endptr;
            errno = 0;
            long temp = strtol(optarg, &endptr, 10);
            if (errno != 0 || *endptr != '\0' || temp <= 0)
            {
                fprintf(stderr, "Error: Umbral inválido '%s'. Debe ser un número entero positivo.\n", optarg);
                return 1;
            }
            threshold = (int)temp;
            break;
        }
        case 'h':
            printHelp(stdout, argv[0]);
            return 0;
        default:
            printHelp(stderr, argv[0]);
            return 1;
        }
    }

    int num_servicios = argc - optind;
    if (num_servicios < 2)
    {
        fprintf(stderr, "Debes especificar al menos 2 servicios a monitorear.\n");
        return 1;
    }

    char *servicios[MAX_SERVICIOS];
    for (int i = 0; i < num_servicios; i++)
    {
        servicios[i] = argv[optind + i];
    }

    while (1)
    {
        time_t t = time(NULL);
        struct tm *tm = localtime(&t);
        printf("\n--- DASHBOARD SIEMLite (%ds) --- %02d/%02d/%04d %02d:%02d:%02d ---\n",
               tiempo_intervalo, tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900, tm->tm_hour, tm->tm_min, tm->tm_sec);
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

                char intervalo_string[15];
                snprintf(intervalo_string, sizeof(intervalo_string), "-%d", tiempo_intervalo);

                execlp("journalctl", "journalctl",
                       "-u", servicios[i],
                       "--since", intervalo_string,
                       "--no-pager",
                       "-o", "export",
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
                       est.emerg, est.alert, est.crit, est.err, est.warning, est.notice, est.info, est.debug);

                const char *nombres[] = {"emerg", "alert", "crit", "err", "warn", "notice", "info", "debug"};
                for (int p = 0; p < 8; p++)
                {
                    mostrar_logs(servicios[i], nombres[p], logs[p], counts[p]);
                }
                int total_alertas = est.emerg + est.alert + est.crit + est.err + est.warning + est.notice + est.info + est.debug;
                if (total_alertas >= threshold)
                {
                    enviar_alerta(servicios[i], total_alertas);
                }
            }
        }

        sleep(tiempo_intervalo);
    }

    return 0;
}