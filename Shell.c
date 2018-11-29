#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#define SIZE 64

void Launch(char **name, int in, int out)//
{
	if (fork() == 0)//создаём процесс
	{
		if (in != 0)
		{
			dup2(in, 0);
			close(in);
		}
		if (out != 1)
		{
			dup2(out, 1);
			close(out);
		}
		execvp(name[0], name);
		perror("Error.Invalid command");
		exit(1);
	}
}

int main()
{
	printf("Командная строка\n");
	while (1)
	{
		char readstr[SIZE], *transOutput = NULL, *transInput = NULL, *invisMode = NULL, *varError = NULL;
		char flag = 1, *args[8], *str = NULL, *endInputFile = NULL, *endOutputFile = NULL;
		int inputfile = 0, outputfile = 1, outputf = 0, fd[2];
		int avtiveProcess = 0, k, i = 0;
		int openflag = O_TRUNC;
		//начало
		printf(":>>");
		fflush(stdout);
		k = read(0, readstr, SIZE);//читаем строку
		readstr[k - 1] = '\0';
		if ((invisMode = strchr(readstr, '&')) != NULL)
		{
			varError = invisMode;
			while (*varError++ == ' ');
			if (*varError != '\0') { printf("Invalid arguments! Please try again0.\n"); continue; }
			if (!fork())
				if (!fork())
					setpgid(0, 0);//теперь вся команда выполняется в фоновом режиме
				else exit(1);
			else
			{
				wait(NULL);
				continue;
			}
		}
		if (invisMode) *invisMode = '\0';
		if ((transInput = strchr(readstr, '<')) != NULL)
		{
			char *str = transInput + 1;
			while (*str == ' ')
				str++;
			while (*str != ' ' && *str != '>' && *str != '\0')
				str++;
			varError = str;
			while (*varError != '<' && *varError != '\0')
				varError++;
			if (*varError != '\0') { printf("Invalid arguments! Please try again1.\n"); continue; }
			endInputFile = str;
			*endInputFile = '\0';
			if ((inputfile = open(transInput + 1, O_RDONLY)) == -1) { perror("Error.File.Please try again.\n"); exit(2); }
		}
		if ((transOutput = strchr(readstr, '>')) != NULL)
		{
			char *str = transOutput + 1;
			if (*str == '>')
			{//если '>>' то дозаписываем файл
				str++;
				openflag = O_APPEND;
			}
			while (*str == ' ')
				str++;
			while (*str != ' ' && *str != '<' && *str != '\0')
				str++;
			varError = str;
			while (*varError != '>' && *varError != '\0')
				varError++;
			if (*varError != '\0') { printf("Invalid arguments! Please try again2.\n"); continue; }
			endOutputFile = str;
			*endOutputFile = '\0';
			if ((outputf = open(transOutput + 1, O_WRONLY | openflag | O_CREAT, 0666)) == -1) { perror("Error.File.Please try again.\n"); exit(2); }
		}
		if (transOutput)// отделение фонового режима, входного и выходного файла
			*transOutput = '\0';
		if (transInput)
			*transInput = '\0';
		if (invisMode && !inputfile) inputfile = ("/dev/null", O_RDONLY);
		str = readstr;//возвращаемся в начало
		flag = 1;
		while (flag != '\0' && *str != '\0')
		{
			while (*str == ' ') str++;//пропускаем пробелы в начале
			args[i++] = str;
			while (*str == ' ') str++;//пропуск пробелов
			while (*str != ' ' && *str != '|' && *str != '\0') str++;
			flag = *str;
			*str = '\0';
			if (flag != '\0') str++;
			while (*str == ' ') str++;//пропуск пробелов
			if (*str == '|' || *str == '\0' || flag == '|' || flag == '\0')
			{
				if (*str == '|' || flag == '|')
				{// делаем пайп,если есть конвеер
					pipe(fd);
					outputfile = fd[1];
				}
				else
					if (outputf) outputfile = outputf;
				if (invisMode)
					if (outputfile == 1) outputfile = open("/dev/null", O_WRONLY);
				args[i] = NULL;
				Launch(args, inputfile, outputfile);//запуск команды
				if (inputfile != 0) close(inputfile);
				if (outputfile != 1) close(outputfile);
				if (*str == '|')
				{
					str++; //если есть еще команда, то переходим
					inputfile = fd[0];
				}
				if (flag == '|') inputfile = fd[0];
				avtiveProcess++;//avtiveProcess - количество запущенных процессов
				while (*str == ' ') str++;
				i = 0;
			}
		}
		for (i = 1; i <= avtiveProcess; i++)  wait(NULL);
		if (invisMode) exit(0);
	}
	return 0;
}
