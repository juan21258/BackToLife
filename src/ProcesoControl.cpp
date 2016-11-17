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
        string id;
        int nDecesos;
        int muertes[];
        //Obtener decesos 
        int getnDecesos(){
            return nDecesos;
        }
        //Obtener id del proceso
        string getid(){
            return id;
        }
        //Asignar id proceso
        void setid(string n){
            id = n;   
        }
        //aumentar decesos
        void setnDeceso(int n){
            nDecesos = n;   
        }

        void setnDecesos(int n){
            nDecesos = nDecesos + n;   
        }
    };
    //Estructura para la memoria compartida
    struct MemoriaCompartida {
        int n; // Numero de procesos controladores
        long int valSeq = 0;
        struct InfoMuerte * muertess = NULL; 
        
        void MemoriaCompartidas(int val){
            n = val;
            muertess = new struct InfoMuerte[val];
            if (muertess == NULL){
                cout << "No se pudo obtener memoria compartida" << endl;
                exit (0);
            }   
        }
  
        long int getvalSeq(){return valSeq;} 
        int getN(){return n;} // Se obtienen el valor de n
        void setvalSeq(long int n){valSeq = n;}
        void setN(int val){n = val;}
        void liberarMemoria(){      
            delete [] muertess; //Elimina memoria no usada
            muertess = NULL;
        }
    };
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
    //Causas de muerte que se inforrman al usuario
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
                sleep(5); //5 segundos entre muertes, esto para controlar los suicidios
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
                razonesDeMuerte.push_back(WEXITSTATUS(status)); // Causa de muerte
                sem_post(&mutex);
            }
        }else{
            sem_post(&mutex); //liberar mutex
        }
    }
}

int main(int argc, char *argv[], char *env[]){

    //Se inicia el mutex en 1, usado principalmente para sumar o restar las vidas
    sem_init(&mutex,0,1);
     
    //Se obtienen los parámetros que se pasaron desde consola
     
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
     bool end = false; //Indica cuando no se reciben comandos
     int exitStatus; //Estado de muerte
     int vidas;
     int estado;
     int saludar=1;
     while(!end){
        read(entrada,&comando,sizeof(comando));
        switch(comando){ 
            /*  Se recibe el entero que se pasa, para identificar
                que comando se paso 
            */
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
            sem_post(&mutex); //Si se llega a esto ya no se reciben comandos
            break;
        }else{
            sem_post(&mutex);
        }
    }

    return 0;
}





