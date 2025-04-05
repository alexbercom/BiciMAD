// Condition time wait
// Revisar que todas las modificaciones de variables esten protegidas
//unlock

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>

typedef struct configuracion {
    int usuarios;           //Usuarios totales
    int estaciones;         //Número de estaciones
    int huecos;             //Número de huecos por estación
    int min_tomar;          //Tiempo mínimo de espera para decidir tomar una bici
    int max_tomar;          //Tiempo máximo de espera para decidir tomar una bici
    int min_montar;         //Tiempo mínimo que pasa un usuario montando una bici
    int max_montar;         //Tiempo máximo que pasa un usuario montando una bici
    int min_paseos;         //Número mínimo de paseos en bici que dará dar cualquier usuario
    int max_paseos;         //Número máximo de paseos en bici que puede dar cualquier usuario
} tConfig;

pthread_t *th;                      //cada usuario es un thread
pthread_mutex_t mutex;              //mutex para controlar los accesos a las estaciones
pthread_cond_t *cond_dejar;         //condicion que servirá para avisar de que hay un hueco en una estacion
pthread_cond_t *cond_coger;         //condicion que servirá para avisar de que hay una bici en una estacion
tConfig *config;
struct timespec ts;                 //estructura para manejo de esperas por tiempo

FILE *entrada;
FILE *salida;

int usuarios_activos;       //variable que contará el número de usuarios que no han finalizado
int usuarios_esperando;     //variable que contará el número de usuarios que hay esperando a poder realizar una accion
int *usuarios_esperando_cond;  //array que contara el número de hilos esperando condicion en cada estacion
int *bicis_por_estacion;        //array que cuenta el número de bicis disponibles por estacion

//Declaracion de cabeceras
char * obtener_fechayhora();
void asignar_entradasalida(int argc, char *argv[]);
void leer_entrada();
void imprimir(const char *format, ...);
void mostrar_config();

//Funcion de los hilos
void * th_func(void *args) {
    int i = *((int *) args);
    int tiempo_tomar_rand;      //variables para determinar una duracion aleatoria dentro del intervalo
    int tiempo_montar_rand;
    int num_paseos_rand;
    int estacion_rand;      //variable que se usara para elegir una estacion aleatoria

    //Se establece el número de paseos que dara el usuario de forma aleatoria
    num_paseos_rand = config->min_paseos + rand() % (config->max_paseos - config->min_paseos + 1);

    for (int j=0; j < num_paseos_rand; j++) {
        // Se elige de manera aleatoria la duracion de cada accion y la estacion en cada paseo
        tiempo_tomar_rand = config->min_tomar + rand() % (config->max_tomar - config->min_tomar + 1);
        tiempo_montar_rand = config->min_montar + rand() % (config->max_montar - config->min_montar + 1);
        estacion_rand = rand() % config->estaciones;

        // Empieza el proceso de transito del usuario
        sleep(tiempo_tomar_rand);   //espera el tiempo establecido antes de tomar la bici
        imprimir("Usuario %d quiere coger bici de estacion %d\n", i, estacion_rand);

        // Coger una bici
        int bici_cogida = 0;
        while (bici_cogida == 0) {
            pthread_mutex_lock(&mutex);  //para coger una bici se bloquea la estacion

            // Si hay bicis disponibles
            if (bicis_por_estacion[estacion_rand] > 0) {
                bicis_por_estacion[estacion_rand]--;    //se coge una bici
                // Si la estacion estaba llena
                if (bicis_por_estacion[estacion_rand] == config->huecos - 1)
                    pthread_cond_signal(&cond_dejar[estacion_rand]);  //avisa de que ahora hay un hueco disponible
                bici_cogida = 1;
            }
            // Si no hay bicis disponibles
            else {
                usuarios_esperando++;
                printf("\t\t\t\tUsuarios esperando: %d de %d\n", usuarios_esperando, usuarios_activos);
                // Si todos están esperando elige otra estacion
                if (usuarios_esperando == usuarios_activos) {
                    do {    // Elige otra estación que tenga huecos disponibles
                        estacion_rand = rand() % config->estaciones;
                    } while (bicis_por_estacion[estacion_rand] == 0);
                }
                // Si todavía hay usuarios en movimiento
                else {
                    usuarios_esperando_cond[estacion_rand]++;
                    // Define el tiempo de espera (por ejemplo, 5 segundos)
                    clock_gettime(CLOCK_REALTIME, &ts);
                    ts.tv_sec += 10;
                    //
                    int ret = pthread_cond_timedwait(&cond_coger[estacion_rand], &mutex, &ts);
                    if (ret == ETIMEDOUT) {
                        // La espera terminó porque se alcanzó el tiempo límite
                        printf("Tiempo de espera alcanzado para %d en la estación %d.\n", i, estacion_rand);
                        estacion_rand = rand() % config->estaciones;    //cambia de estacion si se ha cumplido el tiempo
                        // Aquí puedes agregar cualquier lógica adicional que necesites en caso de tiempo de espera
                    } else if (ret != 0) {
                        // Otro error
                        printf("Error en pthread_cond_timedwait: %d\n", ret);
                        // Aquí puedes manejar el error de forma adecuada
                    }
                    //
                    usuarios_esperando_cond[estacion_rand]--;
                }
                usuarios_esperando--;
            }
            pthread_mutex_unlock(&mutex);
        }

        imprimir("Usuario %d coge bici de estacion %d\n", i, estacion_rand);

        imprimir("Usuario %d montando bici...\n", i);
        sleep(tiempo_montar_rand);  //espera un tiempo mientras monta

        estacion_rand = rand() % config->estaciones;  //se elige una estacion para dejar
        imprimir("Usuario %d quiere dejar bici en estacion %d\n", i, estacion_rand);

        // Dejar una bici
        int bici_dejada = 0;
        while (bici_dejada == 0) {
            pthread_mutex_lock(&mutex);  //para dejar una bici se bloquea la estacion

            // Si hay hueco
            if (bicis_por_estacion[estacion_rand] < config->huecos) {
                bicis_por_estacion[estacion_rand]++;
                // Si estaba vacía
                if (bicis_por_estacion[estacion_rand] == 1)
                    pthread_cond_signal(&cond_coger[estacion_rand]);  //avisa de que ahora hay una bici disponible
                bici_dejada = 1;
            }
            // Si está llena
            else {
                usuarios_esperando++;
                printf("\t\t\t\tUsuarios esperando: %d de %d\n", usuarios_esperando, usuarios_activos);
                // Si todos están esperando elige otra estacion
                if (usuarios_esperando == usuarios_activos) {
                    do {    // Elige otra estación que tenga huecos disponibles
                        estacion_rand = rand() % config->estaciones;
                    } while (bicis_por_estacion[estacion_rand] == config->huecos);
                }
                // Si todavía hay usuarios en movimiento
                else {
                    usuarios_esperando_cond[estacion_rand]++;
                    //pthread_cond_wait(&cond_dejar[estacion_rand], &mutex);
                    //
                    // Define el tiempo de espera (por ejemplo, 5 segundos)
                    clock_gettime(CLOCK_REALTIME, &ts);
                    ts.tv_sec += 10;

                    int ret = pthread_cond_timedwait(&cond_dejar[estacion_rand], &mutex, &ts);
                    if (ret == ETIMEDOUT) {
                        // La espera terminó porque se alcanzó el tiempo límite
                        printf("Tiempo de espera alcanzado para %d en la estación %d.\n", i, estacion_rand);
                        // Aquí puedes agregar cualquier lógica adicional que necesites en caso de tiempo de espera
                    } else if (ret != 0) {
                        // Otro error
                        printf("Error en pthread_cond_timedwait: %d\n", ret);
                        // Aquí puedes manejar el error de forma adecuada
                    }
                    estacion_rand = rand() % config->estaciones;
                    //
                    usuarios_esperando_cond[estacion_rand]--;
                }
                usuarios_esperando--;
            }
            pthread_mutex_unlock(&mutex);
        }

        imprimir("Usuario %d deja bici en estacion %d\n", i, estacion_rand);
    }

    pthread_mutex_lock(&mutex);
    usuarios_activos--;
    pthread_mutex_unlock(&mutex);

    //Al terminar, si todos los restantes están bloqueados, manda una señal a la primera estacion que tenga usuarios esperando
    if ((usuarios_esperando == usuarios_activos) && (usuarios_activos > 0)) {
        //Se van revisando las estaciones hasta que se encuentre alguna con alguien esperando condicion y le envia la señal
        for (int k = 0; k < config->estaciones; k++)
            if (usuarios_esperando_cond[k] > 0) {
                pthread_mutex_lock(&mutex);
                pthread_cond_signal(&cond_coger[k]);
                pthread_cond_signal(&cond_dejar[k]);
                pthread_mutex_unlock(&mutex);
                break;
            }
    }

    pthread_exit(NULL);
    return NULL;
}

//MAIN//
int main(int argc, char *argv[]) {
    //Se establece la semilla para generacion de aleatorios
    srand(time(NULL));

    //Se asigan los ficheros de entrada y salida correspondientes
    asignar_entradasalida(argc, argv);

    //Se crea, asigna y muestra la configuracion
    config = (tConfig *)malloc(sizeof(tConfig));
    leer_entrada();
    mostrar_config();

    imprimir("\nSIMULACIÓN DE FUNCIONAMIENTO DE BiciMAD\n\n");

    //Se crean las variables que contarán el número de usuarios activos, esperando y esperando condicion en cada estacion
    usuarios_activos = config->usuarios;
    usuarios_esperando = 0;
    usuarios_esperando_cond = (int *) malloc(config->estaciones * sizeof (int));
    bicis_por_estacion = (int *) malloc(config->estaciones * sizeof (int));

    //Creacion de mutex y condiciones
    cond_coger = (pthread_cond_t *) malloc(config->estaciones * sizeof(pthread_cond_t));
    cond_dejar = (pthread_cond_t *) malloc(config->estaciones * sizeof(pthread_cond_t));
    pthread_mutex_init(&mutex, NULL);

    for (int i = 0; i < config->estaciones; i++) {
        // Se crean los mutex y condiciones
        pthread_cond_init(&cond_coger[i], NULL);
        pthread_cond_init(&cond_dejar[i], NULL);
        // Se establece el número de bicis a 3/4 del número de huecos y usuarios esperando a 0
        bicis_por_estacion[i] = config->huecos * 3 / 4;
        usuarios_esperando_cond[i] = 0;
    }

    //Creacion de threads
    th = (pthread_t *)malloc(config->usuarios * sizeof(pthread_t));

    for (int i = 0; i < config->usuarios; i++) {
        int *index = malloc(sizeof(int));
        *index = i;
        pthread_create(&th[i], NULL, th_func, (void *) index);  //numero de usuario como argumento
    }

    //Aqui se detendrá el main hasta que los hilos terminen
    for (int i = 0; i < config->usuarios; i++) {
        pthread_join(th[i], NULL);
    }

    imprimir("\nSIMULACIÓN TERMINADA\n");

    //Cerrar mutex y condicion
    pthread_mutex_destroy(&mutex);

    for (int i = 0; i < config->estaciones; i++) {
        pthread_cond_destroy(&cond_coger[i]);
        pthread_cond_destroy(&cond_dejar[i]);
    }
    
    //Se libera la memoria
    free(cond_coger);
    free(cond_dejar);
    free(usuarios_esperando_cond);
    free(bicis_por_estacion);
    free(th);
    free(config);

    //Se cierran los ficheros
    fclose(entrada);
    fclose(salida);

    exit(0);
}

//SUBPROGRAMAS//

//Funcion que devuelve fecha y hora formateada
char * obtener_fechayhora() {
    char *buffer = (char *) malloc(20);
    time_t t;
    struct tm* tiempo;

    time(&t);
    tiempo = localtime(&t);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H-%M_", tiempo);

    return buffer;
}

//Funcion que asigna los archivos de entrada y salida en base a los argumentos del programa
void asignar_entradasalida(int argc, char *argv[]) {
    char *nombre_salida;

    //0 parametros (entrada y salida predeterminadas)
    if (argc == 1) {
        entrada = fopen("entrada_BiciMAD.txt", "r");
        nombre_salida = strcat(obtener_fechayhora(), "salida_BiciMAD.txt");
        salida = fopen(nombre_salida, "w");
    }
    //1 parametro (entrada argv 1 y salida predeterminada)
    else if (argc == 2) {
        entrada = fopen(argv[1], "r");
        nombre_salida = strcat(obtener_fechayhora(), "salida_BiciMAD.txt");
        salida = fopen(nombre_salida, "w");
    }
    //2 parametros (entrada argv 1 y salida argv 2)
    else if (argc == 3) {
        entrada = fopen(argv[1], "r");
        nombre_salida = strcat(obtener_fechayhora(), argv[2]);
        salida = fopen(nombre_salida, "w");
    }
    //Numero de parametros incorrecto
    else {
        printf("Numero de parametros incorrecto. Uso: %s [fichero_entrada] [fichero_salida]\n", argv[0]);
        exit(1);
    }

    //Si falla la apertura de algun fichero
    if (entrada == NULL || salida == NULL) {
        perror("Error al abrir archivos");
        exit(EXIT_FAILURE);
    }

    free(nombre_salida);
}

//Lee el fichero de entrada y aplica la configuracion
void leer_entrada() {
    // Definir un array de punteros a int para facilitar la asignación
    int *campos[] = {&config->usuarios,
                     &config->estaciones,
                     &config->huecos,
                     &config->min_tomar,
                     &config->max_tomar,
                     &config->min_montar,
                     &config->max_montar,
                     &config->min_paseos,
                     &config->max_paseos };

    // Leer cada línea del archivo y almacenar los datos en el struct
    char linea[10];
    for (int i = 0; i < 9; i++) {
        if (fgets(linea, sizeof(linea), entrada) != NULL) {
            *campos[i] = atoi(linea);
            if (*campos[i] == '\0') {
                printf("Error: La línea %d está vacía.\n", i + 1);
                fclose(entrada);
                fclose(salida);
                exit(1);
            }
        } else {
            printf("Error: El archivo no tiene las suficientes lineas.\n");
            exit(1);
        }
    }

    //Comprobar que son datos validos (minimos >= 1 o minimos < maximos)
    if ((*campos[3] < 1) || (*campos[3] > *campos[4]) ||
        (*campos[5] < 1) || (*campos[5] > *campos[6]) ||
        (*campos[7] < 1) || (*campos[7] > *campos[8]) ) {
        fprintf(stderr, "Datos de entrada no validos: Minimos menores a 1 o mayores que los maximos.\n");
        exit(1);
    }
}

//Funcion para imprimir por consola y por el fichero de salida a la vez
void imprimir(const char *format, ...) {
    //Se han utilizado va_list para poder tener una lista de argumentos como parametro
    va_list args;

    // Imprimir en consola
    va_start(args, format);     //inicializa la lista de argumentos
    vprintf(format, args);      //imprime por consola el formato con sus argumentos
    va_end(args);           //finaliza la lista de argumentos

    // Imprimir en archivo
    va_start(args, format);
    vfprintf(salida, format, args);     //imprime en el archivo de salida
    va_end(args);
}

//Muestra por pantalla la configuracion
void mostrar_config() {
    imprimir("BiciMAD: CONFIGURACIÓN INICIAL\n");
    imprimir("Usuarios: %d\n", config->usuarios);
    imprimir("Número de Estaciones: %d\n", config->estaciones);
    imprimir("Número de huecos por estación: %d\n", config->huecos);
    imprimir("Tiempo mínimo de espera para decidir tomar una bici: %d\n", config->min_tomar);
    imprimir("Tiempo máximo de espera para decidir tomar una bici: %d\n", config->max_tomar);
    imprimir("Tiempo mínimo que pasa un usuario montando una bici: %d\n", config->min_montar);
    imprimir("Tiempo máximo que puede pasar un usuario montando una bici: %d\n", config->max_montar);
    imprimir("Número mínimo de paseos en bici que da un usuario: %d\n", config->min_paseos);
    imprimir("Número máximo de paseos en bici que puede dar un usuario: %d\n", config->max_paseos);
}