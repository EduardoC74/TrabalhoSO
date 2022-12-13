#include "medico.h"
#include "balcao.h"

void trataSinalDeVida(int num, siginfo_t *info, void *uc)
{
    Med medico;
    int fd, n;

    //ABRIR FIFO PARA ESCRITA
    fd = open(FIFO_SRDMED, O_WRONLY);
    if(fd == -1){
        printf("[ERRO] Não foi possível abrir o FIFO!\n");
        exit(2);
    }

    medico.sinalDeVida = 1;
    medico.id = getpid();

    n = write(fd, &medico, sizeof(Med));
    if (n == -1) {
        printf("[ERRO] Não foi possível abrir o FIFO!\n");
        exit(2);
    }

    medico.sinalDeVida = 0;

    close(fd);
}

void trataControlC(int num, siginfo_t *info, void *uc)
{
    Med medico;
    int fd, n;

    //ABRIR FIFO PARA ESCRITA
    fd = open(FIFO_SRDMED, O_WRONLY);
    if(fd == -1){
        printf("[ERRO] Não foi possível abrir o FIFO!\n");
        exit(2);
    }

    strcpy(medico.perguntaMedico, "sair");
    medico.id = getpid();

    n = write(fd, &medico, sizeof(Med));
    if (n == -1) {
        printf("[ERRO] Não foi possível abrir o FIFO!\n");
        exit(2);
    }

    close(fd);
}

int main(int argc, char *argv[]){
    int n, res, atendido = 0, fd, ff, fg, fj, pidcli, pidmed;
    char fifomed[40], fifocli[40], fifoclimed[40], fifomedcli[40], resposta[40], perguntaMedico[40], respostaCliente[40];
    Med medico;
    Cli cliente;
    fd_set fds;
    struct timeval tempo;


    //Sinal De Vida
    struct sigaction actVida;
    actVida.sa_sigaction = trataSinalDeVida;
    actVida.sa_flags = SA_SIGINFO;
    sigaction(SIGALRM, &actVida, NULL);

    //Ctrl+C
    struct sigaction actControlC;
    actControlC.sa_sigaction = trataControlC;
    actControlC.sa_flags = SA_SIGINFO;
    sigaction(SIGINT,&actControlC,NULL);

    if(argc != 3)
    {
        printf("ERRO");
        exit(1);
    }

    strcpy(medico.nome, argv[1]);
    strcpy(medico.especialidade,  argv[2]);
    medico.id = getpid();
    medico.estado = 0;
    medico.clientesAfrente = 0;
	medico.sinalDeVida = 0;

	//VERIFICAR SE O FIFO NÃO EXISTE | ACCESS
	if(access(FIFO_SRDMED, F_OK) != 0){
		printf("[ERRO] O FIFO não existe!\n");
		exit(1);
	}
	
	//ABRIR FIFO PARA ESCRITA
	fd = open(FIFO_SRDMED, O_WRONLY);
	if(fd == -1){
		printf("[ERRO] Não foi possível abrir o FIFO!\n");
		exit(2);
	}
	
	//CRIAR O FIFO
	sprintf(fifomed, FIFO_MED, getpid());
	if(mkfifo(fifomed, 0600) == -1){
		printf("[ERRO] Não foi possível criar o FIFO!\n");
		exit(1);
	}
	
	ff = open(fifomed, O_RDWR);
	if(ff == -1){
		printf("[ERRO] Não foi possível abrir o FIFO!\n");
		exit(1);
	}

    n = write(fd, &medico, sizeof(Med));
    if (n == -1) {
        printf("[ERRO] Não foi possível abrir o FIFO!\n");
        exit(2);
    }

    printf("\nAguarde por um cliente Dr.%s\n", medico.nome);

	while(1) {

        alarm(20);

		FD_ZERO(&fds);
		FD_SET(0, &fds);
        FD_SET(ff, &fds);
		tempo.tv_sec = 8;
		tempo.tv_usec = 0;
		res = select(ff + 1, &fds, NULL, NULL, NULL);

        if(res == 0){

		}
		else if (res > 0 && FD_ISSET(0, &fds)){ // dados do teclado

            scanf(" %[^\n]", medico.perguntaMedico);

            if(strncmp(medico.perguntaMedico,"sair",5) == 0){
                n = write(fd, &medico, sizeof(Med));
                if (n == -1) {
                    printf("[ERRO] Não foi possível abrir o FIFO!\n");
                    exit(2);
                }
            }

            if(medico.estado == 2) {
                sprintf(fifocli, FIFO_CLI, cliente.id);

                fg = open(fifocli, O_WRONLY);
                if (fg == -1) {
                    printf("[ERRO] Não foi possível abrir o FIFO!\n");
                    break;
                }

                n = write(fg, &medico, sizeof(Med));
                if (n == -1) {
                    printf("[ERRO] Não foi possível abrir o FIFO!\n");
                    exit(2);
                }

                if (strncmp(medico.perguntaMedico, "adeus", 5) == 0) {
                    medico.estado = 0;

                    printf("\nA consulta com o cliente %s acabou!", cliente.nome);
                    printf("\nAguarde que lhe seja reencaminhado outro cliente Dr.%s\n\n", medico.nome);

                    atendido = 0;

                    strcpy(medico.respostaBalcao, "");
                    strcpy(medico.nomeCliente, "");
                    strcpy(medico.sintomaCliente, "");
                    strcpy(medico.perguntaMedico, "");
                    strcpy(medico.respostaCliente, "");
                    medico.cliente = 0;

                    n = write(fd, &medico, sizeof(Med));
                    if (n == -1) {
                        printf("[ERRO] Não foi possível abrir o FIFO!\n");
                        exit(2);
                    }

                }
            }
        }
    else if(res > 0 && FD_ISSET(ff, &fds)) {

            medico.estado = 2;
            n = read(ff, &cliente, sizeof(Cli));

            if(strncmp(cliente.respostaBalcao,"encerra",7) == 0){
                printf("\nO MEDICO terminou!!!\n");
                strcpy(cliente.respostaBalcao, "");
                medico.sinalDeVida = 0;
                close(ff);
                unlink(fifomed);
                close(fd);
                break;
            } else if(strncmp(cliente.respostaBalcao,"cheio",5) == 0){
                printf("\nO BALCAO nao tem mais vagas!!!\n");
                printf("Volte mais tarde\n");
                strcpy(cliente.respostaBalcao, "");
                medico.sinalDeVida = 0;
                close(ff);
                unlink(fifomed);
                close(fd);
                break;
            }

            if(!atendido) {
                printf("%s voce tem um cliente para atender!!!\n", medico.nome);
                printf("O cliente chama se %s, e o seu sintoma e %s\n", cliente.nome, cliente.sintoma);

                printf("\n[MEDICO]: \n");
            }

            if(medico.estado == 0)
            {

            } else {
                atendido = 1;

                    if(strcmp(cliente.respostaCliente, "") != 0) {
                        cliente.respostaCliente[n] = '\0';
                        printf("[CLIENTE]: %s\n", cliente.respostaCliente);

                        printf("[MEDICO]: \n");
                    }


                if(strncmp(cliente.respostaCliente,"adeus",5) == 0 || strncmp(cliente.respostaCliente, "sair", 4) == 0){
                    medico.estado = 0;

                    printf("\nA consulta com o cliente %s acabou!", cliente.nome);
                    printf("\nAguarde que lhe seja reencaminhado outro cliente Dr.%s\n\n", medico.nome);

                    strcpy(medico.respostaBalcao, "");
                    strcpy(medico.nomeCliente, "");
                    strcpy(medico.sintomaCliente, "");
                    strcpy(medico.perguntaMedico, "");
                    strcpy(medico.respostaCliente, "");
                    medico.cliente = 0;
                    medico.sinalDeVida = 0;

                    atendido = 0;

                    strcpy(medico.perguntaMedico, "");
                    n = write(fd, &medico, sizeof(Med));
                    if (n == -1) {
                        printf("[ERRO] Não foi possível abrir o FIFO!\n");
                        exit(2);
                    }
                }
            }

        }
	}

    close(fg);
	close(ff);
	close(fj);

    unlink(fifomed);
	unlink(fifoclimed);
	close(fd);
	
	exit(0);
}
