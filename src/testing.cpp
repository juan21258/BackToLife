/*
    Práctica Sistemas Operativos
    Procesos Reencarnantes
*/
#include <fstream>
#include <string.h>
#include <iostream>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <vector>

#define PATH "/home/juan/pruebaos/src/FichCfg.txt"
#define TRUE 1
#define SEM_NAME "/sem_valSeq"

using namespace std;

/*
conctrl [--ficheroconfiguracion=<rutaficherocfg>]
[--semaforo=<id>]
[--memoriacompartida=<id>]
*/

//Enteros para el pipe
int entrada = 0;
int salida = 1;

//Vector COMPARTIDO donde se guardan los id de los procesos
vector<string>idProcesos;
//Vector COMPARTIDO donde se guarda el path de cada proceso suicida
vector<string>pathSuicida;
//Vector COMPARTIDO donde se guarda el numero de vidas inicial de cada proceso suicida
vector<int>numeroVidas;

vector<vector<string> >comandoHilos;

//Semaforo consola
sem_t mutexCon;

//Semáforos
sem_t * mutexValSeq;
sem_t mutexOut;
sem_t mutexErr;

//Estructura que contiene la informacion del proceso suicida
struct SuicideProcessInfo
{
    char * key;
    char * id;
    char * path;
    char * filename;
    char * lifes;
};

//Estructura que contiene la informacion para las estadísticas de cada proceso suicida
struct InfoMuerte {
    long int seq;
    int nDecesos;
};

//Estructura para la memoria compartida
struct MemoriaCompartida {
    int n = 254; // Numero de procesos controladores
    long int valSeq;
    struct InfoMuerte muertes[254]; // Cada entrada identifica la información de los procesos suicidas.
};

//sizeof(int)*2+sizeof(InfoMuerte)*n;

//Contar lineas del txt
int getLineCount(){
    FILE * file = fopen(PATH, "r");
    if(file == NULL){
        printf("El archivo no existe\n");
        exit(EXIT_FAILURE);
    }
    int count = 0;
    char line[200];
    while(fgets(line,sizeof(line),file) != NULL){
        if (line[0] != '\n'){
            count++;
        }
    }
    fclose(file);   
    return count;
}

struct SuicideProcessInfo setSuicideInfo(char * line){
    struct SuicideProcessInfo info;
    char * tokensArray[5];
    char * limite = "{}:";
    char * token;
    int index = 0;
    token = strtok(line,limite);
    while (token != NULL){
        tokensArray[index] = token;
        token = strtok(NULL, limite);
        index++;
    }
    info.key = tokensArray[0];
    info.id = tokensArray[1];
    info.path = strcat(tokensArray[2],"/");
    info.filename = tokensArray[3];
    info.lifes = tokensArray[4];
    return info;
}


void *HiloConsola(void *id){
    short tid = (short)(intptr_t) id;
    string idP = idProcesos[tid];
    //Iniciar proceso de control
    int pipe1[2];
    int pipe2[2];
    //in 0 out 1
    pipe(pipe1);
    pipe(pipe2);
    if(fork()==0){
        //Hijo
        //Conecta salida del pipe1 a su entrada y la entrada del pipe2 a su salida
        dup2(pipe1[1],entrada);
        dup2(pipe2[0],salida);
        close(pipe2[1]);
        close(pipe1[0]);
        //Inicia el proceso control
        //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<Cambiar direccion
        execl("bin/procesoctrl", "procesoctrl", NULL);
        cerr << "FUck off" << endl << flush;
    }else{
        //Padre
        //Conecta salida del pipe2 a su entrada y la entrada del pipe1 a su salida
        dup2(pipe1[0],salida);
        dup2(pipe2[1],entrada);
        close(pipe1[1]);
        close(pipe2[0]);
    }

    int testing = 0;
    write(salida,&testing,sizeof(testing));
    read(entrada,&testing,sizeof(testing));
    sem_wait(&mutexCon);
    testing ++;
    cerr << "testing " << testing << endl << flush;
    sem_post(&mutexCon);
    return 0;}

int main(int argc, char *argv[], char *env[]){
    int nprocesos;
    string psuicida;
    ifstream infile;
    //TESTING

    idProcesos.push_back("primerSuicida");
    pathSuicida.push_back("/home/suicidas/");
    numeroVidas.push_back(3);

    idProcesos.push_back("segundoSuicida");
    pathSuicida.push_back("/home/suicidas/");
    numeroVidas.push_back(3);

    idProcesos.push_back("tercerSuicida");
    pathSuicida.push_back("/home/suicidas/");
    numeroVidas.push_back(3);

    int nProcesos = 3;
    sem_init(&mutexCon,0,1);
    
    pthread_t hilos[nProcesos];
    for (int i = 0; i < nProcesos; ++i)
    {
        pthread_create(&hilos[i], NULL, HiloConsola, (void *)(intptr_t) i); 
    }
    

    //Espera de comandos de consola
    while(true){ 
        int x;
        cin >> x;
    }

    return 0;
}

