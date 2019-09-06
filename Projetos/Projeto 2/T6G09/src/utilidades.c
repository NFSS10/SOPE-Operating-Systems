/*
 * utilidades.c
 *
 *  Created on: 20/05/2016
 *      Author: nuno
 */
#include <sys/types.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>


#define TRUE 1
#define FALSE 0
#define NUM_THREADS_CONTROLADOR     4



typedef struct
{
	int id; //id unico do veiculo
	char direcao;//(N,S,E ou O) __ prob 1/4
	clock_t tempo_estacionamento; //(entre 1 e 10) * U_RELOGIO __ prob 1/10
} veiculo;


void wait_ticks(clock_t tempo_espera)
{
		clock_t tempo=0;
		clock_t tempo_inicial;
		clock_t tempo_agr;
		tempo_inicial=clock();

		while(tempo<tempo_espera)
		{
			tempo_agr=clock();
			tempo= tempo_agr-tempo_inicial;
		}
}

veiculo criar_veiculo(int id, int tempo_U_relogio)
{
	veiculo v;
	int n_ale;

	v.id=id;

	n_ale=rand()%4;
	switch (n_ale)
	{
	case 0:
		v.direcao = 'N';
		break;
	case 1:
		v.direcao = 'S';
		break;
	case 2:
		v.direcao= 'E';
		break;
	case 3:
		v.direcao = 'O';
		break;
	default:
		break;
	}

	n_ale=(rand()%10)+1;
	v.tempo_estacionamento=n_ale*tempo_U_relogio;

	return v;
}


void parquelog_reg(int id, int lugares_disp, char str[])
{
    FILE *fic;
    clock_t ticks = clock();

    fic = fopen("parque.log", "a");
    fprintf(fic,"%7d  ; %4d ; %5d   ; %s\n", (int)ticks, lugares_disp, id, str);

    fclose(fic);
    return;
}


void geradorlog_reg(veiculo *veic, char str[], clock_t ticks, int in)
{
     FILE *fic = fopen("gerador.log", "a");
	clock_t tempo_agr = clock();

	//_______________________________________
	if (in == TRUE)
		fprintf(fic, "%9d;%9d;%8c;%12d;    ?    ; %5s\n", (int) ticks, veic->id,
				veic->direcao, (int) veic->tempo_estacionamento, str);
	else
		fprintf(fic, "%9d;%9d;%8c;%12d;%9d;%5s\n", (int) tempo_agr, veic->id,
				veic->direcao, (int) veic->tempo_estacionamento, (int) ticks,
				str);
	//_____________________________
	fclose(fic);
	return;
}


