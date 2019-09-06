#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

#define TAM_INFO 76
#define TAM_CMP 55

//Remove o \n do fim do caminho
void formatar_caminho(char* caminho)
{
	int i;
	for (i = 0;; i++)
	{
		if (caminho[i] == '\n')
		{
			caminho[i] = 0;
			return;
		}
		if (caminho[i] == 0)
			return;
	}
}

//0 = iguais / 1= diferentes
int compare_two_binary_files(FILE *fp1, FILE *fp2) {

    char ch1, ch2;

    int flag = 0;



    while (((ch1 = fgetc(fp1)) != EOF) && ((ch2 = fgetc(fp2)) != EOF))
    {
		if (ch1 == ch2)
		{
			flag = 1;
			continue;
		}
		else
		{
			fseek(fp1, -1, SEEK_CUR);
			flag = 0;
			break;
		}

	}

	if (flag == 0)
		return 1;

	return 0;
}


int main(int argc, char *argv[])
{
	int stat;

	int ficheiro;
	int ficheiro_aux;
	char caminho_prog[256];
	char hlinks[256];
	int hl;
	pid_t pid;


	//Verifica se esta a ser usado corretamnete
	if (argc != 2)
	{
		fprintf( stderr, "Usage: %s <dir>\n", argv[0]);
		return 1;
	}



	//Cria o filestemporario.txt
	getcwd(caminho_prog, sizeof(caminho_prog));
	strcat(caminho_prog, "/filestemporario.txt");
	ficheiro_aux = open(caminho_prog, O_RDWR | O_CREAT | O_TRUNC, 0600);
	close(ficheiro_aux);

	//Executa o lsdir
	pid = fork();
	if (pid == 0)
		execl("lsdir", "lsdir", argv[1], NULL);

	wait(&stat);



	//Cria o files.txt para meter os ficheiros ordenados
	getcwd(caminho_prog, sizeof(caminho_prog));
	strcat(caminho_prog, "/files.txt");
	ficheiro = open(caminho_prog, O_RDWR | O_CREAT | O_TRUNC, 0600);

	//Abre o files.txt auxiliar
	getcwd(caminho_prog, sizeof(caminho_prog));
	strcat(caminho_prog, "/filestemporario.txt");
	ficheiro_aux=open(caminho_prog, O_RDONLY);

	//Ordenar o files.txt
	pid=fork();
	if(pid==0)
	{
		dup2(ficheiro_aux, STDIN_FILENO);
		dup2(ficheiro,STDOUT_FILENO);
		execlp("sort", "sort", NULL);
	}
	wait(&stat);
	close(ficheiro_aux);
	close(ficheiro);
	//______FICHEIRO ORDENADO_________________________

	//Apaga o files.txt auxiliar
	getcwd(caminho_prog, sizeof(caminho_prog));
	strcat(caminho_prog, "/filestemporario.txt");
	remove(caminho_prog);


	//____________Tratar dos duplicados__________________________________________

	//Cria o hlinks.txt
	strcpy(hlinks,argv[1]);
	strcat(hlinks, "/hlinks.txt");
	hl = open(hlinks, O_RDWR | O_CREAT | O_TRUNC, 0600);
	close(hl);


	char ficheiro1[TAM_CMP]; //info do ficheiro
	char linha1[512]; //info do ficheiro + caminho
	char ficheiro2[TAM_CMP];
	char linha2[512];
	char* linha;
	size_t tam=0;
	FILE* f;
	int estado=0;
	char caminho1[256]; //So caminho do ficheiro
	char caminho2[256];

	FILE *fp1, *fp2;

	//Abre o files.txt ordenado
	getcwd(caminho_prog, sizeof(caminho_prog));
	strcat(caminho_prog, "/files.txt");
	f = fopen(caminho_prog, "r");



	if (getline(&linha, &tam, f) == -1)
	{
		perror("getline primeira linha");
		return 2;
	}
	strcpy(linha1, linha);
	strncpy(ficheiro1, linha1, sizeof(ficheiro1));
	ficheiro1[sizeof(ficheiro1) - 1] = 0;

	//Percorre o files.txt
	while (estado != -1)
	{
		estado = getline(&linha, &tam, f);
		if (estado == -1)
			return 0;

		strcpy(linha2, linha);
		strncpy(ficheiro2, linha2, sizeof(ficheiro2));
		ficheiro2[sizeof(ficheiro2) - 1] = 0;

		//Sao iguais
		if (strcmp(ficheiro1, ficheiro2) == 0)
		{
			strncpy(caminho1, linha1 + TAM_INFO, sizeof(linha1) - TAM_INFO);
			strncpy(caminho2, linha2 + TAM_INFO, sizeof(linha2) - TAM_INFO);
			formatar_caminho(caminho1);
			formatar_caminho(caminho2);


			//Compara conteudo
			fp1 = fopen(caminho1,  "r");
			 if (fp1 == NULL)
				 return 3;
			fp2 = fopen(caminho2,  "r");
			 if (fp2 == NULL)
				 return 4;

			if (compare_two_binary_files(fp1, fp2) == 0) //Se sao iguais entao = 0
			{
				if (unlink(caminho2) != 0)
				{
					perror("unlink");
					return 4;
				}
				if (link(caminho1, caminho2) != 0)
				{
					perror("link");
					return 5;
				}

				//Escreve no hlinks.txt
				hl = open(hlinks, O_RDWR | O_CREAT | O_APPEND, 0600);
				write(hl, caminho1, strlen(caminho1));
				write(hl, "\n", 1);
				write(hl, caminho2, strlen(caminho2));
				write(hl, "\n\n", 2);

				close(hl);
			}
		}

		strcpy(ficheiro1, ficheiro2);
		strcpy(linha1, linha2);
	}

	close(ficheiro);

	return 0;
}
