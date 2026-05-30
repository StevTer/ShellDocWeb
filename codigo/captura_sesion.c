#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_COMANDO 512
#define MAX_SALIDA 8192

/* Ejecuta un comando Linux y guarda su salida */
void capturarComando(FILE *archivoSesion, const char *comando)
{
    int pipefd[2];

    if (pipe(pipefd) == -1)
    {
        perror("Error al crear pipe");
        return;
    }

    pid_t pid = fork();

    if (pid < 0)
    {
        perror("Error al crear proceso");
        return;
    }

    /* Proceso hijo */
    if (pid == 0)
    {
        close(pipefd[0]);

        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);

        close(pipefd[1]);

        execl("/bin/sh", "sh", "-c", comando, NULL);

        exit(EXIT_FAILURE);
    }

    /* Proceso padre */
    close(pipefd[1]);

    char salida[MAX_SALIDA];

    int bytesLeidos = read(
        pipefd[0],
        salida,
        MAX_SALIDA - 1
    );

    if (bytesLeidos < 0)
        bytesLeidos = 0;

    salida[bytesLeidos] = '\0';

    fprintf(archivoSesion,
            "\n========================================\n");

    fprintf(archivoSesion,
            "COMANDO:\n%s\n\n",
            comando);

    fprintf(archivoSesion,
            "SALIDA:\n%s\n",
            salida);

    fprintf(archivoSesion,
            "========================================\n");

    close(pipefd[0]);

    waitpid(pid, NULL, 0);
}

int main()
{
    FILE *archivoSesion;

    archivoSesion = fopen(
        "sesion_linux.txt",
        "w"
    );

    if (archivoSesion == NULL)
    {
        printf("No se pudo crear sesion_linux.txt\n");
        return 1;
    }

    char comando[MAX_COMANDO];

    printf("====================================\n");
    printf("   CAPTURA DE SESION LINUX\n");
    printf("====================================\n");
    printf("Ingrese comandos Linux.\n");
    printf("Escriba 'exit' para finalizar.\n\n");

    while (1)
    {
        printf("$ ");
        fflush(stdout);

        if (fgets(
                comando,
                sizeof(comando),
                stdin
            ) == NULL)
        {
            break;
        }

        comando[strcspn(
                     comando,
                     "\n")] = '\0';

        if (strlen(comando) == 0)
            continue;

        if (strcmp(comando, "exit") == 0)
            break;

        capturarComando(
            archivoSesion,
            comando
        );
    }

    fclose(archivoSesion);

    printf("\nSesion guardada correctamente en:\n");
    printf("sesion_linux.txt\n");

    return 0;
}
