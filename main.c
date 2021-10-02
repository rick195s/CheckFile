/**
 * @file main.c
 * @brief Description
 * @date 2018-1-1
 * @author name of author
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "args.h"
#include "debug.h"

char *types[2] = {"text/plain", "text/plain"};
//#include "memory.h"

void getFileExtension(char* filePath){
	printf("Extensao: %s\n", strrchr(filePath, '.')+1);

}

// Validate the file extension with the actual file type
int mimeValidation(char *mimeType)
{

	printf("Mime: %s\n", mimeType);
	mimeType = strrchr(mimeType, ':');
	printf("O mime type: %s\n", mimeType + 2);

	return 0;
}

int mimeParsing(FILE *fd, char *mime)
{
	// saves the file size in bytes
	int fileSize = lseek(fileno(fd), 0L, SEEK_END);

	lseek(fileno(fd), 0L, SEEK_SET);

	mime = realloc(mime,fileSize * sizeof(char));

	// gets the fist line
	fgets(mime, fileSize, fd);

	return 0;
}

int fileProcessing(char *filePath)
{

	FILE *file = fopen("out.txt", "w+");
	char *mime = NULL;

	mime = realloc(mime,0);

	pid_t id = fork();

	if (id == 0)
	{
		// Redirecting the output to out.txt
		dup2(fileno(file), STDOUT_FILENO);
		execlp("file", "file", "--mime-type", filePath, NULL);
	}
	else
		wait(NULL);

	mimeParsing(file, mime);

	if(mime != NULL)
		printf("Mime: %s\n", mime);

	fclose(file);

	getFileExtension(filePath);

	free(mime);
	
	return 0;
}

// Validate file permissions, existance, etc
int fileValidation(char *filePath)
{
	FILE *file = fopen(filePath, "r"); 

	if (file == NULL)
		ERROR(2, "Não foi possível abrir o ficheiro \n\t'%s'", filePath);

	fclose(file);

	fileProcessing(filePath);

	return 0;
}

// Validate dir permissions, existance, etc
int dirValidation()
{

	return 0;
}

// Validate if file have file paths in it
int batchValidation(char *batchPath)
{
	fileValidation(batchPath);

	return 0;
}


int main(int argc, char *argv[])
{

	struct gengetopt_args_info args;

	// gengetopt parser: deve ser a primeira linha de código no main
	if (cmdline_parser(argc, argv, &args))
		ERROR(1, "Erro: execução de cmdline_parser\n");

	if (args.files_given > 0)
		for (size_t i = 0; i < args.files_given; i++)
		{
			fileValidation(args.files_arg[i]);
		}


	if (args.dir_given > 0)
		dirValidation(args.dir_arg);

	if (args.batch_given > 0)
		batchValidation(args.batch_arg);

	// gengetopt: libertar recurso (assim que possível)
	cmdline_parser_free(&args);

	return 0;
}
