#ifndef MEDICO_H_INCLUDED
#define MEDICO_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

typedef struct Medicos Med;

struct Medicos{
    char nome[40];
    char respostaBalcao[40];
    char nomeCliente[40];
    char sintomaCliente[40];
    char perguntaMedico[40];
    char respostaCliente[40];
    char especialidade[40];
    int id;
    int sinalDeVida;
    int cliente;
    int clientesAfrente;
    int estado; //0 -> nao registado ; 1 -> em espera ; 2 -> ocupado

};

#endif
