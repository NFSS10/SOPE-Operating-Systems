#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>

int main(int argc, char *argv[])
{
	int st;

	DIR *dir;
	struct dirent *dir_info;
	struct stat stat_info;
	char caminhodir[256];
	char *tipo;
	int ficheiro;
	char linha[256];
	char caminho_str[256];
	char date[20];
	char caminho_prog[256];


	//Verifica se esta a ser usado corretamnete
	if (argc != 2)
	{
		fprintf( stderr, "Usage: %s <dir>\n", argv[0]);
		return 1;
	}

	if ((dir = opendir(argv[1])) == NULL)
	{
		perror(argv[1]);
		return 2;
	}


	//files.txt na pasta do programa
	 getcwd(caminho_prog, sizeof(caminho_prog));
	 strcat(caminho_prog, "/filestemporario.txt");
	 ficheiro = open(caminho_prog,O_RDWR | O_CREAT | O_APPEND, 0600);


	 //Percorre o diretorio
	while ((dir_info = readdir(dir)) != NULL)
	{
		sprintf(caminhodir, "%s/%s", argv[1], dir_info->d_name);

		if (stat(caminhodir, &stat_info) == -1)
		{
			perror("ERRO stat()");
			return 3;
		}


		if (S_ISREG(stat_info.st_mode)) //Se for ficheiro
		{
			tipo = "regular";

			//Caminho ficheiro
			 getcwd(caminho_str, sizeof(caminho_str));
			 strcat(caminho_str, "/");
			 strcat(caminho_str, caminhodir);

			 //Guarda no files.txt
			 strftime(date, sizeof date, "%d-%m-20%y %H:%M:%S", localtime(&(stat_info.st_mtime)));
			 sprintf(linha, "%-25s   %6ld bytes   %10o  %-10s  %s\n",dir_info->d_name, stat_info.st_size, stat_info.st_mode, date, caminho_str);
			 write(ficheiro, linha, strlen(linha));

		}
		else if (S_ISDIR(stat_info.st_mode)) //Se for diretorio
		{
			if ((strcmp(dir_info->d_name, ".") != 0) && ((strcmp(dir_info->d_name, "..") != 0))) //Ignorar estes
			{
				tipo = "directorio";

				pid_t pid = fork();
				if (pid == 0)
					execl("lsdir", "lsdir", caminhodir, NULL);
				else if (pid > 0)
					wait(&st);
			}
		}


		printf("%-25s - %s\n", dir_info->d_name, tipo);
	}
	closedir(dir);
	close(ficheiro);
	printf("_______________\n");

	return 0;
}
