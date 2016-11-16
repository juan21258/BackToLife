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
#include <vector>
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
bool suspendido = false;
bool notificar = false;

vector<int>razonesDeMuerte;


sem_t mutex;

void* HiloControl(void*){
    bool sigueVivo = true;
    while(sigueVivo){
        sem_wait(&mutex);
        if(!suspendido && !notificar){
            sem_post(&mutex);
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
                sleep(5);
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
                notificar = true;
                razonesDeMuerte.push_back(WEXITSTATUS(status));
                sem_post(&mutex);
            }
        }else{
            sem_post(&mutex);
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
     
     rutafichero = argv[1];
     filename = argv[2];
     reencarnacion = stoi(argv[3]);
     idProcesoControl = stoi(argv[4]);

     /* Se crea un hilo para encargarse de crear los procesos suicidas mientras
      * que el padre espera comandos de consola
      */
     pthread_t hiloControl;
     pthread_create(&hiloControl,NULL,HiloControl,NULL);
     int comando;
     int numeroComando;
     bool end = false;
     int exitStatus;
     int vidas;
     int estado;
     int saludar=1;
     write(salida,&saludar,sizeof(saludar));
     while(!end){
        //Read/write
        /*read(entrada,&comando,sizeof(comando));
        if(comando==0){
            int msg;
            sem_wait(&mutex);
            if(notificar){
                sem_post(&mutex);
                msg=1;
                write(salida,&msg,sizeof(msg));
                sem_wait(&mutex);
                exitStatus = razonesDeMuerte.back();
                razonesDeMuerte.pop_back();
                vidas = reencarnacion;
                notificar = false;
                sem_post(&mutex);
                write(salida,&exitStatus,sizeof(exitStatus));
                write(salida,&vidas,sizeof(vidas));
            }else{
                if(murio && !notificar){
                    sem_post(&mutex);
                    msg=2;
                    end = true;
                    write(salida,&msg,sizeof(msg));  
                }else{
                    sem_post(&mutex);
                    msg=3;
                    write(salida,&msg,sizeof(msg));
                }
            }
        }*/
        // sem_wait(&mutex);
        // cerr << "COMANDO " << comando << endl << flush;
        // sem_post(&mutex);
        read(entrada,&comando,sizeof(comando));
        switch(comando){
            //Listar - retornar vidas
            case 1: sem_wait(&mutex);
                    if(suspendido){
                        estado = 1;
                    }else{
                        estado = 0;
                    }
                    write(salida,&reencarnacion,sizeof(reencarnacion));
                    write(salida,&estado,sizeof(estado));
                    sem_post(&mutex);
                    write(salida,&comando,sizeof(comando));
                    break;
            //Sumar - sumar vidas SOLO cuando no sea inmortal
            case 2: read(entrada,&numeroComando,sizeof(numeroComando));
                    sem_wait(&mutex);
                    if(reencarnacion!=0){
                        reencarnacion = reencarnacion + numeroComando;
                    }
                    write(salida,&comando,sizeof(comando));
                    sem_post(&mutex);
                    break;
            //Restar - restar vidas SOLO cuando no sea inmortal
            case 3: read(entrada,&numeroComando,sizeof(numeroComando));
                    sem_wait(&mutex);
                    if(reencarnacion!=0)reencarnacion = reencarnacion - 
                                                        numeroComando;
                    write(salida,&comando,sizeof(comando));
                    sem_post(&mutex);
                    break;
            //Suspender - No empezar mas procesos suicidas
            case 4: sem_wait(&mutex);
                    suspendido = true;
                    write(salida,&comando,sizeof(comando));
                    sem_post(&mutex);
                    break;
            //Reestablecer
            case 5: sem_wait(&mutex);
                    suspendido = false;
                    write(salida,&comando,sizeof(comando));
                    sem_post(&mutex);
                    break;
            //Indefinir - cambia el numero de vidas a 0 (inmortal)
            case 6: sem_wait(&mutex);
                    reencarnacion = 0;
                    write(salida,&comando,sizeof(comando));
                    sem_post(&mutex);
                    break;
            //Definir - cambia el numero de vidas
            case 7: sem_wait(&mutex);
                    read(entrada,&numeroComando,sizeof(numeroComando));
                    reencarnacion = numeroComando;
                    write(salida,&comando,sizeof(comando));
                    sem_post(&mutex);
                    break;
            //Terminar
            case 8: sem_wait(&mutex);
                    murio = true;
                    write(salida,&comando,sizeof(comando));
                    sem_post(&mutex);
                    break;
            //Mandar vidas y/o un mensaje de que un proceso suicida murio
            default: sem_wait(&mutex);
                     //integer para que el hilo de consola interprete el mensaje
                     //Si msg = 0 todavia no ha muerto ningun suicida
                     //Si msg = 1 hay que mandar un mensaje adicional por el pipe
                     //si msg = 3 notifica que el proceso murio 
                     int msg;
                     //cerr << "ummmm " << reencarnacion << endl << flush;

                     if(notificar){
                        //Hay que mandar mensaje de muerte
                        msg = 1;
                        exitStatus = razonesDeMuerte.back();
                        razonesDeMuerte.pop_back();
                        vidas = reencarnacion;
                        notificar = false;
                        sem_post(&mutex);
                        write(salida,&msg,sizeof(msg));
                        write(salida,&vidas,sizeof(vidas));
                        write(salida,&exitStatus,sizeof(exitStatus));
                     }else{
                        //No hay que mandar mensaje de muerte
                        if(murio && !notificar){
                            end = true;
                            msg = 2;
                            sem_post(&mutex);
                            write(salida,&msg,sizeof(msg));
                            write(salida,&comando,sizeof(comando));
                        }else{
                            msg = 0;
                            sem_post(&mutex);
                            write(salida,&msg,sizeof(msg));
                            write(salida,&comando,sizeof(comando));
                        }
                     }
                     break;
            
        }
        sem_wait(&mutex);
        if(end){
            sem_post(&mutex);
            break;
        }else{
            sem_post(&mutex);
        }
    }

    return 0;
}





