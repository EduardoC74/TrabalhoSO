#ifndef CLIENTE_H_INCLUDED
#define CLIENTE_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/stat.h>

typedef struct Clientes Cli;

struct Clientes{
    char nome[40];
    char respostaCliente[40];
    char respostaClassificador[40];
    char respostaBalcao[40];
    char perguntaMedico[40];
    char nomeMedico[40];
    char sintoma[40];
    char especialidade[40];
    int prioridade;
    int nrUtentes;
    int nrEspecialistas;
    int id;
    int medico;
    int estado; //0 -> em espera ; 1 -> a ser atendido

};

#endif
