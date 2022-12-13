#include "balcao.h"

void *filas(void *dados)
{
    int i, contOftalmologia = 0, contNeurologia = 0, contEstomatologia = 0, contOrtopedia = 0, contGeral = 0;
    TDATA *td = (TDATA *) dados;
    do {
        sleep(td->segundos);

        for (i = 0; i < 5; i++){

            if(td->oftalmologia[i].id != 0){
                printf("%d", td->oftalmologia[i].id);
                contOftalmologia++;
            }

            if(td->neurologia[i].id != 0){
                printf("oi\n");
                contNeurologia++;
            }
            if(td->estomatologia[i].id != 0){
                contEstomatologia++;
            }

            if(td->ortopedia[i].id != 0){
                contOrtopedia++;
            }

            if(td->geral[i].id != 0){
                contGeral++;
            }

        }

        printf("\n---------> FILAS DE ESPERA <--------");
        printf("\nClientes na fila de oftalmologia:  %d", contOftalmologia);
        printf("\nClientes na fila de neurologia:    %d", contNeurologia);
        printf("\nClientes na fila de estomatologia: %d", contEstomatologia);
        printf("\nClientes na fila de ortopedia:     %d", contOrtopedia);
        printf("\nClientes na fila geral:            %d\n", contGeral);

        contOftalmologia = 0;
        contNeurologia = 0;
        contEstomatologia = 0;
        contOrtopedia = 0;
        contGeral = 0;

    } while (td->continua);
    pthread_exit(NULL);
}

void trataControlC(int num, siginfo_t *info, void *uc)
{
    Cli cliente;
    int fd, n;

    //ABRIR FIFO PARA ESCRITA
    fd = open(FIFO_SRD, O_WRONLY);
    if(fd == -1){
        printf("[ERRO] Não foi possível abrir o FIFO!\n");
        exit(2);
    }

    strcpy(cliente.respostaBalcao, "encerrar");
    cliente.estado = 2;

    n = write(fd, &cliente, sizeof(Cli));

    close(fd);
}

int main(int argc, char *argv[]){ //char **argv

    int cont = 0, cont2 = 0, contOftalmologia = 0, contNeurologia =0, contEstomatologia = 0, contOrtopedia = 0, contGeral = 0, pidcli, erro_write, i, j, erro_read, res, resSelect, envia[2], recebe[2], fd, fdmed, ff, fdbalmed, n, nrclientes = 0, posCliIgual, posMedIgual, auxClientes, auxMedicos, contaClientes = 0, contaMedicos = 0;
    char recebe_msg[40], envia_msg[16], fifocli[40], fifomed[40], comandos[40], token[40];
    char comandoGuarda[40];
    int guardaTempoFreq;
    char *guardacomando[2];
    char *comandoStrtok = NULL;
    char copia[40];
    int contStrtok = 0;
    fd_set fds;
    struct timeval tempo;
    Med medico;
    Cli cliente, cliAux;
    Cli oftalmologia[5] = {0, 0, 0, 0, 0};
    Cli neurologia[5] = {0, 0, 0, 0, 0};
    Cli estomatologia[5] = {0, 0, 0, 0, 0};
    Cli ortopedia[5] = {0, 0, 0, 0, 0};
    Cli geral[5] = {0, 0, 0, 0, 0};
    struct balcao bal;

    //Ctrl+C
    struct sigaction actControlC;
    actControlC.sa_sigaction = trataControlC;
    actControlC.sa_flags = SA_SIGINFO;
    sigaction(SIGINT,&actControlC,NULL);

    pthread_t tid;
    TDATA tdata;
    pthread_mutex_t trinco;


    if ((bal.MAXMEDICOS = getMAXMEDICOS()) == -1)
        return (EXIT_FAILURE);

    printf("| MAX MEDICOS -> %d  |\n", bal.MAXMEDICOS);

    if ((bal.MAXCLIENTES = getMAXCLIENTES()) == -1)
        return (EXIT_FAILURE);

    printf("| MAX CLIENTES -> %d |\n", bal.MAXCLIENTES);

    Cli arrayClientes[bal.MAXCLIENTES];
    Med arrayMedicos[bal.MAXMEDICOS];

	//VERIFICAR SE O FIFO EXISTE
    if(access(FIFO_SRD, F_OK) == 0){
		printf("[ERRO] O FIFO já existe!\n");
		exit(1);
	}

    //CRIAR O FIFO
    if(mkfifo(FIFO_SRD, 0600) == -1){
		printf("[ERRO] Não foi possível criar o FIFO!\n");
		exit(1);
	}

    //ABRIR O FIFO PARA LEITURA
    fd = open(FIFO_SRD, O_RDWR);
	if(fd == -1){
        printf("ERRO ao abrir o FIFO para leitura!!!\n");
		exit(3);
	}
	
	
	//VERIFICAR SE O FIFO MEDICO EXISTE
    if (access(FIFO_SRDMED, F_OK) == 0) {
        printf("O FIFO ja existe!!!\n");
        exit(1);
    }

    //CRIAR O FIFO DO MEDICO
    if (mkfifo(FIFO_SRDMED, 0600) == -1) {
        printf("ERRO ao criar o FIFO!!!\n");
        exit(1);
    }

    //ABRIR O FIFO PARA LEITURA
    fdmed = open(FIFO_SRDMED, O_RDWR);
    if (fdmed == -1) {
        printf("ERRO ao abrir o FIFO para leitura!!!\n");
        exit(1);
    }


    printf("\n-----BALCAO-----\n\n");
    fflush(stdout);

    pipe(envia);
    pipe(recebe);

    res = fork();

    if (res < 0){
        printf("Erro ao criar o filho\n");
        exit(1);
    } 
    if (res == 0){ //filho
        //enganar o filho
        close(STDIN_FILENO);
        dup(envia[0]);
        close(envia[0]);
        close(envia[1]);

        close(STDOUT_FILENO);
        dup(recebe[1]);
        close(recebe[0]);
        close(recebe[1]);

        execl("classificador", "classificador", NULL); //executa o classificador
        fprintf(stderr, "ERRO ao executar o classificador!!!\n");
        exit(1);

    } else {
        close(envia[0]);
        close(recebe[1]);

        tdata.continua = 1;
        tdata.segundos = 30;
        //tdata.trinco = &trinco;

        tdata.oftalmologia = oftalmologia;
        tdata.neurologia = neurologia;
        tdata.estomatologia = estomatologia;
        tdata.ortopedia = ortopedia;
        tdata.geral = geral;

        pthread_create(&tid, NULL, filas, (void *) &tdata);

	    do{

            printf("\nComando?\n");
            //SELECT
            FD_ZERO(&fds);
            FD_SET(0, &fds);
            FD_SET(fd, &fds);
            FD_SET(fdmed, &fds);
            tempo.tv_sec = 8; //TIMOUT
            tempo.tv_usec = 0;
            resSelect = select(fd > fdmed ? fd + 1 : fdmed + 1, &fds, NULL, NULL, NULL);
		
		if(resSelect == 0){


		}
		else if (resSelect > 0 && FD_ISSET(0, &fds)) { // dados do teclado
            scanf(" %[^\n]", comandos);


            strcpy(copia, comandos);

            comandoStrtok = strtok(copia, " ");

            while (comandoStrtok != NULL) {
                guardacomando[contStrtok++] = comandoStrtok;
                comandoStrtok = strtok(NULL, " ");

            }
            contStrtok = 0;

            strcpy(comandoGuarda, guardacomando[0]);

            if (strcmp(comandos, "utentes") == 0) {

                printf("\n-----> FILAS DE ESPERA <-----\n");

                printf("\n*****OFTALMOLOGIA*****\n");
                if (contOftalmologia == 0)
                    printf("Nao existem utentes nesta fila de espera\n");
                for (i = 0; i < contOftalmologia; i++) {
                    printf("Nome: %s # PID: %d # Prioridade: %d\n", oftalmologia[i].nome, oftalmologia[i].id,
                           oftalmologia[i].prioridade);
                }

                printf("\n******NEUROLOGIA******\n");
                if (contNeurologia == 0)
                    printf("Nao existem utentes nesta fila de espera\n");
                for (i = 0; i < contNeurologia; i++) {
                    printf("Nome: %s # PID: %d # Prioridade: %d\n", neurologia[i].nome, neurologia[i].id,
                           neurologia[i].prioridade);
                }

                printf("\n*****ESTOMATOLOGIA****\n");
                if (contEstomatologia == 0)
                    printf("Nao existem utentes nesta fila de espera\n");
                for (i = 0; i < contEstomatologia; i++) {
                    printf("Nome: %s # PID: %d # Prioridade: %d\n", estomatologia[i].nome, estomatologia[i].id,
                           estomatologia[i].prioridade);
                }

                printf("\n*******ORTOPEDIA******\n");
                if (contOrtopedia == 0)
                    printf("Nao existem utentes nesta fila de espera\n");
                for (i = 0; i < contOrtopedia; i++) {
                    printf("Nome: %s # PID: %d # Prioridade: %d\n", ortopedia[i].nome, ortopedia[i].id,
                           ortopedia[i].prioridade);
                }

                printf("\n*********GERAL********\n");
                if (contGeral == 0)
                    printf("Nao existem utentes nesta fila de espera\n");
                for (i = 0; i < contGeral; i++) {
                    printf("Nome: %s # PID: %d # Prioridade: %d\n", geral[i].nome, geral[i].id, geral[i].prioridade);
                }

                printf("\n-> UTENTES A SER ATENDIDOS <-\n");

                bool existir = false;
                for (i = 0; i < contaClientes; i++) {
                    if (arrayClientes[i].estado == 2) {
                        existir = true;
                        for (j = 0; j < contaMedicos; j++) {
                            if (arrayMedicos[i].id == arrayClientes[i].medico) {
                                printf("O utente %s esta a ser atentido pelo Dr.%s, destinado a especialidade %s\n",
                                       arrayClientes[i].nome, arrayMedicos[j].nome, arrayClientes[i].especialidade);
                            }
                        }
                    }
                }
                if (!existir)
                    printf("Nao existem utentes a serem atendidos\n");

            } else if (strcmp(comandos, "especialistas") == 0) {

                printf("\n\n--> Especialistas em espera <--\n");

                bool existirMed = false;
                for (i = 0; i < contaMedicos; i++) {
                    if (arrayMedicos[i].estado != 2) {
                        existirMed = true;
                        printf("Nome: %s # PID: %d # Especialidade: %s\n", arrayMedicos[i].nome, arrayMedicos[i].id,
                               arrayMedicos[i].especialidade);
                    }
                }
                if (!existirMed)
                    printf("Nao existem medicos em espera\n");

                printf("\n-> Especialistas em consulta <-\n");

                existirMed = false;
                for (i = 0; i < contaMedicos; i++) {
                    if (arrayMedicos[i].estado == 2) {
                        existirMed = true;
                        for (j = 0; j < contaClientes; j++) {
                            if (arrayClientes[i].id == arrayMedicos[i].cliente) {
                                printf("O Dr.%s, destinado a especialidade %s esta a atender o utente %s\n",
                                       arrayMedicos[i].nome, arrayMedicos[j].especialidade, arrayClientes[i].nome);
                            }
                        }
                    }
                }
                if (!existirMed)
                    printf("Nao existem medicos em consulta\n");

            } else if (strcmp(comandoGuarda, "freq") == 0) {


                bool digito = true;
                char outro[40];

                if(guardacomando[1] != NULL) {

                    strcpy(outro, guardacomando[1]);

                        for (i = 0; i < strlen(outro); i++) {
                            if (isdigit(outro[i]) == 0)
                                digito = false;
                        }

                        if (digito == false) {
                            printf("Comando mal inserido!\n");
                        } else {
                            int pedeTempo = atoi(guardacomando[1]);
                            tdata.segundos = pedeTempo;
                            printf("Tempo de amostragem das listas mudado para %d", pedeTempo);
                        }

                } else
                    printf("Comando mal inserido!\n");
            }else if(strcmp(comandoGuarda, "delut") == 0) {
                bool igual = false, estado = false, digito = true;
                char outro[40];

                if (guardacomando[1] != NULL) {

                    strcpy(outro, guardacomando[1]);

                    for (i = 0; i < strlen(outro); i++) {
                        if (isdigit(outro[i]) == 0)
                            digito = false;
                    }

                    if (digito == false) {
                        printf("Comando mal inserido!\n");
                    } else {

                        int id = atoi(guardacomando[1]);

                        for (i = 0; i < contaClientes; i++) {

                            if (arrayClientes[i].id == id) {
                                igual = true;
                                if (arrayClientes[i].estado != 2) {
                                    estado = true;

                                    sprintf(fifocli, FIFO_CLI, arrayClientes[i].id);

                                    ff = open(fifocli, O_WRONLY);
                                    if (ff == -1) {
                                        printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                        close(fd);

                                        unlink(FIFO_SRD);
                                    }

                                    strcpy(medico.respostaBalcao, "delut");

                                    n = write(ff, &medico, sizeof(Med));
                                    if (n == -1) {
                                        printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                        close(fd);

                                        unlink(FIFO_SRD);
                                    }

                                    close(ff);
                                }
                            }
                        }

                        if (igual != true)
                            printf("Cliente inexistente!\n");
                        else if (estado != true)
                            printf("Este cliente encontra-se numa consulta!\n");
                    }
                } else
                    printf("Comando mal inserido!\n");

                } else if (strcmp(comandoGuarda, "delesp") == 0) {
                    bool igual = false, estado = false, digito = true;
                    char outro[40];

                    if (guardacomando[1] != NULL) {

                        strcpy(outro, guardacomando[1]);

                        for (i = 0; i < strlen(outro); i++) {
                            if (isdigit(outro[i]) == 0)
                                digito = false;
                        }

                        if (digito == false) {
                            printf("Comando mal inserido!\n");
                        } else {

                            int id = atoi(guardacomando[1]);

                            for (i = 0; i < contaMedicos; i++) {
                                if (arrayMedicos[i].id == id) {
                                    igual = true;
                                    if (arrayClientes[i].estado != 2) {
                                        estado = true;

                                        printf("O Dr.%s com a especialidade %s foi embora.\n", arrayMedicos[i].nome,
                                               arrayMedicos[i].especialidade);


                                        sprintf(fifomed, FIFO_MED, arrayMedicos[i].id);

                                        fdbalmed = open(fifomed, O_WRONLY);
                                        if (fdbalmed == -1) {
                                            printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                            close(fd);
                                            unlink(FIFO_SRDMED);
                                        }

                                        strcpy(cliente.respostaBalcao, "encerra");
                                        n = write(fdbalmed, &cliente, sizeof(Cli));
                                        if (n == -1) {
                                            printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                            close(fd);

                                            unlink(FIFO_SRD);
                                        }

                                        close(fdbalmed);

                                        for (i = 0; i < contaMedicos; i++) {
                                            if (arrayMedicos[i].id == medico.id) {
                                                arrayMedicos[i] = arrayMedicos[contaMedicos - 1];
                                                contaMedicos--;
                                            }
                                        }
                                    }
                                }
                            }

                            if (igual != true)
                                printf("Medico inexistente!\n");
                            else if (estado != true)
                                printf("Este medico encontra-se numa consulta!\n");
                        }
                    } else
                        printf("Comando mal inserido!\n");
                    } else if (strcmp(comandos, "lista") == 0) {

                        for (i = 0; i < contaClientes; i++)
                            printf("Nome: %s # Sintoma: %s # Estado: %d\n", arrayClientes[i].nome,
                                   arrayClientes[i].sintoma, arrayClientes[i].estado);
                        for (i = 0; i < contaMedicos; i++)
                            printf("Nome: %s # Especialidade: %s # Estado: %d\n", arrayMedicos[i].nome,
                                   arrayMedicos[i].especialidade, arrayMedicos[i].estado);

                    } else if (strcmp(comandos, "encerra") == 0) {
                        for (int i = 0; i < contaMedicos; i++) {

                            sprintf(fifomed, FIFO_MED, arrayMedicos[i].id);

                            fdbalmed = open(fifomed, O_WRONLY);
                            if (fdbalmed == -1) {
                                printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                close(fd);
                                unlink(FIFO_SRDMED);
                            }

                            strcpy(cliente.respostaBalcao, comandos);
                            n = write(fdbalmed, &cliente, sizeof(Cli));
                            if (n == -1) {
                                printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                close(fd);

                                unlink(FIFO_SRD);
                            }

                            close(fdbalmed);
                        }

                        for (int i = 0; i < contaClientes; i++) {

                            sprintf(fifocli, FIFO_CLI, arrayClientes[i].id);

                            ff = open(fifocli, O_WRONLY);
                            if (ff == -1) {
                                printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                close(fd);

                                unlink(FIFO_SRD);
                            }

                            strcpy(medico.respostaBalcao, comandos);

                            n = write(ff, &medico, sizeof(Med));
                            if (n == -1) {
                                printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                close(fd);

                                unlink(FIFO_SRD);
                            }

                            close(ff);
                        }

                        printf("\nO BALCAO terminou!!!\n");

                        close(fd);
                        unlink(FIFO_SRD);
                        close(fdmed);
                        unlink(FIFO_SRDMED);
                        break;
                    } else
                        printf("Comando inexistente!\n");
                }
		else if (resSelect > 0 && FD_ISSET(fd, &fds)){

	       	n = read(fd, &cliente, sizeof(Cli));

			if(n == sizeof(Cli)) {

                if(strcmp(cliente.respostaBalcao, "encerrar") == 0 ) {

                    for(int i = 0; i < contaMedicos; i++){

                        sprintf(fifomed, FIFO_MED, arrayMedicos[i].id);

                        fdbalmed = open(fifomed, O_WRONLY);
                        if(fdbalmed == -1){
                            printf("[ERRO] Não foi possível abrir o FIFO!\n");
                            close(fd);
                            unlink(FIFO_SRDMED);
                        }

                        strcpy(cliente.respostaBalcao, "encerra");
                        n = write(fdbalmed, &cliente, sizeof(Cli));
                        if (n == -1) {
                            printf("[ERRO] Não foi possível abrir o FIFO!\n");
                            close(fd);

                            unlink(FIFO_SRD);
                        }

                        close(fdbalmed);
                    }

                    for(int i = 0; i < contaClientes; i++){

                        sprintf(fifocli, FIFO_CLI, arrayClientes[i].id);

                        ff = open(fifocli, O_WRONLY);
                        if(ff == -1){
                            printf("[ERRO] Não foi possível abrir o FIFO!\n");
                            close(fd);

                            unlink(FIFO_SRD);
                        }

                        strcpy(medico.respostaBalcao, "encerra");

                        n = write(ff, &medico, sizeof(Med));
                        if (n == -1) {
                            printf("[ERRO] Não foi possível abrir o FIFO!\n");
                            close(fd);

                            unlink(FIFO_SRD);
                        }

                        close(ff);
                    }

                    printf("\nO BALCAO terminou!!!\n");

                    close(fd);
                    unlink(FIFO_SRD);
                    close(fdmed);
                    unlink(FIFO_SRDMED);
                    break;

                } else if (strcmp(cliente.sintoma, "sair") == 0 || strcmp(cliente.respostaCliente, "adeus") == 0 || strcmp(cliente.respostaCliente, "sair") == 0) {

                    if (strcmp(cliente.sintoma, "sair") == 0) {
                        for(i = 0; i < contaClientes; i++) {
                            if (arrayClientes[i].id == cliente.id) {
                                printf("\nO paciente %s desistiu da consulta.\n", cliente.nome);
                            }
                        }
                    } else {
                        for(i = 0; i < contaMedicos; i++) {
                            if(arrayMedicos[i].id == cliente.medico) {
                                printf("\nA consulta do paciente %s e do Dr.%s acabou.\n", cliente.nome, arrayMedicos[i].nome);
                            }
                        }
                    }


                    sprintf(fifocli, FIFO_CLI, cliente.id);

                    ff = open(fifocli, O_WRONLY);
                    if (ff == -1) {
                        printf("[ERRO] Não foi possível abrir o FIFO!\n");
                        close(fd);

                        unlink(FIFO_SRD);
                    }

                    if(cliente.estado == 0) {
                        strcpy(cliente.respostaBalcao, "encerra");
                        n = write(ff, &cliente, sizeof(Cli));
                        if (n == -1) {
                            printf("[ERRO] Não foi possível abrir o FIFO!\n");
                            close(fd);

                            unlink(FIFO_SRD);
                        }
                    } else {
                        strcpy(medico.respostaBalcao, "encerra");
                        n = write(ff, &medico, sizeof(Med));
                        if (n == -1) {
                            printf("[ERRO] Não foi possível abrir o FIFO!\n");
                            close(fd);

                            unlink(FIFO_SRD);
                        }
                    }

                    close(ff);

                    for (i = 0; i < contaClientes; i++) {
                        if (arrayClientes[i].id == cliente.id) {
                            arrayClientes[i] = arrayClientes[contaClientes-1];
                            contaClientes--;
                        }
                    }

                    if (strcmp(cliente.especialidade, "oftalmologia") == 0) {
                        for (j = 0; j < contOftalmologia; j++) {
                            if (oftalmologia[j].id == cliente.id) {

                                for(int x = j + 1; x < contOftalmologia; x++) {


                                    sprintf(fifocli, FIFO_CLI, oftalmologia[x].id);

                                    ff = open(fifocli, O_WRONLY);
                                    if (ff == -1) {
                                        printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                        exit(1);
                                    }

                                    medico.clientesAfrente = -1;

                                    strcpy(medico.respostaBalcao, "");
                                    n = write(ff, &medico, sizeof(Med));
                                    if (n == -1) {
                                        printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                        close(fd);

                                        unlink(FIFO_SRD);
                                    }
                                    close(ff);

                                    medico.clientesAfrente = 0;
                                }

                                oftalmologia[j] = oftalmologia[contOftalmologia - 1];
                                oftalmologia[contOftalmologia -1].id = 0;
                                contOftalmologia--;
                            }
                        }

                        for(int cont = 1; cont < contOftalmologia; cont++){
                            for(i = 0; i < contOftalmologia-1; i++) {
                                if (oftalmologia[i].prioridade > oftalmologia[i+1].prioridade) {
                                    Cli aux = oftalmologia[i];
                                    oftalmologia[i] = oftalmologia[i+1];
                                    oftalmologia[i+1] = aux;
                                }
                            }
                        }

                        tdata.oftalmologia = oftalmologia;

                    } else if (strcmp(cliente.especialidade, "neurologia") == 0) {
                        for (j = 0; j < contNeurologia; j++) {
                            if (neurologia[j].id == cliente.id) {

                                for(int x = j + 1; x < contNeurologia; x++) {


                                    sprintf(fifocli, FIFO_CLI, neurologia[x].id);

                                    ff = open(fifocli, O_WRONLY);
                                    if (ff == -1) {
                                        printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                        exit(1);
                                    }

                                    medico.clientesAfrente = -1;

                                    strcpy(medico.respostaBalcao, "");
                                    n = write(ff, &medico, sizeof(Med));
                                    if (n == -1) {
                                        printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                        close(fd);

                                        unlink(FIFO_SRD);
                                    }
                                    close(ff);

                                    medico.clientesAfrente = 0;
                                }

                                neurologia[j] = neurologia[contNeurologia - 1];
                                neurologia[contNeurologia -1].id = 0;
                                contNeurologia--;
                            }
                        }

                        for(int cont = 1; cont < contNeurologia; cont++){
                            for(i = 0; i < contNeurologia-1; i++) {
                                if (neurologia[i].prioridade > neurologia[i+1].prioridade) {
                                    Cli aux = neurologia[i];
                                    neurologia[i] = neurologia[i+1];
                                    neurologia[i+1] = aux;
                                }
                            }
                        }

                        tdata.neurologia = neurologia;

                    } else if (strcmp(cliente.especialidade, "estomatologia") == 0) {
                        for (j = 0; j < contEstomatologia; j++) {
                            if (estomatologia[j].id == cliente.id) {

                                for(int x = j + 1; x < contEstomatologia; x++) {


                                    sprintf(fifocli, FIFO_CLI, estomatologia[x].id);

                                    ff = open(fifocli, O_WRONLY);
                                    if (ff == -1) {
                                        printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                        exit(1);
                                    }

                                    medico.clientesAfrente = -1;

                                    strcpy(medico.respostaBalcao, "");
                                    n = write(ff, &medico, sizeof(Med));
                                    if (n == -1) {
                                        printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                        close(fd);

                                        unlink(FIFO_SRD);
                                    }
                                    close(ff);

                                    medico.clientesAfrente = 0;
                                }

                                estomatologia[j] = estomatologia[contEstomatologia - 1];
                                estomatologia[contEstomatologia -1].id = 0;
                                contEstomatologia--;
                            }
                        }

                        for(int cont = 1; cont < contEstomatologia; cont++){
                            for(i = 0; i < contEstomatologia-1; i++) {
                                if (estomatologia[i].prioridade > estomatologia[i+1].prioridade) {
                                    Cli aux = estomatologia[i];
                                    estomatologia[i] = estomatologia[i+1];
                                    estomatologia[i+1] = aux;
                                }
                            }
                        }

                        tdata.estomatologia = estomatologia;

                    } else if (strcmp(cliente.especialidade, "ortopedia") == 0) {
                        for (j = 0; j < contOrtopedia; j++) {
                            if (ortopedia[j].id == cliente.id) {


                                for(int x = j + 1; x < contOrtopedia; x++) {


                                    sprintf(fifocli, FIFO_CLI, ortopedia[x].id);

                                    ff = open(fifocli, O_WRONLY);
                                    if (ff == -1) {
                                        printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                        exit(1);
                                    }

                                    medico.clientesAfrente = -1;

                                    strcpy(medico.respostaBalcao, "");
                                    n = write(ff, &medico, sizeof(Med));
                                    if (n == -1) {
                                        printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                        close(fd);

                                        unlink(FIFO_SRD);
                                    }
                                    close(ff);

                                    medico.clientesAfrente = 0;
                                }


                                for(int k = j; k < contOrtopedia-1; k++) {
                                    ortopedia[k] = ortopedia[k+1];
                                }

                                ortopedia[contOrtopedia - 1].id = 0;
                                contOrtopedia--;
                            }
                        }

                        /*for(int cont = 1; cont < contOrtopedia; cont++){
                            for(i = 0; i < contOrtopedia-1; i++) {
                                if (ortopedia[i].prioridade > ortopedia[i+1].prioridade) {
                                    Cli aux = ortopedia[i];
                                    ortopedia[i] = ortopedia[i+1];
                                    ortopedia[i+1] = aux;
                                }
                            }
                        }*/

                        tdata.ortopedia = ortopedia;

                    } else if (strcmp(cliente.especialidade, "geral") == 0) {
                        for (j = 0; j < contGeral; j++) {
                            if (geral[j].id == cliente.id) {

                                for(int x = j + 1; x < contGeral; x++) {


                                    sprintf(fifocli, FIFO_CLI, geral[x].id);

                                    ff = open(fifocli, O_WRONLY);
                                    if (ff == -1) {
                                        printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                        exit(1);
                                    }

                                    medico.clientesAfrente = -1;

                                    strcpy(medico.respostaBalcao, "");
                                    n = write(ff, &medico, sizeof(Med));
                                    if (n == -1) {
                                        printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                        close(fd);

                                        unlink(FIFO_SRD);
                                    }
                                    close(ff);

                                    medico.clientesAfrente = 0;
                                }

                                geral[j] = geral[contGeral - 1];
                                geral[contGeral -1].id = 0;
                                contGeral--;
                            }
                        }

                        for(int cont = 1; cont < contGeral; cont++){
                            for(i = 0; i < contGeral-1; i++) {
                                if (geral[i].prioridade > geral[i+1].prioridade) {
                                    Cli aux = geral[i];
                                    geral[i] = geral[i+1];
                                    geral[i+1] = aux;
                                }
                            }
                        }

                        tdata.geral = geral;
                    }

                    strcpy(cliente.sintoma, "");
                    strcpy(cliente.respostaCliente, "");
                } else {

                    if (contaClientes >= bal.MAXCLIENTES) {

                        sprintf(fifocli, FIFO_CLI, cliente.id);

                        ff = open(fifocli, O_WRONLY);
                        if (ff == -1) {
                            printf("[ERRO] Não foi possível abrir o FIFO!\n");
                            close(fd);

                            unlink(FIFO_SRD);
                        }

                        strcpy(cliente.respostaBalcao, "cheio");
                        n = write(ff, &cliente, sizeof(Cli));
                        if (n == -1) {
                            printf("[ERRO] Não foi possível abrir o FIFO!\n");
                            close(fd);

                            unlink(FIFO_SRD);
                        }

                        close(ff);

                    } else {
                        if (cliente.estado == 0) {
                            printf("\n---NOVO CLIENTE--- ");
                            printf("\nNome -> %s\n", cliente.nome);
                            printf("Sintoma -> %s\n", cliente.sintoma);

                            erro_write = write(envia[1], strcat(cliente.sintoma, "\n"), strlen(cliente.sintoma) + 1);
                            if (erro_write == -1) {
                                fprintf(stderr, "[ERRO] na escrita\n");
                                exit(1);
                            }

                            erro_read = read(recebe[0], recebe_msg, 16);
                            if (erro_read == -1) {
                                fprintf(stderr, "[ERRO] na leitura\n");
                                exit(1);
                            }

                            recebe_msg[erro_read] = 0;

                            strcpy(token, recebe_msg);
                            strcpy(cliente.especialidade, strtok(recebe_msg, " "));
                            cliente.prioridade = atoi(strtok(NULL, " "));

                            printf("Especialidade : Prioridade -> %s : %d\n\n", cliente.especialidade,
                                   cliente.prioridade);
                            fflush(stdout);

                            strcpy(cliente.respostaClassificador, token);

                            cliente.estado = 1;

                            int contFila = 0;
                            bool filaCheia = false;
                            if (strcmp(cliente.especialidade, "oftalmologia") == 0) {

                                if (contOftalmologia >= 5)
                                    filaCheia = true;

                                for (i = 0; i < contOftalmologia; i++) {
                                    if (oftalmologia[i].prioridade <= cliente.prioridade)
                                        contFila++;
                                }

                            } else if (strcmp(cliente.especialidade, "neurologia") == 0) {

                                if (contNeurologia >= 5)
                                    filaCheia = true;

                                for (i = 0; i < contNeurologia; i++) {
                                    if (neurologia[i].prioridade <= cliente.prioridade)
                                        contFila++;
                                }

                            } else if (strcmp(cliente.especialidade, "estomatologia") == 0) {

                                if (contEstomatologia >= 5)
                                    filaCheia = true;

                                for (i = 0; i < contEstomatologia; i++) {
                                    if (estomatologia[i].prioridade <= cliente.prioridade)
                                        contFila++;
                                }

                            } else if (strcmp(cliente.especialidade, "ortopedia") == 0) {

                                if (contOrtopedia >= 5)
                                    filaCheia = true;

                                for (i = 0; i < contOrtopedia; i++) {
                                    if (ortopedia[i].prioridade <= cliente.prioridade)
                                        contFila++;
                                }

                            } else if (strcmp(cliente.especialidade, "geral") == 0) {

                                if (contGeral >= 5)
                                    filaCheia = true;

                                for (i = 0; i < contGeral; i++) {
                                    if (geral[i].prioridade <= cliente.prioridade)
                                        contFila++;
                                }

                            }

                            cliente.nrUtentes = contFila;

                            int contEspecialistas = 0;

                            for (i = 0; i < contaMedicos; i++) {
                                if (strcmp(arrayMedicos[i].especialidade, cliente.especialidade) == 0)
                                    contEspecialistas++;
                            }

                            if (filaCheia == true)
                                strcpy(cliente.respostaBalcao, "cheio");

                            cliente.nrEspecialistas = contEspecialistas;

                            //ABRIR FIFO DO CLIENTE
                            sprintf(fifocli, FIFO_CLI, cliente.id);

                            ff = open(fifocli, O_WRONLY);
                            if (ff == -1) {
                                printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                exit(1);

                            }

                            n = write(ff, &cliente, sizeof(Cli));
                            if (n == -1) {
                                printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                close(fd);

                                unlink(FIFO_SRD);
                            }

                            close(ff);

                            if (filaCheia != true) {
                                bool existe = false;
                                for (int i = 0; i < contaMedicos; i++) {
                                    if (strcmp(arrayMedicos[i].especialidade, cliente.especialidade) == 0 &&
                                        arrayMedicos[i].estado != 2) {

                                        existe = true;
                                        printf("Existem agora medicos com a mesma especialidade do cliente %s\n",
                                               cliente.nome);
                                        //ABRIR FIFO DO MEDICO
                                        sprintf(fifomed, FIFO_MED, arrayMedicos[i].id);


                                        fdbalmed = open(fifomed, O_WRONLY);
                                        if (fdbalmed == -1) {
                                            printf("[ERRO] Não foi possível abrir o FIFO MEDICO!\n");
                                            exit(1);
                                        }

                                        cliente.estado = 2;
                                        cliente.medico = arrayMedicos[i].id;

                                        arrayMedicos[i].estado = 2;
                                        arrayMedicos[i].cliente = cliente.id;
                                        strcpy(arrayMedicos[i].perguntaMedico, "");

                                        n = write(fdbalmed, &cliente, sizeof(Cli));
                                        if (n == -1) {
                                            printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                            close(fd);

                                            unlink(FIFO_SRD);
                                        }

                                        close(fdbalmed);

                                        sprintf(fifocli, FIFO_CLI, cliente.id);

                                        ff = open(fifocli, O_WRONLY);
                                        if (ff == -1) {
                                            printf("[ERRO] Não foi possível abrir o FIFO MEDICO!\n");
                                            exit(1);
                                        }

                                        n = write(ff, &arrayMedicos[i], sizeof(Med));
                                        if (n == -1) {
                                            printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                            close(fd);

                                            unlink(FIFO_SRD);
                                        }

                                        close(ff);

                                        printf("\nO cliente %s comecou a ser atendido pelo Dr.%s\n", cliente.nome,
                                               arrayMedicos[i].nome);
                                        break;
                                    }
                                }

                                if (!existe) {
                                    printf("Não existem medicos com a mesma especialidade do cliente %s\n",
                                           cliente.nome);

                                    if (strcmp(cliente.especialidade, "oftalmologia") == 0 && contOftalmologia < 5) {

                                        oftalmologia[contOftalmologia] = cliente;
                                        contOftalmologia++;

                                        for (int cont = 1; cont < contOftalmologia; cont++) {
                                            for (i = 0; i < contOftalmologia - 1; i++) {
                                                if (oftalmologia[i].prioridade > oftalmologia[i + 1].prioridade) {
                                                    Cli aux = oftalmologia[i];
                                                    oftalmologia[i] = oftalmologia[i + 1];
                                                    oftalmologia[i + 1] = aux;
                                                }
                                            }
                                        }

                                        for (int z = 0; z < contOftalmologia; z++) {
                                            if (oftalmologia[z].id == cliente.id) {

                                                for (int x = z + 1; x < contOftalmologia; x++) {


                                                    sprintf(fifocli, FIFO_CLI, oftalmologia[x].id);

                                                    ff = open(fifocli, O_WRONLY);
                                                    if (ff == -1) {
                                                        printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                                        exit(1);

                                                    }

                                                    medico.clientesAfrente = 1;

                                                    strcpy(medico.respostaBalcao, "");
                                                    n = write(ff, &medico, sizeof(Med));
                                                    if (n == -1) {
                                                        printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                                        close(fd);

                                                        unlink(FIFO_SRD);
                                                    }
                                                    close(ff);

                                                    medico.clientesAfrente = 0;
                                                }

                                            }
                                        }

                                        tdata.oftalmologia = oftalmologia;

                                    } else if (strcmp(cliente.especialidade, "neurologia") == 0 && contNeurologia < 5) {

                                        neurologia[contNeurologia] = cliente;
                                        contNeurologia++;

                                        for (int cont = 1; cont < contNeurologia; cont++) {
                                            for (i = 0; i < contNeurologia - 1; i++) {
                                                if (neurologia[i].prioridade > neurologia[i + 1].prioridade) {
                                                    Cli aux = neurologia[i];
                                                    neurologia[i] = neurologia[i + 1];
                                                    neurologia[i + 1] = aux;
                                                }
                                            }
                                        }

                                        for (int z = 0; z < contNeurologia; z++) {
                                            if (neurologia[z].id == cliente.id) {

                                                for (int x = z + 1; x < contNeurologia; x++) {


                                                    sprintf(fifocli, FIFO_CLI, neurologia[x].id);

                                                    ff = open(fifocli, O_WRONLY);
                                                    if (ff == -1) {
                                                        printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                                        exit(1);

                                                    }

                                                    medico.clientesAfrente = 1;

                                                    strcpy(medico.respostaBalcao, "");
                                                    n = write(ff, &medico, sizeof(Med));
                                                    if (n == -1) {
                                                        printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                                        close(fd);

                                                        unlink(FIFO_SRD);
                                                    }
                                                    close(ff);

                                                    medico.clientesAfrente = 0;
                                                }

                                            }
                                        }

                                        tdata.neurologia = neurologia;

                                    } else if (strcmp(cliente.especialidade, "estomatologia") == 0 &&
                                               contEstomatologia < 5) {

                                        estomatologia[contEstomatologia] = cliente;
                                        contEstomatologia++;

                                        for (int cont = 1; cont < contEstomatologia; cont++) {
                                            for (i = 0; i < contEstomatologia - 1; i++) {
                                                if (estomatologia[i].prioridade > estomatologia[i + 1].prioridade) {
                                                    Cli aux = estomatologia[i];
                                                    estomatologia[i] = estomatologia[i + 1];
                                                    estomatologia[i + 1] = aux;
                                                }
                                            }
                                        }

                                        for (int z = 0; z < contEstomatologia; z++) {
                                            if (estomatologia[z].id == cliente.id) {

                                                for (int x = z + 1; x < contEstomatologia; x++) {


                                                    sprintf(fifocli, FIFO_CLI, estomatologia[x].id);

                                                    ff = open(fifocli, O_WRONLY);
                                                    if (ff == -1) {
                                                        printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                                        exit(1);

                                                    }

                                                    medico.clientesAfrente = 1;

                                                    strcpy(medico.respostaBalcao, "");
                                                    n = write(ff, &medico, sizeof(Med));
                                                    if (n == -1) {
                                                        printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                                        close(fd);

                                                        unlink(FIFO_SRD);
                                                    }
                                                    close(ff);

                                                    medico.clientesAfrente = 0;
                                                }

                                            }
                                        }

                                        tdata.estomatologia = estomatologia;

                                    } else if (strcmp(cliente.especialidade, "ortopedia") == 0 && contOrtopedia < 5) {

                                        ortopedia[contOrtopedia] = cliente;
                                        contOrtopedia++;


                                        for (int cont = 1; cont < contOrtopedia; cont++) {
                                            for (i = 0; i < contOrtopedia - 1; i++) {
                                                if (ortopedia[i].prioridade > ortopedia[i + 1].prioridade) {
                                                    Cli aux = ortopedia[i];
                                                    ortopedia[i] = ortopedia[i + 1];
                                                    ortopedia[i + 1] = aux;
                                                }
                                            }
                                        }

                                        for (int z = 0; z < contOrtopedia; z++) {
                                            if (ortopedia[z].id == cliente.id) {

                                                for (int x = z + 1; x < contOrtopedia; x++) {


                                                    sprintf(fifocli, FIFO_CLI, ortopedia[x].id);

                                                    ff = open(fifocli, O_WRONLY);
                                                    if (ff == -1) {
                                                        printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                                        exit(1);

                                                    }

                                                    medico.clientesAfrente = 1;

                                                    strcpy(medico.respostaBalcao, "");
                                                    n = write(ff, &medico, sizeof(Med));
                                                    if (n == -1) {
                                                        printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                                        close(fd);

                                                        unlink(FIFO_SRD);
                                                    }
                                                    close(ff);

                                                    medico.clientesAfrente = 0;
                                                }

                                            }
                                        }
                                        /*for (i = 0; i < contOrtopedia - 1; i++) {
                                          for (j = i + 1; j < contOrtopedia; j++) {

                                              cliAux = ortopedia[i];
                                            if (ortopedia[i].prioridade > ortopedia[j].prioridade) {
                                                ortopedia[i] = ortopedia[j];
                                                ortopedia[j] = cliAux;
                                            }

                                        }
                                    }*/

                                        tdata.ortopedia = ortopedia;

                                    } else if (strcmp(cliente.especialidade, "geral") == 0 && contGeral < 5) {

                                        geral[contGeral] = cliente;
                                        contGeral++;

                                        for (int cont = 1; cont < contGeral; cont++) {
                                            for (i = 0; i < contGeral - 1; i++) {
                                                if (geral[i].prioridade > geral[i + 1].prioridade) {
                                                    Cli aux = geral[i];
                                                    geral[i] = geral[i + 1];
                                                    geral[i + 1] = aux;
                                                }
                                            }
                                        }

                                        for (int z = 0; z < contGeral; z++) {
                                            if (geral[z].id == cliente.id) {

                                                for (int x = z + 1; x < contGeral; x++) {


                                                    sprintf(fifocli, FIFO_CLI, geral[x].id);

                                                    ff = open(fifocli, O_WRONLY);
                                                    if (ff == -1) {
                                                        printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                                        exit(1);

                                                    }

                                                    medico.clientesAfrente = 1;

                                                    strcpy(medico.respostaBalcao, "");
                                                    n = write(ff, &medico, sizeof(Med));
                                                    if (n == -1) {
                                                        printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                                        close(fd);

                                                        unlink(FIFO_SRD);
                                                    }
                                                    close(ff);

                                                    medico.clientesAfrente = 0;
                                                }

                                            }
                                        }

                                        tdata.geral = geral;

                                    }

                                }


                                /*if(contOftalmologia == 0)
                                    oftalmologia[0] = cliente;
                                else {
                                    for(i = 0; i < 5; i++) {
                                        if(cliente.prioridade < oftalmologia[i]) {
                                            int existe2 = 1;
                                        }
                                    }

                                    if(!existe 2) {
                                        oftalmologia[contOftalmologia] = cliente;
                                        contOftalmologia++;
                                    } else {

                                    }

                                }*/



                                //guardar -> mudar
                                if (contaClientes < bal.MAXCLIENTES) {
                                    auxClientes = 0;

                                    //SERVE PARA VERIFICAR SE O CLIENTE JA SE ENCONTRA NA ESTRUTURA
                                    for (i = 0; i < contaClientes; i++) {
                                        if (cliente.id == arrayClientes[i].id) {
                                            auxClientes = 1;
                                            posCliIgual = i;
                                        }
                                    }

                                    //SE NAO SE ENCONTRAR NA ESTRUTURA GUARDA
                                    if (auxClientes == 0) {
                                        arrayClientes[contaClientes] = cliente;
                                        contaClientes++;
                                    } else {
                                        arrayClientes[posCliIgual] = cliente;
                                    }
                                } else {
                                    for (i = 0; i < contaClientes; i++) {
                                        if (arrayClientes[i].id == cliente.id) {
                                            arrayClientes[i] = cliente;
                                        }
                                    }

                                }
                            }
                        }
                    }
                }

                }
        }
        else if (resSelect > 0 && FD_ISSET(fdmed, &fds)){

            n = read(fdmed, &medico, sizeof(Med));

            if(n == sizeof(Med)) {

                if (medico.sinalDeVida == 1) {

                    for (i = 0; i < contaMedicos; i++) {
                        if (arrayMedicos[i].id == medico.id)
                            printf("O Dr.%s encontra-se ativo.\n", arrayMedicos[i].nome);
                    }

                    medico.sinalDeVida = 0;

                } else if (strcmp(medico.perguntaMedico, "sair") == 0) {

                    for (i = 0; i < contaMedicos; i++) {
                        if (arrayMedicos[i].id == medico.id)
                            printf("O Dr.%s com a especialidade %s foi embora.\n", arrayMedicos[i].nome,
                                   arrayMedicos[i].especialidade);
                    }


                    sprintf(fifomed, FIFO_MED, medico.id);

                    fdbalmed = open(fifomed, O_WRONLY);
                    if (fdbalmed == -1) {
                        printf("[ERRO] Não foi possível abrir o FIFO!\n");
                        close(fd);
                        unlink(FIFO_SRDMED);
                    }

                    strcpy(cliente.respostaBalcao, "encerra");
                    n = write(fdbalmed, &cliente, sizeof(Cli));
                    if (n == -1) {
                        printf("[ERRO] Não foi possível abrir o FIFO!\n");
                        close(fd);

                        unlink(FIFO_SRD);
                    }

                    close(fdbalmed);

                    for (i = 0; i < contaMedicos; i++) {
                        if (arrayMedicos[i].id == medico.id) {
                            arrayMedicos[i] = arrayMedicos[contaMedicos - 1];
                            contaMedicos--;
                        }
                    }

                } else {

                    bool existeMedicoNoSistema = false;

                    for (i = 0; i < contaMedicos; i++) {
                        if (arrayMedicos[i].id == medico.id)
                            existeMedicoNoSistema = true;
                    }

                    if (contaMedicos >= bal.MAXMEDICOS && !existeMedicoNoSistema) {

                        sprintf(fifomed, FIFO_MED, medico.id);

                        fdbalmed = open(fifomed, O_WRONLY);
                        if (fdbalmed == -1) {
                            printf("[ERRO] Não foi possível abrir o FIFO!\n");
                            close(fd);
                            unlink(FIFO_SRDMED);
                        }

                        strcpy(cliente.respostaBalcao, "cheio");
                        n = write(fdbalmed, &cliente, sizeof(Cli));
                        if (n == -1) {
                            printf("[ERRO] Não foi possível abrir o FIFO!\n");
                            close(fd);

                            unlink(FIFO_SRD);
                        }

                        close(fdbalmed);
                    } else {

                        if (existeMedicoNoSistema == true) {
                            printf("\nO Dr.%s com a especialidade %s esta disponivel novamente.\n", medico.nome,
                                   medico.especialidade);
                        } else {
                            printf("\n---NOVO MEDICO--- ");
                            printf("\nNome -> %s\n", medico.nome);
                            printf("Especialidade -> %s\n\n", medico.especialidade);

                        }
                        int existe = -1;
                        int idCliente = 0;

                        if (strcmp(medico.especialidade, "oftalmologia") == 0 && contOftalmologia != 0) {
                            idCliente = oftalmologia[0].id;
                        } else if (strcmp(medico.especialidade, "neurologia") == 0 && contNeurologia != 0) {
                            idCliente = neurologia[0].id;
                        } else if (strcmp(medico.especialidade, "estomatologia") == 0 && contEstomatologia != 0) {
                            idCliente = estomatologia[0].id;
                        } else if (strcmp(medico.especialidade, "ortopedia") == 0 && contOrtopedia != 0) {
                            idCliente = ortopedia[0].id;
                        } else if (strcmp(medico.especialidade, "geral") == 0 && contGeral != 0) {
                            idCliente = geral[0].id;
                        }

                        if (idCliente != 0) {
                            for (int i = 0; i < contaClientes; i++) {

                                if (arrayClientes[i].id == idCliente && arrayClientes[i].estado != 2) {

                                    existe = i;
                                    printf("Existem agora clientes com a mesma especialidade do Dr.%s\n", medico.nome);
                                    //ABRIR FIFO DO MEDICO
                                    sprintf(fifomed, FIFO_MED, medico.id);

                                    fdbalmed = open(fifomed, O_WRONLY);
                                    if (fdbalmed == -1) {
                                        printf("[ERRO] Não foi possível abrir o FIFO MEDICO!\n");
                                        exit(1);
                                    }

                                    arrayClientes[i].estado = 2;
                                    arrayClientes[i].medico = medico.id;

                                    medico.estado = 2;
                                    medico.cliente = arrayClientes[i].id;
                                    strcpy(medico.perguntaMedico, "");

                                    n = write(fdbalmed, &arrayClientes[i], sizeof(Cli));
                                    if (n == -1) {
                                        printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                        close(fd);

                                        unlink(FIFO_SRD);
                                    }
                                    close(fdbalmed);

                                    sprintf(fifocli, FIFO_CLI, arrayClientes[i].id);

                                    ff = open(fifocli, O_WRONLY);
                                    if (ff == -1) {
                                        printf("[ERRO] Não foi possível abrir o FIFO MEDICO!\n");
                                        exit(1);
                                    }

                                    n = write(ff, &medico, sizeof(Med));
                                    if (n == -1) {
                                        printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                        close(fd);

                                        unlink(FIFO_SRD);
                                    }
                                    close(ff);

                                    printf("\nO cliente %s comecou a ser atendido pelo Dr.%s\n", medico.nome,
                                           arrayClientes[i].nome);

                                }
                            }

                            if (strcmp(arrayClientes[existe].especialidade, "oftalmologia") == 0) {
                                for (j = 0; j < contOftalmologia; j++) {
                                    if (oftalmologia[j].id == arrayClientes[existe].id) {

                                        for (int x = j + 1; x < contOftalmologia; x++) {


                                            sprintf(fifocli, FIFO_CLI, oftalmologia[x].id);

                                            ff = open(fifocli, O_WRONLY);
                                            if (ff == -1) {
                                                printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                                exit(1);
                                            }

                                            medico.clientesAfrente = -1;

                                            strcpy(medico.respostaBalcao, "");
                                            n = write(ff, &medico, sizeof(Med));
                                            if (n == -1) {
                                                printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                                close(fd);

                                                unlink(FIFO_SRD);
                                            }
                                            close(ff);

                                            medico.clientesAfrente = 0;
                                        }

                                        oftalmologia[j] = oftalmologia[contOftalmologia - 1];
                                        oftalmologia[contOftalmologia - 1].id = 0;
                                        contOftalmologia--;
                                    }
                                }

                                for (int cont = 1; cont < contOftalmologia; cont++) {
                                    for (i = 0; i < contOftalmologia - 1; i++) {
                                        if (oftalmologia[i].prioridade > oftalmologia[i + 1].prioridade) {
                                            Cli aux = oftalmologia[i];
                                            oftalmologia[i] = oftalmologia[i + 1];
                                            oftalmologia[i + 1] = aux;
                                        }
                                    }
                                }

                                tdata.oftalmologia = oftalmologia;

                            } else if (strcmp(arrayClientes[existe].especialidade, "neurologia") == 0) {
                                for (j = 0; j < contNeurologia; j++) {
                                    if (neurologia[j].id == arrayClientes[existe].id) {

                                        for (int x = j + 1; x < contNeurologia; x++) {

                                            sprintf(fifocli, FIFO_CLI, neurologia[x].id);

                                            ff = open(fifocli, O_WRONLY);
                                            if (ff == -1) {
                                                printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                                exit(1);
                                            }

                                            medico.clientesAfrente = -1;

                                            strcpy(medico.respostaBalcao, "");
                                            n = write(ff, &medico, sizeof(Med));
                                            if (n == -1) {
                                                printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                                close(fd);

                                                unlink(FIFO_SRD);
                                            }
                                            close(ff);

                                            medico.clientesAfrente = 0;
                                        }

                                        neurologia[j] = neurologia[contNeurologia - 1];
                                        neurologia[contNeurologia - 1].id = 0;
                                        contNeurologia--;
                                    }
                                }

                                for (int cont = 1; cont < contNeurologia; cont++) {
                                    for (i = 0; i < contNeurologia - 1; i++) {
                                        if (neurologia[i].prioridade > neurologia[i + 1].prioridade) {
                                            Cli aux = neurologia[i];
                                            neurologia[i] = neurologia[i + 1];
                                            neurologia[i + 1] = aux;
                                        }
                                    }
                                }

                                tdata.neurologia = neurologia;

                            } else if (strcmp(arrayClientes[existe].especialidade, "estomatologia") == 0) {
                                for (j = 0; j < contEstomatologia; j++) {
                                    if (estomatologia[j].id == arrayClientes[existe].id) {

                                        for (int x = j + 1; x < contEstomatologia; x++) {


                                            sprintf(fifocli, FIFO_CLI, estomatologia[x].id);

                                            ff = open(fifocli, O_WRONLY);
                                            if (ff == -1) {
                                                printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                                exit(1);
                                            }

                                            medico.clientesAfrente = -1;

                                            strcpy(medico.respostaBalcao, "");
                                            n = write(ff, &medico, sizeof(Med));
                                            if (n == -1) {
                                                printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                                close(fd);

                                                unlink(FIFO_SRD);
                                            }
                                            close(ff);

                                            medico.clientesAfrente = 0;
                                        }

                                        estomatologia[j] = estomatologia[contEstomatologia - 1];
                                        estomatologia[contEstomatologia - 1].id = 0;
                                        contEstomatologia--;
                                    }
                                }

                                for (int cont = 1; cont < contEstomatologia; cont++) {
                                    for (i = 0; i < contEstomatologia - 1; i++) {
                                        if (estomatologia[i].prioridade > estomatologia[i + 1].prioridade) {
                                            Cli aux = estomatologia[i];
                                            estomatologia[i] = estomatologia[i + 1];
                                            estomatologia[i + 1] = aux;
                                        }
                                    }
                                }

                                tdata.estomatologia = estomatologia;

                            } else if (strcmp(arrayClientes[existe].especialidade, "ortopedia") == 0) {
                                for (j = 0; j < contOrtopedia; j++) {
                                    if (ortopedia[j].id == arrayClientes[existe].id) {

                                        for (int x = j + 1; x < contOrtopedia; x++) {


                                            sprintf(fifocli, FIFO_CLI, ortopedia[x].id);

                                            ff = open(fifocli, O_WRONLY);
                                            if (ff == -1) {
                                                printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                                exit(1);
                                            }

                                            medico.clientesAfrente = -1;

                                            strcpy(medico.respostaBalcao, "");
                                            n = write(ff, &medico, sizeof(Med));
                                            if (n == -1) {
                                                printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                                close(fd);

                                                unlink(FIFO_SRD);
                                            }
                                            close(ff);

                                            medico.clientesAfrente = 0;
                                        }

                                        ortopedia[j] = ortopedia[contOrtopedia - 1];
                                        ortopedia[contOrtopedia - 1].id = 0;
                                        contOrtopedia--;
                                    }
                                }

                                for (int cont = 1; cont < contOrtopedia; cont++) {
                                    for (i = 0; i < contOrtopedia - 1; i++) {
                                        if (ortopedia[i].prioridade > ortopedia[i + 1].prioridade) {
                                            Cli aux = ortopedia[i];
                                            ortopedia[i] = ortopedia[i + 1];
                                            ortopedia[i + 1] = aux;
                                        }
                                    }
                                }

                                tdata.ortopedia = ortopedia;

                            } else if (strcmp(arrayClientes[existe].especialidade, "geral") == 0) {
                                for (j = 0; j < contGeral; j++) {
                                    if (geral[j].id == arrayClientes[existe].id) {

                                        for (int x = j + 1; x < contGeral; x++) {


                                            sprintf(fifocli, FIFO_CLI, geral[x].id);

                                            ff = open(fifocli, O_WRONLY);
                                            if (ff == -1) {
                                                printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                                exit(1);
                                            }

                                            medico.clientesAfrente = -1;

                                            strcpy(medico.respostaBalcao, "");
                                            n = write(ff, &medico, sizeof(Med));
                                            if (n == -1) {
                                                printf("[ERRO] Não foi possível abrir o FIFO!\n");
                                                close(fd);

                                                unlink(FIFO_SRD);
                                            }
                                            close(ff);

                                            medico.clientesAfrente = 0;
                                        }

                                        geral[j] = geral[contGeral - 1];
                                        geral[contGeral - 1].id = 0;
                                        contGeral--;
                                    }
                                }

                                for (int cont = 1; cont < contGeral; cont++) {
                                    for (i = 0; i < contGeral - 1; i++) {
                                        if (geral[i].prioridade > geral[i + 1].prioridade) {
                                            Cli aux = geral[i];
                                            geral[i] = geral[i + 1];
                                            geral[i + 1] = aux;
                                        }
                                    }
                                }

                                tdata.geral = geral;

                            }

                        } else
                            printf("Não existem clientes com a mesma especialidade do Dr.%s\n", medico.nome);


                        //guardar -> mudarrr
                        if (contaMedicos < bal.MAXMEDICOS) {
                            auxMedicos = 0;

                            //SERVE PARA VERIFICAR SE O CLIENTE JA SE ENCONTRA NA ESTRUTURA
                            for (i = 0; i < contaMedicos; i++) {
                                if (medico.id == arrayMedicos[i].id) {
                                    auxMedicos = 1;
                                    posMedIgual = i;
                                }
                            }

                            //SE NAO SE ENCONTRAR NA ESTRUTURA GUARDA
                            if (auxMedicos == 0) {
                                arrayMedicos[contaMedicos] = medico;
                                contaMedicos++;
                            } else {
                                arrayMedicos[posMedIgual] = medico;
                            }
                        } else {
                            for (i = 0; i < contaClientes; i++) {
                                if (arrayClientes[i].id == cliente.id) {
                                    arrayClientes[i] = cliente;
                                }
                            }
                        }

                    }
                }
            }
        }
        }while(strncmp(comandos,"encerra",7) != 0);
    }

    write(envia[1],"#fim\n",5); // Fecha o classificador
    wait(&res);

 	close(fd);
 	close(fdmed);

	unlink(FIFO_SRD);
	unlink(FIFO_SRDMED);
    
    close(envia[1]);
    close(recebe[0]);

    tdata.continua = 0;
    pthread_join(tid, NULL);
    //pthread_mutex_destroy(&trinco);

    exit(0);
}

int getMAXCLIENTES()
{
    int valor;
    char *clientes;

    clientes = getenv("MAXCLIENTES");

    if (clientes == NULL)
    {
        fprintf(stderr, "\nA variável MAXCLIENTES nao foi definida\n");
        fprintf(stderr, "MAXMEDICOS foi inicializado com valor default\n");
        return 10;
    }

    if((sscanf(clientes, "%d", &valor)!=1) || valor < 1)
    {
        fprintf(stderr, "\nA variável MAXCLIENTES foi definida com  um valor incorreto: %s\n",valor);
        fprintf(stderr, "A variável MAXCLIENTES deve ser um valor inteiro maior do que 1\n");
        return -1;
    }

    return valor;
}


int getMAXMEDICOS()
{

    int valor;
    char *medicos;

    medicos = getenv("MAXMEDICOS");

    if (medicos == NULL)
    {
        fprintf(stderr, "A variável MAXCMEDICOS nao foi definida\n");
        fprintf(stderr, "MAXMEDICOS foi inicializado com valor default\n");
        return 10;

    }

    if ((sscanf(medicos, "%d", &valor)!=1) || valor < 1)
    {
        fprintf(stderr, "A variável MAXMEDICOS foi definida com  um valor incorreto: %s\n",valor);
        fprintf(stderr, "A variável deve ser um valor inteiro maior do que 1\n");
        return -1;
    }

    return valor;
}
