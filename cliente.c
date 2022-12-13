#include "cliente.h"
#include "balcao.h"

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

    strcpy(cliente.sintoma, "sair");
    cliente.id = getpid();
    cliente.estado = 2;

    n = write(fd, &cliente, sizeof(Cli));
    if (n == -1) {
        printf("[ERRO] Não foi possível abrir o FIFO!\n");
        exit(2);
    }

    close(fd);
}

int main(int argc, char *argv[]){

    int n, res, prioridade, atendido = 0, fd, ff, fg, fj, medpid;
    char fifocli[40], resposta[40], fifoclimed[40], fifomedcli[40], fifomed[40], especialidade[40], token[40], perguntaMedico[40], respostaCliente[40];
    Cli cliente;
    Med medico;
    fd_set fds;
    struct timeval tempo;

    //Ctrl+C
    struct sigaction actControlC;
    actControlC.sa_sigaction = trataControlC;
    actControlC.sa_flags = SA_SIGINFO;
    sigaction(SIGINT,&actControlC,NULL);

    if(argc !=2)
    {
        printf("Numero de argumentos invalido!!!\n");
        exit(1);
    }

	strcpy(cliente.nome, argv[1]);
    cliente.id = getpid();
    cliente.estado = 0;

    medico.estado = 0;

	//VERIFICAR SE O FIFO NÃO EXISTE
	if(access(FIFO_SRD, F_OK) != 0){
		printf("[ERRO] O FIFO não existe!\n");
		exit(1);
	}

    //ABRIR FIFO PARA ESCRITA
    fd = open(FIFO_SRD, O_WRONLY);
    if(fd == -1){
        printf("[ERRO] Não foi possível abrir o FIFO!\n");
        exit(1);
    }
	
	//CRIAR FIFO
	sprintf(fifocli, FIFO_CLI, cliente.id);
	if(mkfifo(fifocli, 0600) == -1){
		printf("[ERRO] Não foi possível criar o FIFO!\n");
		exit(1);
	}
	
	ff = open(fifocli, O_RDWR);
    if(ff == -1){
        printf("[ERRO] Não foi possível abrir o FIFO!\n");
        exit(1);
    }

    printf("\nQuais os seus sintomas %s? \n", cliente.nome);
    fflush(stdout);

	while(1) {

        FD_ZERO(&fds);
        FD_SET(0, &fds);
        FD_SET(ff, &fds);
        tempo.tv_sec = 8; //TIMOUT
        tempo.tv_usec = 0;
        res = select(ff + 1, &fds, NULL, NULL, NULL); //se quiser tempo infinito metes tempo a null

        if (res == 0) {

        } else if (res > 0 && FD_ISSET(0, &fds)) { // dados do teclado

                if(cliente.estado == 0){

                    scanf(" %[^\n]", cliente.sintoma);

                    n = write(fd, &cliente, sizeof(Cli));
                    if (n == -1) {
                        printf("[ERRO] Não foi possível abrir o FIFO!\n");
                        exit(2);
                    }

                    n = read(ff, &cliente, sizeof(Cli));

                    if (n > 0) {

                        if(strcmp(cliente.respostaBalcao, "cheio") == 0) {
                            printf("\nO BALCAO nao tem mais vagas!!!\n");
                            printf("Volte mais tarde\n");
                            close(ff);
                            unlink(fifocli);
                            close(fd);
                            break;
                        }
                        else if (strcmp(cliente.respostaBalcao, "encerra") != 0) {
                            printf("\nSintoma -> %s", cliente.sintoma);
                            printf("Especialidade : Prioridade -> %s : %d\n", cliente.especialidade, cliente.prioridade);
                            printf("Neste momento encontram-se %d clientes a sua frente\n", cliente.nrUtentes);
                            printf("Existem atualmente %d especialistas nesta area no sistema\n", cliente.nrEspecialistas);

                            printf("\nAguarde por um medico %s\n", cliente.nome);
                        } else {
                            printf("O CLIENTE terminou!!!\n");
                            close(ff);
                            unlink(fifocli);
                            close(fd);
                            break;
                        }
                    }
                } else if(cliente.estado == 1) {

                    scanf(" %[^\n]", cliente.sintoma);

                    n = write(fd, &cliente, sizeof(Cli));
                    if (n == -1) {
                        printf("[ERRO] Não foi possível abrir o FIFO!\n");
                        exit(2);
                    }

                } else if (cliente.estado == 2) {

                    scanf(" %[^\n]", cliente.respostaCliente);

                    sprintf(fifomed, FIFO_MED, medico.id);

                    fj = open(fifomed, O_WRONLY);
                    if (fj == -1) {
                        printf("[ERRO] Não foi possível abrir o FIFO!\n");
                        break;
                    }

                    n = write(fj, &cliente, sizeof(Cli));
                    if (n == -1) {
                        printf("[ERRO] Não foi possível abrir o FIFO!\n");
                        exit(2);
                    }

                    if (strncmp(cliente.respostaCliente, "adeus", 5) == 0 || strncmp(cliente.respostaCliente, "sair", 4) == 0) {
                        cliente.medico = medico.id;
                        n = write(fd, &cliente, sizeof(Cli));
                        if (n == -1) {
                            printf("[ERRO] Não foi possível abrir o FIFO!\n");
                            exit(2);
                        }
                    }
                }

        } else if (res > 0 && FD_ISSET(ff, &fds)) {

            n = read(ff, &medico, sizeof(Med));

            if (n > 0) {
                if (strcmp(medico.respostaBalcao, "encerra") == 0) {
                    printf("\nO CLIENTE terminou!!!\n");
                    close(ff);
                    unlink(fifocli);
                    close(fd);
                    break;
                } else if(strcmp(medico.respostaBalcao, "delut") == 0) {
                    cliente.medico = medico.id;
                    strcpy(cliente.respostaCliente, "sair");
                    n = write(fd, &cliente, sizeof(Cli));
                    if (n == -1) {
                        printf("[ERRO] Não foi possível abrir o FIFO!\n");
                        exit(2);
                    }
                }
            }

            if (medico.clientesAfrente == 1) {
                cliente.nrUtentes++;
                printf("\nEncontram-se agora %d clientes a sua frente\n", cliente.nrUtentes);
            } else if(medico.clientesAfrente == -1) {
                cliente.nrUtentes--;
                printf("\nEncontram-se agora %d clientes a sua frente\n", cliente.nrUtentes);
            } else if(strcmp(medico.respostaBalcao, "delut") != 0){

                cliente.estado = 2;

                if (atendido == 0) //estado =1
                    printf("%s, o Dr.%s encontra se a espera para o atender!!!\n\n", cliente.nome, medico.nome);

                atendido = 1;

                if (n > 0) {
                    if (strcmp(medico.perguntaMedico, "") != 0) {

                        medico.perguntaMedico[n] = '\0';
                        printf("[MEDICO]: %s\n", medico.perguntaMedico);
                    }

                    printf("[CLIENTE]: \n");

                }

                if (strncmp(medico.perguntaMedico, "adeus", 5) == 0 || strncmp(medico.perguntaMedico, "sair", 4) == 0) {
                    strcpy(cliente.respostaCliente, "adeus");
                    cliente.medico = medico.id;
                    n = write(fd, &cliente, sizeof(Cli));
                    if (n == -1) {
                        printf("[ERRO] Não foi possível abrir o FIFO!\n");
                        exit(2);
                    }
                }

            }
        }
    }
	
	close(fg);
	close(fj);
    close(ff);

	unlink(fifoclimed);
	unlink(fifocli);

    close(fd);

	exit(0);
}

