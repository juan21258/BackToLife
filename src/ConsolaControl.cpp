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
	#include <iterator>
	#include <sstream>
	#include <algorithm>
	#include <thread>
	
	const size_t n = 1024;
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
	//Vector COMPARTIDO donde se guarda el nombre del proceso suicida a ejecutar
	vector<string>nombreSuicida;
	//Vector COMPARTIDO donde se guarda el numero de vidas inicial de cada proceso suicida
	vector<int>numeroVidas;

	vector<vector<string>>comandoHilos;
	
	vector<string>mensajes;
	int counter = 0;
	bool end = false;
	string mensaje = "";
    int muertesControl = 0;
	//Semaforo consola
	sem_t mutexCon;

	//Semáforos
	sem_t * mutexValSeq;
	sem_t mutexOut;
	sem_t mutexErr;
	sem_t mutexPipe;

	sem_t semMensaje;

	//vector<int>ind;
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
		string id;
		int nDecesos;
		int muertes[];

		int getnDecesos(){
        	return nDecesos;
    	}

    	string getid(){
    		return id;
    	}
    
	    void setid(string n){
	    	id = n;   
	    }
	    
	    void setnDeceso(int n){
	    	nDecesos = n;   
	    }

	    void setnDecesos(int n){
	    	nDecesos = nDecesos + n;   
	    }
	};

	/*
	void *p = shmat(shmid,NULL,SHM_RDONLY);
	int *n = (int *)p;
	*p = n++;
	long *valSeq = (long *)p;
	p = valSeq++;
	InfoMuerte *InfoMuerte = (InfoMuerte *)p;
	
	sizeof(int)+sizeof(long)+n*sizeof(InfoMuerte);
	*/

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
		    //muertess[0].setnDecesos(3);
		    //muertess[0].setid("Hola");
		    //muertess[1].setnDecesos(4);
		    //muertess[1].setid("Holasd");
  		}
  
		long int getvalSeq(){return valSeq;} 
		int getN(){return n;}
		void setvalSeq(long int n){valSeq = n;}
		void setN(int val){n = val;}
		void liberarMemoria(){      
    		delete [] muertess;
    		muertess = NULL;
  		}
	};

	MemoriaCompartida cseg;

	void leerFichero(string ruta){
	  int i = 1;
	  int j = 3; //Path
	  int exec = 5;
	  int k = 6;
	  string infile;
	  vector<string>param;
	  ifstream myReadFile(ruta);
	  while (!myReadFile.eof()) {
	  	getline(myReadFile,infile);
	  	istringstream iss(infile);
	  	copy(istream_iterator<string>(iss),
	  		istream_iterator<string>(),
	  		back_inserter(param));
	  }
	  //ver archivo extraido 
	  /*
	  for(vector<string>::const_iterator i = param.begin(); i != param.end(); ++i){
	    cout << *i << "\n";
	  }
	  */
	  //Prueba de que toma los argumentos
	  while(i < param.size() && j < param.size() && k < param.size()){
	  	cout << param.at(i) << " " << param.at(j) << " "<< stoi(param.at(k)) << endl;

	  	idProcesos.push_back(param.at(i));
	  	pathSuicida.push_back(param.at(j)+"/");
	  	nombreSuicida.push_back(param.at(exec));
	  	numeroVidas.push_back(stoi(param.at(k)));
	  	
	  	i=i+8;
	  	j=j+8;
	  	k=k+8;
	  	exec=exec+8;
	  }
	  myReadFile.close();
	}

	void HiloConsola(int id, int p11, int p12, int p21, int p22){
		//short tid = (short)(intptr_t) id;
		int tid = id;
		string idP = idProcesos[tid];
		//Iniciar proceso de control
		int pipe1[2];
		int pipe2[2];
		pipe1[0]=p11;
		pipe1[1]=p12;
		pipe2[0]=p21;
		pipe2[1]=p22;
		int idHijo=fork();
		if(idHijo==0){
			//Hijo
			//Conecta salida del pipe1 a su entrada y la entrada del pipe2 a su salida
			dup2(pipe1[0],0);
			close(pipe1[1]);
			dup2(pipe2[1],1);
			close(pipe2[0]);
			//Inicia el proceso control
			//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<Cambiar direccion
			string tempPath= pathSuicida[tid];
			string tempNombre= nombreSuicida[tid];
			string tempNum= to_string(numeroVidas[tid]);
			string tempId = to_string(tid);
			execl("bin/procesoctrl","procesoctrl",tempPath.c_str(),
				  tempNombre.c_str(), tempNum.c_str(), tempId.c_str());
			sem_wait(&mutexCon);
			cerr << "Error abriendo:" << endl << flush;
			cerr << "Path "<<pathSuicida[tid]<<endl<<flush;
			cerr << "NombreSuicida "<<nombreSuicida[tid]<<endl<<flush;
			cerr << "Numero de vidas "<<numeroVidas[tid]<<endl<<flush;
			cerr << "Id "<<to_string(tid).c_str()<<endl<<flush;
			sem_post(&mutexCon);
		}else{
			//Padre
			//Conecta salida del pipe2 a su entrada y la entrada del pipe1 a su salida
			close(pipe1[0]);
			close(pipe2[1]);
			sem_post(&mutexPipe);
			//Matar zombies
			signal(SIGCHLD,SIG_IGN);
		}	
		bool murio = false;
		int comando;
		int vidas;
		int msg;
		int exitStatus;
		int estado;
		string strComando;
		int test =0;
		string strEstado;
        string strInfinito;
			
		read(pipe2[0],&test,sizeof(test));
		sem_post(&mutexPipe);
		while(!murio){
			
			if(comandoHilos[tid].empty()){
				comando = 0;
				write(pipe1[1],&comando,sizeof(comando));
				read(pipe2[0],&msg,sizeof(msg));
				//Si msg = 0 todavia no ha muerto ningun suicida
                //Si msg = 1 notificacion de que murio un suicida
                //Si msg = 2 notifica que el proceso murio  
               	if(msg==0){
					read(pipe2[0],&comando,sizeof(comando));
                }else if(msg==1){
					read(pipe2[0],&vidas,sizeof(vidas));
					read(pipe2[0],&exitStatus,sizeof(exitStatus));
					sem_wait(&mutexCon);
					cerr << "Proceso suicida " << idProcesos[tid] << " termino por causa: "
					<< exitStatus << " -- Proceso Control " << tid << " Vidas restantes: " << 
					vidas << endl;
					for(int i = 0;i < idProcesos.size();i++){
						if(tid == i){
							//ind.at(i)++;
							cseg.muertess[i].setnDecesos(1);
						}
					}
					cseg.valSeq++;
					sem_post(&mutexCon);
                }else if(msg==2){
					read(pipe2[0],&comando,sizeof(comando));
                	sem_wait(&mutexCon);
                	murio = true;
                	cerr << "Proceso: "<<idProcesos[tid]<< " termino." << endl << flush;
                    sem_post(&mutexCon);
                }
			}else{
				sem_wait(&mutexCon);
				strComando = comandoHilos[tid].back();
				comandoHilos[tid].pop_back();
				if(strComando=="listar"){
					sem_post(&mutexCon);
					comando = 1;
					write(pipe1[1],&comando,sizeof(comando));
					read(pipe2[0],&vidas,sizeof(vidas));
					read(pipe2[0],&estado,sizeof(estado));
					sem_wait(&mutexCon);
					if(estado==0){
						strEstado="corriendo";
					}else{
						strEstado="suspendido";
					}
					if(vidas == 0){
                        strInfinito = "infinitas";
                    }else{
                        strInfinito = to_string(vidas);
                    }
                    cerr << "Listar - Proceso: "<<idProcesos[tid]
                         << " - Vidas: "<< strInfinito 
                         << " - Estado: "<< strEstado 
                         << endl << flush;
                    sem_post(&mutexCon);
					read(pipe2[0], &comando, sizeof(comando));
				}else if(strComando=="sumar"){
					comando = 2;
					vidas = stoi(comandoHilos[tid].back());
					comandoHilos[tid].pop_back();
					sem_post(&mutexCon);
					write(pipe1[1],&comando,sizeof(comando));
					write(pipe1[1],&vidas,sizeof(vidas));
					read(pipe2[0], &comando, sizeof(comando));
				}else if(strComando=="restar"){
					comando = 3;
					vidas = stoi(comandoHilos[tid].back());
					comandoHilos[tid].pop_back();
					sem_post(&mutexCon);
					write(pipe1[1],&comando,sizeof(comando));
					write(pipe1[1],&vidas,sizeof(vidas));
					read(pipe2[0], &comando, sizeof(comando));
				}else if(strComando=="suspender"){
					sem_post(&mutexCon);
					comando = 4;
					write(pipe1[1],&comando,sizeof(comando));
					read(pipe2[0], &comando, sizeof(comando));
				}else if(strComando=="restablecer"){
					sem_post(&mutexCon);
					comando = 5;
					write(pipe1[1],&comando,sizeof(comando));
					read(pipe2[0], &comando, sizeof(comando));
				}else if(strComando=="indefinir"){
					sem_post(&mutexCon);
					comando = 6;
					write(pipe1[1],&comando,sizeof(comando));
					read(pipe2[0], &comando, sizeof(comando));
				}else if(strComando=="definir"){
					comando = 7;
					vidas = stoi(comandoHilos[tid].back());
					comandoHilos[tid].pop_back();
					sem_post(&mutexCon);
					write(pipe1[1],&comando,sizeof(comando));
					write(pipe1[1],&vidas,sizeof(vidas));
					read(pipe2[0], &comando, sizeof(comando));
				}else if(strComando=="terminar"){
					sem_post(&mutexCon);
					comando = 8;
					write(pipe1[1],&comando,sizeof(comando));
					read(pipe2[0], &comando, sizeof(comando));
				}
			}

		}
        int status;
        //waitpid(idHijo,&status,0);
		// close(pipe1[0]);
		// close(pipe2[1]);
        waitpid(idHijo,&status,0);
        sem_wait(&mutexCon);
        muertesControl++;
        if(muertesControl == comandoHilos.size()){
        	
        	cerr << "Total muertes: " << cseg.valSeq << endl << flush;
        	for(int i=0;i<idProcesos.size();i++){
        		cerr << "Muertes "<< cseg.muertess[i].getid() << ": "<< 
        		cseg.muertess[i].getnDecesos() << endl <<flush;
        	}
        	cseg.liberarMemoria();
        	cerr << "Todos los procesos han terminado. Presione enter para terminar"
        	<< endl << flush;

        }
        sem_post(&mutexCon);
		
	}

	int getTid(string cmd){
		for (int i = 0; i < idProcesos.size(); ++i)
		{
			if(cmd == idProcesos[i]){
				return i;
			}
		}
		return -1;
	}

	//int MemoriaCompartida::n = 254;


	int main(int argc, char *argv[], char *env[]){

		string psuicida;
		ifstream infile;
		//Consola control argumentos
		string ruta = "./bin/conctrl.cfg";
		string semid = "conctrlsem";
		string memoria = "666";
		string str_temp;
		string str_temp2;
		if(argc <= 4){
			for(int i=0;i<argc;i++){
				str_temp = argv[i];
				str_temp2 = str_temp.substr(0,23); 
				if(str_temp2 == "--ficheroconfiguracion="){
					ruta = str_temp.substr(23,str_temp.size()); 
				}
			}
			for(int i2=0;i2<argc;i2++){
				str_temp = argv[i2];
				str_temp2 = str_temp.substr(0,11); 
				if(str_temp2 == "--semaforo="){
					semid = str_temp.substr(11,str_temp.size()); 
				}
			}
			for(int i3=0;i3<argc;i3++){
				str_temp = argv[i3];
				str_temp2 = str_temp.substr(0,10); 
				if(str_temp2 == "--memoria="){
					memoria = str_temp.substr(10,str_temp.size()); 
				}
			}
		}
		else {
			cout << "Modo de uso:"<< endl;	
			cout << "conctrl [--ficheroconfiguracion=<rutaficherocfg>]" <<
			"[--semaforo=<id>][--memoriacompartida=<id>] " << endl;
		}


		key_t key = stoi(memoria);
		//TESTING

		leerFichero(ruta);
		int nProcesos = idProcesos.size();
		cseg.MemoriaCompartidas(nProcesos);
		for(int i=0;i<nProcesos;i++){
			cseg.muertess[i].setid(idProcesos.at(i));
			cseg.muertess[i].setnDeceso(0);
		}

		int sarray = sizeof(cseg.muertess)/sizeof(*cseg.muertess);
		int TAM = sizeof(int)+sizeof(long)+sarray*sizeof(InfoMuerte);

		int pipes1[nProcesos][2];
		int pipes2[nProcesos][2];
		for (int i = 0; i < nProcesos; ++i)
		{
			vector<string>cmd;
			comandoHilos.push_back(cmd);
			int pipe1[2];
			int pipe2[2];
			pipe(pipe1);
			pipe(pipe2);
			pipes1[i][0]=pipe1[0];
			pipes1[i][1]=pipe1[1];
			pipes2[i][0]=pipe2[0];
			pipes2[i][1]=pipe2[1];
		}
		cerr << "Total procesos: " << nProcesos << endl << flush;
	    sem_init(&mutexCon,0,1);
	    sem_init(&mutexPipe,0,0);
	    sem_init(&semMensaje,0,0);
	    thread t[nProcesos];
	    for (int i = 0; i < nProcesos; ++i)
	    {
	    	t[i] = thread(HiloConsola, i, pipes1[i][0], pipes1[i][1], 
	        							  pipes2[i][0], pipes2[i][1]); 
	    	
	    }
	    for (int i = 0; i < nProcesos; ++i)
	    {
	    	sem_wait(&mutexPipe);
	    	sem_wait(&mutexPipe);
	    }
	    



	    //Espera de comandos de consola
	    // inicio parser
	    string line;
	    vector<string>comando;
	    int i = 0;
    	try{
    		/*
    		while(true){
    			sem_wait(&mutexCon);
    			cerr << "hai "<< endl << flush;
    			sem_post(&mutexCon);
    		
    			string x;
    			getline(cin,x);//cin >> x;
    		
    			sem_wait(&mutexCon);
    			cerr << "got it ! "<< x << endl << flush;
    			sem_post(&mutexCon);
    		
    		}*/
    		//Parser consola
    		while(!cin.eof()){
    			
    			cout << "conctrl>";
    			getline(cin,line);
    			istringstream iss(line);
    			copy(istream_iterator<string>(iss),
    				istream_iterator<string>(),
    				back_inserter(comando));
    		if(comando.size()>1){		
    			if(comando.at(1)=="*"){
    				for (int i = 0; i < nProcesos; ++i){
    					sem_wait(&mutexCon);
    					if(comando.at(0)=="sumar"  || 
    						comando.at(0)=="restar" || 
    						comando.at(0)=="definir"){
    						comandoHilos[i].push_back(comando.at(2));
    					}
    					comandoHilos[i].push_back(comando.at(0));
    					sem_post(&mutexCon);
    				}
    			}else{
    				int id = getTid(comando.at(1));
    				sem_wait(&mutexCon);
					if(comando.at(0)=="sumar"   || 
    				   comando.at(0)=="restar" || 
    				   comando.at(0)=="definir"){
    					comandoHilos[id].push_back(comando.at(2));
    				}
    				comandoHilos[id].push_back(comando.at(0));
    				sem_post(&mutexCon);
    			}
   			}
    			comando.clear();
    			sem_wait(&mutexCon);
    			if (muertesControl == nProcesos){
    				break;
    			}
    			sem_post(&mutexCon);
    		}
    		sem_post(&mutexCon);
    	}catch(int i){}

    	for(int i=0; i < nProcesos; i++){
    		t[i].join();
    	}
	return 0;
}

