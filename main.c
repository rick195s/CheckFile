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

#define MAX_EXT_SIZE 30

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

char *getFileExtension(char *fileExtension, char *filePath)
{

	fileExtension = MALLOC(MAX_EXT_SIZE);

	strcpy(fileExtension, strrchr(filePath, '.') + 1);

	return fileExtension;
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

	detectedExtension = MALLOC(MAX_EXT_SIZE);

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
		return -2;

	return -1;
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

	strcpy(mimeType, strrchr(mimeType, ':') + 2);

	fclose(outputFile);

	return mimeType;
}

int fileProcessing(char *filePath)
{
	FILE *file = openFile(filePath);

	if (file == NULL)
		return -1;

	char *mimeType = NULL;
	char *fileExtension = NULL;
	char *detectedExtension = NULL;

	mimeType = mimeParsing(mimeType, filePath);

	fileExtension = getFileExtension(fileExtension, filePath);

	// Show output information
	switch (mimeValidation(mimeType, fileExtension, detectedExtension))
	{
	case 0:
		printf("[OK] '%s': extension '%s' matches file type '%s'\n", filePath, fileExtension, detectedExtension);
		break;

	case -1:
		printf("[MISMATCH] '%s': extension is '%s', file type is '%s'\n", filePath, fileExtension, detectedExtension);
		break;

	case -2:
		printf("[INFO] '%s': type '%s' is not supported by checkFile\n", filePath, mimeType);
		break;

	default:
		break;
	}

	fclose(file);
	FREE(detectedExtension);
	FREE(mimeType);
	FREE(fileExtension);

	return 0;
}

// Validate dir permissions, existance, etc
int dirProcessing(char *dirPath)
{
	DIR *dir = opendir(dirPath);
	struct dirent *dir_entry;
	int finito = 0;
	char *fullPath = NULL;

	// Check last char of dirPath
	// Using values of ASCII table to avoid calling strcomp because
	// I'm just comparing char
	if (dirPath[strlen(dirPath) - 1] != (int)'/')
	{
		strcat(dirPath, "/");
	}

	if (dir == NULL)
	{
		fprintf(stderr, "ERROR: cannot open dir '%s' -- %s\n", dirPath, strerror(errno));
		return -1;
	}

	while (finito == 0)
	{
		dir_entry = readdir(dir);
		if (dir_entry == NULL)
		{
			if (errno)
			{
				closedir(dir);
				return -1;
			}
			else
				finito = 1;
		}
		else
		{
			fullPath = MALLOC(strlen(dirPath) + strlen(dir_entry->d_name));
			if (fullPath == NULL)
			{
				fprintf(stderr, "ERROR: cannot allocate memory %s\n", strerror(errno));
				return -1;
			}

			strcat(fullPath, dirPath);
			strcat(fullPath, dir_entry->d_name);
			fileProcessing(fullPath);
		}
	}

	if (closedir(dir) == -1)
	{
		fprintf(stderr, "ERROR: cannot close dir '%s' -- %s\n",
				dirPath, strerror(errno));
		return -1;
	}

	FREE(fullPath);
	return 0;
}

// Validate if file have file paths in it
int batchProcessing(char *batchPath)
{
	FILE *file = openFile(batchPath);

	if (file == NULL)
		return -1;

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
		dirProcessing(args.dir_arg);

	if (args.batch_given > 0)
		batchProcessing(args.batch_arg);

	// gengetopt: libertar recurso (assim que possível)
	cmdline_parser_free(&args);

	return 0;
}
