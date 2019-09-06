#include "utilidades.c"


static int N_Lugares;
static int tempo_abertura;

static int Lugares_Disponiveis;
static int fechar_parque = FALSE;

pthread_t controladores[NUM_THREADS_CONTROLADOR]; //4 controladores
sem_t *sem[NUM_THREADS_CONTROLADOR]; //semaforos
static int ind_sem = 0; //para selecionar o semaforo

pthread_mutex_t parque_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t controlador_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t condEmpty = PTHREAD_COND_INITIALIZER; //Nao sei usar muito bem isto...

//___________________________________________________

void alarm_handler(int sig)
{
    fechar_parque = TRUE;

    int i;
    for(i=0; i<NUM_THREADS_CONTROLADOR; i++)
        sem_post(sem[i]);
}



//Thread arrumador
void *arrumador_thread(void *arg)
{
    pthread_detach(pthread_self());

    veiculo *veic = arg;
    int fd_veiculo;

    char fifo_nome_unico[128];
    sprintf(fifo_nome_unico, "/tmp/FIFO%d", veic->id); //Cria o nome do FIFO do veiculo
    fd_veiculo = open(fifo_nome_unico, O_WRONLY);//Abre o FIFO do veiculo para escrever a resposta

    pthread_mutex_lock(&parque_mutex); //////////////////////////////

    if(fechar_parque) //Se o parque fechou
    {
        parquelog_reg(veic->id,Lugares_Disponiveis, "parquefechado");
        char resposta_fechado[64];
        sprintf(resposta_fechado, "%s", "parquefechado"); //////////
        write(fd_veiculo, resposta_fechado, sizeof(resposta_fechado)); //Escreve a mensagem "parquefechado"
    }
    else //Nao esta fechado
	{
        if(Lugares_Disponiveis <= 0) //Se esta cheio
        {
            parquelog_reg(veic->id,Lugares_Disponiveis, "cheio!"); //////////////////////////////
            char resposta_cheio[64];
            sprintf(resposta_cheio, "%s", "cheio!");
            write(fd_veiculo, resposta_cheio, sizeof(resposta_cheio));
        }
        else //Tem vaga, pode estacionar
        {
            Lugares_Disponiveis--;
            parquelog_reg(veic->id,Lugares_Disponiveis, "entrada");
            pthread_mutex_unlock(&parque_mutex); //////////////
            char resposta_entrada[64];
            sprintf(resposta_entrada, "%s", "entrada");
            write(fd_veiculo, resposta_entrada, sizeof(resposta_entrada));

            wait_ticks(veic->tempo_estacionamento); //Fica estacionado pelo tempo especificado

            Lugares_Disponiveis++; //O lugar fica livre
            pthread_mutex_lock(&parque_mutex); /////////
            //Lugares_Disponiveis++; //O lugar fica livre

            parquelog_reg(veic->id,Lugares_Disponiveis, "saida"); //parque.log < saiu
            pthread_mutex_unlock(&parque_mutex);/////////////////
            char resposta_saida[64];
            sprintf(resposta_saida, "%s", "saida");
            write(fd_veiculo, resposta_saida, sizeof(resposta_saida)); //Da a resposta que saiu
	   // close(fd_veiculo); talvez?????????????????????????????
            unlink(fifo_nome_unico);

            pthread_mutex_lock(&parque_mutex);

            if(fechar_parque == TRUE && Lugares_Disponiveis == N_Lugares) //Se ja esta fechado e vazio
            {
                pthread_cond_broadcast(&condEmpty);
            }
        }
    }
    pthread_mutex_unlock(&parque_mutex);
    close(fd_veiculo);
    free(veic);
    return NULL;
}


void *controlador_thread(void *arg)
{
    char dir_controlador =*(char *)arg;

    char fifo_nome_controlador[128];
    char sem_nome[64];
    sprintf(fifo_nome_controlador, "/tmp/FIFO%c", dir_controlador); // "/tmp/FIFOx onde x é N, S, E ou O
    sprintf(sem_nome, "%s%c", "/mysemaphore", dir_controlador); // "mysemaphorex onde x é N, S, E ou O

    int fd_control;
    mkfifo(fifo_nome_controlador, 0660); //Cria o FIFO controlador
    fd_control = open(fifo_nome_controlador, O_RDONLY | O_NONBLOCK); //Abre o FIFO controlador
    if(fd_control  == -1)
    {
       //TODO erro
    }

    pthread_mutex_lock(&controlador_mutex); //////////
    int a = ind_sem++; ///////////////TODO ver isto----------
    sem[a] = sem_open(sem_nome,O_CREAT,0660,0);
    pthread_mutex_unlock(&controlador_mutex);

	while (TRUE)
	{
		sem_wait(sem[a]); ///////////////////// ////////////////////////

		if (fechar_parque == TRUE)
		{
			sem_close(sem[a]); //Fechar o semaforo
			sem_unlink(sem_nome);  //Apagar o semaforo
		}
		//--------------Se chegar aqui n é para fechar--------------

		//// TODO VER ISTO
		 veiculo *veic = malloc(sizeof(veiculo));
		if (read(fd_control, veic, sizeof(veiculo)) > 0)
		{
			pthread_t thread_arrumador;
			pthread_create(&thread_arrumador, NULL, arrumador_thread, veic); ///////////////////// /////////////////////
		}
		else if (fechar_parque)
		{
			pthread_mutex_lock(&parque_mutex);

			while (Lugares_Disponiveis != N_Lugares) //Espera que fique vazio
				pthread_cond_wait(&condEmpty, &parque_mutex);

			pthread_mutex_unlock(&parque_mutex);

			//Fechar e limpar
			close(fd_control);
			unlink(fifo_nome_controlador);
			return NULL;
		}
	}

}

int main(int argc, char* argv[])
{
	/**
	 * argc2 = numero de lugares
	 * argc3 = tempo de abertura
	 */
	if (argc != 3) {
		printf("Usar: %s <n_lugares> <tempo_abertura>\n", argv[0]);
		exit(1);
	}

	N_Lugares = atoi(argv[1]);
	tempo_abertura=atoi(argv[2]);

	Lugares_Disponiveis = N_Lugares;

	if (N_Lugares < 0 || tempo_abertura < 0)
	{
		printf("\n Insira valores válidos \n");
		exit(2);
	}

    //Cria o parque.log
    FILE *parque_log;
    parque_log= fopen("parque.log", "w");
    fprintf(parque_log, "t(ticks) ; nlug ; id_viat ; observ \n");
    fclose(parque_log);

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
    alarm(tempo_abertura);
    //_________________________________________________

    char orientacoes[4]= {'N', 'S', 'E', 'O'};

	int i;
	for (i = 0; i < NUM_THREADS_CONTROLADOR; i++)
		pthread_create(&controladores[i], NULL, controlador_thread,&orientacoes[i]);


	for (i = 0; i < NUM_THREADS_CONTROLADOR; i++)
			pthread_join(controladores[i], NULL);

    pthread_exit(0);
}
