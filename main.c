#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>
#include "dashboard.h"

#define MAX_SERVICIOS 10

void printHelp(FILE *stream, const char *programa)
{
    fprintf(stream, "Uso: %s [-t intervalo] [-u threshold] servicio1 servicio2 ...\n", programa);
    fprintf(stream, "Opciones:\n");
    fprintf(stream, " -t\tPermite pasar el tiempo de actualización en segundos\n");
    fprintf(stream, " -u\tPermite indicar un threshold para alertas\n");
    fprintf(stream, " -h\tMuestra ayuda\n");
    fprintf(stream, "Por defecto: intervalo = 60s, threshold = 50.\n");
}

int main(int argc, char *argv[])
{
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
               tiempo_intervalo, tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900,
               tm->tm_hour, tm->tm_min, tm->tm_sec);
        printf("%-22s EMERG ALERT CRIT ERR WARN NOTICE INFO DEBUG\n", "Servicio");
        printf("--------------------------------------------------------------\n");

        Estadisticas est[MAX_SERVICIOS] = {0};
        char logs_legibles[MAX_SERVICIOS][10][1024] = {{{0}}};
        int count_legibles[MAX_SERVICIOS] = {0};
        
        for (int i = 0; i < num_servicios; i++) 
        {
            {
                int pipefd[2]; pipe(pipefd);
                pid_t pid = fork();
        
                if (pid == 0) {
                    dup2(pipefd[1], STDOUT_FILENO);
                    close(pipefd[0]); close(pipefd[1]);
        
                    char intervalo_string[16];
                    snprintf(intervalo_string, sizeof(intervalo_string), "-%d", tiempo_intervalo);
        
                    execlp("journalctl", "journalctl",
                           "-u", servicios[i],
                           "--since", intervalo_string,
                           "--no-pager",
                           "-o", "export",
                           (char *)NULL);
                    perror("execlp");
                    exit(1);
                } else {
                    close(pipefd[1]);
                    FILE *fp = fdopen(pipefd[0], "r");
                    if (!fp) { perror("fdopen"); continue; }
                    analizar_logs(fp, &est[i]);
                    fclose(fp);
                    wait(NULL);
                }
            }
        
            {
                int pipefd[2]; pipe(pipefd);
                pid_t pid = fork();
        
                if (pid == 0) {
                    dup2(pipefd[1], STDOUT_FILENO);
                    close(pipefd[0]); close(pipefd[1]);
        
                    char intervalo_string[16];
                    snprintf(intervalo_string, sizeof(intervalo_string), "-%d", tiempo_intervalo);
        
                    execlp("journalctl", "journalctl",
                           "-u", servicios[i],
                           "--since", intervalo_string,
                           "--no-pager",
                           (char *)NULL);
                    perror("execlp");
                    exit(1);
                } else {
                    close(pipefd[1]);
                    FILE *fp = fdopen(pipefd[0], "r");
                    if (!fp) { perror("fdopen"); continue; }
                    obtener_logs_formateados(fp, logs_legibles[i], &count_legibles[i]);
                    fclose(fp);
                    wait(NULL);
                }
            }
        }
        
        for (int i = 0; i < num_servicios; i++) {
            printf("%-22s %5d %5d %4d %3d %4d %6d %4d %5d\n", servicios[i],
                   est[i].emerg, est[i].alert, est[i].crit, est[i].err, est[i].warning,
                   est[i].notice, est[i].info, est[i].debug);
        }
        
        for (int i = 0; i < num_servicios; i++) {
            if (count_legibles[i] > 0) {
                printf("-> %s [ últimos logs ]:\n", servicios[i]);
                for (int j = 0; j < count_legibles[i]; j++) {
                    printf("  - %s\n", logs_legibles[i][j]);
                }
            }
        
            int total = est[i].emerg + est[i].alert + est[i].crit + est[i].err +
                        est[i].warning + est[i].notice + est[i].info + est[i].debug;
        
            if (total >= threshold) {
                enviar_alerta(servicios[i], total);
            }
        }
        sleep(tiempo_intervalo);
    }    

}    
