#ifndef BALCAO_H_INCLUDED
#define BALCAO_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>

#include "medico.h"
#include "cliente.h"

#define FIFO_SRD "canal"
#define FIFO_SRDMED "canalmed"
#define FIFO_CLI "cli%d"
#define FIFO_MED "med%d"

int getMAXCLIENTES();
int getMAXMEDICOS();

typedef struct balcao bal;

struct balcao{
    char nomeCliente[40];
    char sintoma[40];
    char nomeMedico[40];
    char especialidade[40];
    bool fila;  //0 em fila/consulta terminada, 1 em consulta
    int prioridade;
    int nrDePessoasNaFila;
    int numeroDeMedicos;
    int id_consulta;

    int MAXMEDICOS;
    int MAXCLIENTES;

};

typedef struct{
    int continua;
    int segundos;
    //pthread_mutex_t *trinco;
    Cli *oftalmologia;
    Cli *neurologia;
    Cli *estomatologia;
    Cli *ortopedia;
    Cli *geral;
}TDATA;



#endif

