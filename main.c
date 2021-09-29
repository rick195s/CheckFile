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

//#include "memory.h"

// Validate the file extension with the actual file type
int mimeValidation(char *mimeType){


		
	printf("%s\n", mimeType);
	return 0;
}

int mimeParsing(char *mime){
	mimeValidation(mime);

	return 0;
}


// Validate file permissions, existance, etc
int fileValidation(char *filePath){

	FILE *file = fopen("out.txt", "w+");
	
	if (file == NULL)
		 ERROR(2,"Não foi possível criar o ficheiro \n\t'%s'", filePath);

	// Redirecting the output to out.txt
	dup2(fileno(file), STDOUT_FILENO);


	pid_t id = fork();

	if (id == 0)
	{
		execlp("file","file","--mime-type",filePath,NULL);
	}else
		wait(NULL);

	dup2(STDOUT_FILENO, fileno(file));


	fclose(file);
	return 0;	
}

// Validate dir permissions, existance, etc
int dirValidation(){

	return 0;
}

// Validate if file have file paths in it
int batchValidation(char *batchPath){
	fileValidation(batchPath);

	return 0;
}


int main(int argc, char *argv[]) {

    struct gengetopt_args_info args;

	// gengetopt parser: deve ser a primeira linha de código no main
	if(cmdline_parser(argc, argv, &args))
		ERROR(1, "Erro: execução de cmdline_parser\n");

	if(args.files_given > 0)
		for (size_t i = 0; i < args.files_given; i++)
		{
			fileValidation(args.files_arg[i]);
		}
		
	if(args.dir_given > 0)
		dirValidation(args.dir_arg);
	
	if(args.batch_given > 0)
		batchValidation(args.batch_arg);
	
	// gengetopt: libertar recurso (assim que possível)
	cmdline_parser_free(&args);

    return 0;
}
