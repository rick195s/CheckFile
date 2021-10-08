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
#include <dirent.h>
#include "args.h"
#include "debug.h"
#include "memory.h"

// Validate file permissions, existance, etc
FILE *openFile(char *filePath)
{
	FILE *file = fopen(filePath, "r");

	// Using exit will stop the program and not verify the other files.
	// So I decided just to show the warning message and then return NULL or the file

	if (file == NULL)
		fprintf(stderr, "ERROR: cannot open file '%s' -- %s\n", filePath, strerror(errno));

	return file;
}

char *getFileExtension(char *filePath)
{
	return strrchr(filePath, '.') + 1;
}

void extractFileType(FILE *outputFile, char *filePath)
{
	// Creates child process
	pid_t id = fork();

	if (id == 0)
	{
		// Redirecting the output to out.txt
		dup2(fileno(outputFile), STDOUT_FILENO);
		execlp("file", "file", "--mime-type", filePath, NULL);
	}
	else
		// Parent waits for Child
		wait(NULL);
}

// Validate the file extension with the actual file type
int mimeValidation(char *mimeType, char *fileExtension, char *detectedExtension)
{
	// define the supported mime types and extensions
	char *types[3] = {"application/pdf", "image/png", "image/jpeg"};
	char *extensions[3] = {"pdf", "png", "jpeg/jpg/jpe/jfif"};

	strcpy(detectedExtension, "");

	for (size_t i = 0; i < 3; i++)
		// Check if mimeType is supported by checkFile
		if (!strcmp(types[i], mimeType))
		{
			strcpy(detectedExtension, extensions[i]);
			if (strstr(extensions[i], fileExtension) != NULL)
				return 0;
		}

	// if detectedExtension is empty then the mime type isn't supported
	if (!strcmp(detectedExtension, ""))
		return 2;

	return 1;
}

char *mimeParsing(char *mimeType, char *filePath)
{
	FILE *outputFile = fopen("out.txt", "w+");

	extractFileType(outputFile, filePath);

	// saves the file size in bytes
	size_t fileSize = lseek(fileno(outputFile), 0, SEEK_END);

	lseek(fileno(outputFile), 0, SEEK_SET);

	// sizeof(char) = 1
	//mimeType = realloc(mimeType, fileSize);
	mimeType = MALLOC(fileSize);

	if (mimeType == NULL)
		ERROR(2, "Not hable to request memory");

	// gets the fist line
	fgets(mimeType, fileSize, outputFile);

	// finds the last occurence of ':' and adds 2
	// so mimeType wont have ': '
	mimeType = strrchr(mimeType, ':') + 2;

	fclose(outputFile);

	return mimeType;
}

int fileProcessing(char *filePath)
{
	FILE *file = openFile(filePath);

	if (file == NULL)
		return 1;

	char *mimeType = NULL;
	char *fileExtension = NULL;
	char *detectedExtension = MALLOC(100);

	mimeType = mimeParsing(mimeType, filePath);

	fileExtension = getFileExtension(filePath);

	// Show output information
	switch (mimeValidation(mimeType, fileExtension, detectedExtension))
	{
	case 0:
		printf("[OK] '%s': extension '%s' matches file type '%s'\n", filePath, fileExtension, detectedExtension);
		break;

	case 1:
		printf("[MISMATCH] '%s': extension is '%s', file type is '%s'\n", filePath, fileExtension, detectedExtension);
		break;

	case 2:
		printf("[INFO] '%s': type '%s' is not supported by checkFile\n", filePath, mimeType);
		break;

	default:
		break;
	}

	fclose(file);
	FREE(detectedExtension);

	return 0;
}

// Validate dir permissions, existance, etc
int dirValidation(char *dirPath)
{
	
	opendir(dirPath);

	return 0;
}

// Validate if file have file paths in it
int batchValidation(char *batchPath)
{
	FILE *file = openFile(batchPath);

	if (file == NULL)
		return 1;

	char *fileToVal = MALLOC(100);

	// "ler" até ao fim do ficheiro
	while (!feof(file))
	{
		if (fgets(fileToVal, 100, file) != NULL)
		{
			// remove \n char from string
			fileToVal = strtok(fileToVal, "\n");
			fileProcessing(fileToVal);
		}
	}

	fclose(file);
	FREE(fileToVal);

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
			fileProcessing(args.files_arg[i]);

	if (args.dir_given > 0)
		dirValidation(args.dir_arg);

	if (args.batch_given > 0)
		batchValidation(args.batch_arg);

	// gengetopt: libertar recurso (assim que possível)
	cmdline_parser_free(&args);

	return 0;
}
