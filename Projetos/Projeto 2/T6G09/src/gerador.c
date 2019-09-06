#include "utilidades.c"

static int fechar_gerador = FALSE;
static int tempo_gerador;
static int tempo_U_relogio;

//_____________________________________________


void* veiculo_thread(void* arg)
{
    pthread_detach(pthread_self());
    veiculo *veic = (veiculo *)arg;

    clock_t tempo_vida;
    clock_t tempo_inicial;
    clock_t tempo_agr;
    tempo_inicial = clock();




    char nome_sem[64]; // "/mysemaphore"
    sprintf(nome_sem, "%s%c", "/mysemaphore", veic->direcao); // "mysemaphorex" onde x é N, S, E ou O

    sem_t * sem = sem_open(nome_sem,0, 0600, 1); //Abre semaforo

    char fifo_nome[128]; //Nome FIFO controlador
    sprintf(fifo_nome, "/tmp/FIFO%c", veic->direcao); //Cria o nome para abrir o FIFO controlador correspondente

    int fd_control;
    fd_control = open(fifo_nome, O_WRONLY);// Abre fifo controlador
    if(fd_control == -1)
    {
    	perror("ERRO a abrir Cntrolador");
        free(veic);
        return NULL;
    }


    char fifo_nome_unico[128];
    sprintf(fifo_nome_unico, "/tmp/FIFO%d", veic->id); //Cria o nome do FIFO unico do veiculo
    if(mkfifo(fifo_nome_unico, 0660) == -1)//Abre o FIFO unico do veiculo
        {
    	perror("ERRO a criar FIFO Veiculo "); //...
        }



    if(write(fd_control, veic, sizeof(veic)) == -1) //Escreve no FIFO de controlador
    {
      perror("ERRO a escrever FIFO Veiculo ");
      close(fd_control);
      unlink(fifo_nome_unico);
      free(veic);
      return NULL;
    }

    close(fd_control);
    sem_post(sem);

    sem_close(sem); //Fechar semaforo



    int fd_veiculo;
    fd_veiculo = open(fifo_nome_unico, O_RDONLY); //Abre o FIFO unico do veiculo
    if(fd_veiculo == -1)
    {
    	perror("ERRO a abrir FIFO Veiculo" );
    }


    char resposta[64];
    int r =0;
    while(TRUE) 
    {

		r = read(fd_veiculo, resposta, sizeof(resposta));
		if (r == -1)
			perror("ERRO a ler FIFO Veiculo" );		
		else if (r > 0)
			printf("Veiculo Resposta: %s\n", resposta); //////////////////////////////


		if (strcmp(resposta, "entrada") == 0) //Entrada
			geradorlog_reg(veic, "entrada", tempo_inicial, TRUE);
		else if (strcmp(resposta, "saida") == 0) //Saida
		{
			tempo_agr = clock();
			tempo_vida = tempo_agr - tempo_inicial;
			geradorlog_reg(veic, "saida", tempo_vida, FALSE);
			break; //para limpar
		}
		else if (strcmp(resposta, "cheio!") == 0)
		{
			tempo_agr = clock();
			tempo_vida = tempo_agr - tempo_inicial;
			geradorlog_reg(veic, "cheio!", tempo_vida, FALSE);
			break; //para limpar
		}
		else if (strcmp(resposta, "parquefechado") == 0)
		{
			tempo_agr = clock();
			tempo_vida = tempo_agr - tempo_inicial;
			geradorlog_reg(veic, "parquefechado", tempo_agr, FALSE);
			break; //para limpar
		}

    }

    close(fd_veiculo);
    unlink(fifo_nome_unico);
    free(veic);
    return NULL;
}

void alarm_handler(int sig)
{
    fechar_gerador = TRUE;
    printf("\n\n ----------O Gerador Fechou --------- \n");
}


int main(int argc, char* argv[])
{
	/**
	 * argc2 = tempo geracao
	 * argc3 = intervalo tempo minimo entre quaisquer dois eventos
	 */
	if (argc != 3)
	{
		printf("Usar: %s <T_geracao> <U_relogio>\n", argv[0]);
		exit(1);
	}
    tempo_gerador=atoi(argv[1]); //Tempo do gerador
    tempo_U_relogio =atoi(argv[2]); //Tempo minimo entre acoes


    if(tempo_gerador < 0 || tempo_U_relogio < 10)
    {
        printf("Insira valores válidos\n");
        exit(2);
    }

    // Crai o gerador.log
    FILE *gerador_log;
    gerador_log = fopen("gerador.log", "w");
    fprintf(gerador_log, "t(ticks) ; id_viat ; destin ; t_estacion ; t_vida  ; observ \n" );
    fclose(gerador_log);

    //Sigaction______________________________________
    struct sigaction action;
    action.sa_handler = alarm_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(SIGALRM,&action, NULL) < 0)
    {
        fprintf(stderr,"Falha ao instalar SIGALRM handler\n");
        exit(3);
    }
    alarm(tempo_gerador);
    //_____________________________________________


    int idCarro = 0;
    int n_ale;
    while(fechar_gerador != TRUE)
    {
		idCarro++;

		n_ale = rand() % 10;

		//Espera para gerar o proximo veiculo
		if ((n_ale > 1) && (n_ale < 5)) //30%
			wait_ticks(tempo_U_relogio);
		else if (n_ale < 2) // 20%
			wait_ticks(tempo_U_relogio * 2);

		//gera veiculo
		veiculo *veic = malloc(sizeof(veiculo));
		*veic = criar_veiculo(idCarro, tempo_U_relogio);

		//novo thread do veiculo
		pthread_t thread_veiculo;
		pthread_create(&thread_veiculo, NULL, veiculo_thread, (void *)veic);

    }
    pthread_exit(0);
}
