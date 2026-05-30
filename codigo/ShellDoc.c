#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <time.h>

#define MAX_COMANDO 512
#define MAX_RESULTADO 4096
#define ARCHIVO_SALIDA "sesion.qmd"

typedef struct {
    char comando[MAX_COMANDO];
    char resultado[MAX_RESULTADO];
    int numeroComando;
} RegistroComando;

pthread_mutex_t mutexArchivo = PTHREAD_MUTEX_INITIALIZER;
FILE *archivoSesion;

/* Obtiene fecha y hora actual */
void obtenerFechaHora(char *buffer, int tamanio) {
    time_t tiempoActual = time(NULL);
    struct tm *infoTiempo = localtime(&tiempoActual);

    strftime(
        buffer,
        tamanio,
        "%Y-%m-%d %H:%M:%S",
        infoTiempo
    );
}

/* Ejecuta un comando Linux utilizando fork() */
void ejecutarComandoLinux(
    const char *comando,
    char *resultado,
    int maxResultado
) {
    int pipefd[2];

    if (pipe(pipefd) == -1) {
        strcpy(resultado, "Error al crear el pipe.");
        return;
    }

    pid_t pid = fork();

    if (pid < 0) {
        strcpy(resultado, "Error al crear el proceso.");
        return;
    }

    /* Proceso hijo */
    if (pid == 0) {

        close(pipefd[0]);

        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);

        close(pipefd[1]);

        execl("/bin/sh", "sh", "-c", comando, NULL);

        exit(EXIT_FAILURE);
    }

    /* Proceso padre */
    close(pipefd[1]);

    int bytesLeidos = read(
        pipefd[0],
        resultado,
        maxResultado - 1
    );

    if (bytesLeidos < 0) {
        bytesLeidos = 0;
    }

    resultado[bytesLeidos] = '\0';

    close(pipefd[0]);

    waitpid(pid, NULL, 0);
}

/* Función que ejecuta cada hilo */
void *registrarComando(void *arg) {

    RegistroComando *datos = (RegistroComando *)arg;

    ejecutarComandoLinux(
        datos->comando,
        datos->resultado,
        MAX_RESULTADO
    );

    char fechaHora[64];
    obtenerFechaHora(fechaHora, sizeof(fechaHora));

    pthread_mutex_lock(&mutexArchivo);

    fprintf(
        archivoSesion,
        "## Comando %d\n\n",
        datos->numeroComando
    );

    fprintf(
        archivoSesion,
        "**Fecha y hora:** %s\n\n",
        fechaHora
    );

    fprintf(
        archivoSesion,
        "```bash\n$ %s\n```\n\n",
        datos->comando
    );

    fprintf(
        archivoSesion,
        "**Resultado obtenido:**\n\n"
    );

    fprintf(
        archivoSesion,
        "```\n%s\n```\n\n",
        datos->resultado
    );

    fprintf(
        archivoSesion,
        "---\n\n"
    );

    fflush(archivoSesion);

    pthread_mutex_unlock(&mutexArchivo);

    free(datos);

    return NULL;
}

int main(int argc, char *argv[]) {

    char *tituloSesion =
        "Documentacion de Sesion Linux";

    char *autorDocumento =
        "Karen Grefa, Steven Arauz, Geovanni Casa";

    char *nombreArchivo =
        ARCHIVO_SALIDA;

    if (argc >= 2)
        tituloSesion = argv[1];

    if (argc >= 3)
        autorDocumento = argv[2];

    if (argc >= 4)
        nombreArchivo = argv[3];

    archivoSesion = fopen(
        nombreArchivo,
        "w"
    );

    if (archivoSesion == NULL) {

        perror(
            "No se pudo crear el archivo"
        );

        return EXIT_FAILURE;
    }

    char fechaInicio[64];
    obtenerFechaHora(
        fechaInicio,
        sizeof(fechaInicio)
    );

    /* Cabecera Quarto */
    fprintf(archivoSesion, "---\n");
    fprintf(archivoSesion,
            "title: \"%s\"\n",
            tituloSesion);

    fprintf(archivoSesion,
            "author: \"%s\"\n",
            autorDocumento);

    fprintf(archivoSesion,
            "date: \"%s\"\n",
            fechaInicio);

    fprintf(archivoSesion,
            "format: html\n");

    fprintf(archivoSesion, "---\n\n");

    fprintf(
        archivoSesion,
        "# Registro de Sesion Linux\n\n"
    );

    fprintf(
        archivoSesion,
        "Documento generado automaticamente por el sistema ShellDoc.\n\n"
    );

    fprintf(
        archivoSesion,
        "---\n\n"
    );

    pthread_t listaHilos[256];

    int totalHilos = 0;
    int contadorComandos = 1;

    char linea[MAX_COMANDO];

    printf("\n");
    printf("=====================================\n");
    printf("      SHELLDOC WEB - PROYECTO\n");
    printf("=====================================\n");
    printf("Ingrese comandos Linux.\n");
    printf("Escriba 'exit' para finalizar.\n\n");

    while (1) {

        printf("$ ");
        fflush(stdout);

        if (fgets(
                linea,
                sizeof(linea),
                stdin
            ) == NULL)
            break;

        linea[strcspn(linea, "\n")] = '\0';

        if (strlen(linea) == 0)
            continue;

        if (strcmp(linea, "exit") == 0)
            break;

        RegistroComando *nuevoRegistro =
            malloc(sizeof(RegistroComando));

        if (nuevoRegistro == NULL) {

            printf(
                "Error al reservar memoria.\n"
            );

            continue;
        }

        strncpy(
            nuevoRegistro->comando,
            linea,
            MAX_COMANDO - 1
        );

        nuevoRegistro->comando[
            MAX_COMANDO - 1
        ] = '\0';

        nuevoRegistro->numeroComando =
            contadorComandos++;

        if (pthread_create(
                &listaHilos[totalHilos],
                NULL,
                registrarComando,
                nuevoRegistro
            ) == 0) {

            totalHilos++;
        }
        else {

            printf(
                "Error al crear hilo.\n"
            );

            free(nuevoRegistro);
        }
    }

    for (int i = 0; i < totalHilos; i++) {

        pthread_join(
            listaHilos[i],
            NULL
        );
    }

    char fechaFin[64];
    obtenerFechaHora(
        fechaFin,
        sizeof(fechaFin)
    );

    fprintf(
        archivoSesion,
        "# Resumen de la sesion\n\n"
    );

    fprintf(
        archivoSesion,
        "- Fecha de cierre: %s\n",
        fechaFin
    );

    fprintf(
        archivoSesion,
        "- Total de comandos ejecutados: **%d**\n\n",
        contadorComandos - 1
    );

    fclose(archivoSesion);

    pthread_mutex_destroy(
        &mutexArchivo
    );

    printf("\n");
    printf("-------------------------------------\n");
    printf("Sesion documentada correctamente\n");
    printf("Archivo generado: %s\n",
           nombreArchivo);

    printf("\nPara generar la pagina web:\n");
    printf(
        "quarto render %s\n",
        nombreArchivo
    );

    printf("-------------------------------------\n");

    return 0;
}
