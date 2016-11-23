Documento del proyecto

Lenguaje:
La práctica se realizó en el lenguaje de programación C++.

Este proyecto en sistema de procesos que pueden volver a la vida (Procesos reencarnantes),
este tiene una estructura dividida básicamente en 3 partes:

Consola Control: Se encarga de mostrar mensajes al usuario sobre el estado de los procesos,
también puede recibir comandos en tiempo de ejecución.
Proceso Control: Es el encargado de manejar los procesos suicidas, de tal manera que este
notifica la causa de muerte de tal forma que se lleve un conteo de muertes del sistema.
También recibe comandos de consola control los cuales ejecuta sobre un suicida 
Nota: Hay un proceso controlador por cada proceso suicida
Proceso Suicida: Este proceso muere por una de las diversas causas que se exponen en el
fichero ProcesoSuicida.cpp

Ficheros externos: 
Contienen información sobre la cantidad de suicidas a ejecutar, ruta de los ficheros y 
cantidad de vidas

Información del proyecto:
El proyecto utiliza el compilador de C++11.

Algunas de las bibliotecas usadas son:

<fstream>
<string.h>
<iostream>
<pthread.h>
<stdlib.h>
<stdio.h>
<unistd.h>
<limits.h>
<sys/wait.h>
<sys/types.h>
<semaphore.h>
<fcntl.h>
<sys/stat.h>
<sys/ipc.h>
<sys/shm.h>
<errno.h>
<vector>
<iterator>
<sstream>
<algorithm>
<thread>