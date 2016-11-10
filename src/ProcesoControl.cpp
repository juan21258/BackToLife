/*
	Práctica Sistemas Operativos
	Procesos Reencarnantes
*/
#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>  
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <getopt.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#define TRUE 1

using namespace std;

//Estructura que contiene la informacion para las estadísticas de cada proceso suicida
struct InfoMuerte {
	long int seq;
	int nDecesos;
};
/*
//Estructura para la memoria compartida
struct MemoriaCompartida {
	int n; // Numero de procesos controladores
	long int valSeq;
	struct InfoMuerte muertes[n]; // Cada entrada identifica la información de los procesos suicidas.
};

sizeof(int)*2+sizeof(InfoMuerte)*n;
*/
//Variables para los pipes
int entrada = 0;
int salida = 1;

//Variables compartidas
string rutafichero;
string filename;
int reencarnacion;
int idProcesoControl;
bool murio = false;

sem_t mutex;

void* HiloControl(void*){
    bool sigueVivo = true;
    while(sigueVivo){
        int idHijo = fork();
        if(idHijo==0){
            //Hijo, crear nuevo suicida
            sem_wait(&mutex);
            string ruta = rutafichero+filename;
            string nombre = filename;
            sem_post(&mutex);
            execl(ruta.c_str(),nombre.c_str(),NULL);
            cerr << 
            "Error ejecutando el proceso suicida (filepath o filename)"
            << endl << flush;
        }else{
            //Padre, esperar a que el hijo se suicide
            int status;
            waitpid(idHijo,&status,0);
            //Si llego hasta aqui, significa que el hijo se suicido
            sem_wait(&mutex);
            if(reencarnacion==0){
                //Continuar, el hijo vive por siempre
            }else{
                reencarnacion=reencarnacion-1;
                if(reencarnacion<=0){
                    murio=true;
                    sigueVivo=false;
                }
            }
            sem_post(&mutex);
            /*
            string mensaje = "Proceso suicida "+filename+" termino por causa: "
                             + std::to_string(status);*/
            int mensaje = WEXITSTATUS(status);
            write(salida, &mensaje, sizeof(&mensaje));
        }
    }
}

int main(int argc, char *argv[], char *env[]){
/*
    int in = 0;
    int out = 1;
    int sup;
    read(in, &sup, sizeof(sup));
    sup = sup+1;
    write(out,&sup,sizeof(sup));
	return 0;*/

    //Se inicia el mutex en 1, usado principalmente para sumar o restar las vidas
    sem_init(&mutex,0,1);
     

    /* Entradas de arg:
     * --filepath=<rutafichero>
     * --filename=<fichero>
     * --reencarnacion=<numero>
     * <numero del proceso de control
     */ 
     
     rutafichero = "/home/skp/b/pruebaos/bin";
     rutafichero=rutafichero+"/";
     filename = "ProcesoSuicida";
     reencarnacion = 3;
     idProcesoControl = 1;

     /* Se crea un hilo para encargarse de crear los procesos suicidas mientras
      * que el padre espera comandos de consola
      */
     pthread_t hiloControl;
     pthread_create(&hiloControl,NULL,HiloControl,NULL);
     while(true){
        //Read/write
        sem_wait(&mutex);
        if(murio){
            sem_post(&mutex);
            break;
        }else{
            sem_post(&mutex);
        }
        sleep(1);
     }
}





