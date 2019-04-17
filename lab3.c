/*
	.-----------------------------------------------------------------------------------------------------------------.
	|ЛАБОРАТОРНАЯ РАБОТА №3																							  |
	|ВАРИАНТ №6																										  |
	|-----------------------------------------------------------------------------------------------------------------|
	|Написать программу подсчета частоты встречающихся символов в файлах заданного каталога и его подкаталогов.		  |
	|Пользователь задает имя каталога. Главный процесс открывает каталоги и запускает для каждого файла каталога	  |
	|отдельный процесс подсчета количества символов. Каждый процесс выводит на экран свой pid, полный путь к файлу,	  |
	|общее число просмотренных байт и частоты встречающихся символов(все в одной строке). Число одновременно		  |
	|работающих процессов не должно превышать N(вводится пользователем). Проверить работу программы для каталога /etc.|
	`-----------------------------------------------------------------------------------------------------------------`
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>

const char *errFileName = "err.log";
char *GetDirPath(const char *path);
char *GetSymbolArr(const int count, char *arr[], int *length);
void CreateErrFile();
void ParseDir(const char *dirPath, const int N, int *pcount, const char *exeName, const char *symbolArr, const int symbolArrLen);
void WriteErr(const char *exeName, const char *dirPath, const int pid);
char *GetFullPath(const char *dirPath, const char *fileName);
void DisplayErr();

int main(int argc, char *argv[], char *envp[])
{
	char *dirPath = GetDirPath(argv[1]);
	const int N = atoi(argv[2]) - 1;
	int symbolArrLength = 0;
	char *symbolArr = GetSymbolArr(argc, argv, &symbolArrLength);
	CreateErrFile();
	int pcount = 0;
	ParseDir(dirPath, N, &pcount, basename(argv[0]), symbolArr, symbolArrLength);

	while (pcount > 0)
	{
		int status;
		waitpid(-1, &status, WUNTRACED);
		pcount--;
	}

	DisplayErr();
}

char *GetDirPath(const char *path)
{
	char *dirPath;
	if (path[0] == '/')
	{
		dirPath = (char *)malloc(strlen(path) + 1);
		strcpy(dirPath, path);
	}
	else
	{
		char cwd[PATH_MAX];
		getcwd(cwd, sizeof(cwd));
		dirPath = (char *)malloc(strlen(cwd) + 1 + strlen(path) + 1);
		strcpy(dirPath, cwd);
		strcat(dirPath, "/");
		strcat(dirPath, path);
	}
	return dirPath;
}

char *GetSymbolArr(const int count, char *arr[], int *length)
{
	char *symbolArr = (char *)calloc(0, sizeof(char));
	for (int i = 3; i < count; i++)
	{
		symbolArr = (char *)realloc(symbolArr, (*length + 1) * sizeof(char) + 1);
		symbolArr[*length] = arr[i][0];
		*length += 1;
	}
	return symbolArr;
}

void CreateErrFile()
{
	FILE *F = fopen(errFileName, "w");
	fclose(F);
}

void WriteErr(const char *exeName, const char *dirPath, const int pid)
{
	char *err = malloc(sizeof(int) + strlen(exeName) + strlen(dirPath) + 3);
	char *errpid = (char *)malloc(sizeof(int));
	sprintf(errpid, "%d", pid);
	strcpy(err, errpid);
	free(errpid);
	strcat(err, " ");
	strcat(err, exeName);
	strcat(err, " ");
	strcat(err, dirPath);
	FILE *errfile;
	errfile = freopen(errFileName, "a", stderr);
	perror(err);
	fclose(errfile);
	free(err);
}

char *GetFullPath(const char *dirPath, const char *fileName)
{
	char *fullPath = malloc(strlen(dirPath) + strlen(fileName) + 2);
	strcpy(fullPath, dirPath);
	strcat(fullPath, "/");
	strcat(fullPath, fileName);
	return fullPath;
}

void DisplayErr()
{
	char *errstr = (char *)calloc(0, sizeof(char));
	int i = 0;
	FILE *errfile = fopen(errFileName, "r");
	while (!feof(errfile))
	{
		char c = fgetc(errfile);
		errstr = (char *)realloc(errstr, (i + 1) * sizeof(char) + 1);
		errstr[i] = c;
		i++;
	}
	fclose(errfile);
	printf("%s", errstr);
	free(errstr);
}

void ParseDir(const char *dirPath, const int N, int *pcount, const char *exeName, const char *symbolArr, const int symbolArrLen)
{
	DIR *dir = opendir(dirPath);
	struct stat st;
	lstat(dirPath, &st);
	if ((S_ISDIR(st.st_mode)) && (dir == NULL))
	{
		WriteErr(exeName, dirPath, getpid());
		closedir(dir);
		return;
	}
	else if (!S_ISDIR(st.st_mode))
	{
		closedir(dir);
		return;
	}

	struct dirent *de;
	pid_t pid;
	while (de = readdir(dir))
	{
		char *fullPath = GetFullPath(dirPath, de->d_name);
		struct stat buf;
		stat(fullPath, &buf);
		if ((S_ISDIR(buf.st_mode)) && (strcmp(de->d_name, ".")) && (strcmp(de->d_name, "..")))
			ParseDir(fullPath, N, &*pcount, exeName, symbolArr, symbolArrLen);
		else if (!S_ISDIR(buf.st_mode))
		{
			if (*pcount == N)
			{
				int status;
			 	waitpid(-1, &status, WUNTRACED);
			 	*pcount -= 1;
			}
			if (*pcount < N)
			{
				pid = fork();
				*pcount += 1;
			}
			if (pid == 0)
			{
				int byteCount = 0;
				char *countStr = (char *)calloc(0, sizeof(int));
				for (int i = 0; i < symbolArrLen; i++)
				{
					int count = 0;
					FILE *F = fopen(fullPath, "r");
					while (!feof(F))
					{
						byteCount++;
						char c = fgetc(F);
						if (c == symbolArr[i])
							count++;
					}
					fclose(F);
					countStr = (char *)realloc(countStr, (i + 1) * (sizeof(int) + sizeof(char) + 1));
					char *temp = (char *)malloc(sizeof(int));
					sprintf(temp, "%d", count);
					strcat(countStr, temp);
					strcat(countStr, " ");
					free(temp);
				}
				byteCount = byteCount / 2 - 1;
				printf("%d %s %d %s\n", getpid(), fullPath, byteCount, countStr);
				free(countStr);

				char *killstr = (char *)calloc(8, sizeof(char));
				strcpy(killstr, "kill -9 ");
				killstr = (char *)realloc(killstr, strlen(killstr) + sizeof(int) + 1);
				char *temp = (char *)malloc(sizeof(int));
				sprintf(temp, "%d", getpid());
				strcat(killstr, temp);
				system(killstr);
				*pcount -= 1;
				free(temp);
				free(killstr);
			}
		}
		free(fullPath);
	}
	closedir(dir);
}