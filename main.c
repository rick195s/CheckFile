/**
 * @file main.c
 * @brief Description
 * @date 2021-10-5
 * @author Ricardo dos Santos Franco 2202314
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
#define MAX_FILENAME_SIZE 100

int getFileExtension(char *fileExtension, const char *filePath);
int extractFileType(FILE *outputFile, const char *filePath);
int mimeValidation(const char *mimeType, const char *fileExtension, char *detectedExtension);
char *mimeParsing(char *mimeType, const char *filePath);
int fileProcessing(const char *filePath);
int dirProcessing(char *dirPath);
int batchProcessing(const char *batchPath);

int getFileExtension(char *fileExtension, const char *filePath)
{
	char *ptr;

	// returns ptr if found '.' or NULL if not
	if ((ptr = strrchr(filePath, '.')) == NULL)
		return -1;

	// I want was after the '.' character so +1
	strcpy(fileExtension, ptr + 1);
	return 0;
}

int extractFileType(FILE *outputFile, const char *filePath)
{

	// Creates child process
	pid_t id = fork();

	if (id == 0)
	{
		// Redirecting the child output to out.txt
		dup2(fileno(outputFile), STDOUT_FILENO);
		execlp("file", "file", "--mime-type", filePath, NULL);
	}
	else
		// Parent waits for Child
		wait(NULL);

	return 0;
}

// Validate the file extension with the actual file type
int mimeValidation(const char *mimeType, const char *fileExtension, char *detectedExtension)
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
		return -2;

	return -1;
}

char *mimeParsing(char *mimeType, const char *filePath)
{
	FILE *outputFile = fopen("out.txt", "w+");

	if (outputFile == NULL)
		return NULL;

	if (extractFileType(outputFile, filePath) < 0)
		return NULL;

	// saves the file size in bytes
	size_t fileSize = lseek(fileno(outputFile), 0, SEEK_END);

	lseek(fileno(outputFile), 0, SEEK_SET);

	// sizeof(char) = 1
	mimeType = MALLOC(fileSize * sizeof(char));

	if (mimeType == NULL)
	{
		fclose(outputFile);
		fprintf(stderr, "[ERROR]: not hable to allocate memory\n");
		exit(2);
	}

	// gets the fist line
	fgets(mimeType, (int)fileSize, outputFile);

	// finds the last occurence of ':' and adds 2
	// so mimeType wont have ': '
	strcpy(mimeType, strrchr(mimeType, ':') + 2);
	fclose(outputFile);

	return mimeType;
}

int fileProcessing(const char *filePath)
{
	// Checking file permissions
	int asPerm = access(filePath, R_OK);
	if (asPerm != 0)
	{
		fprintf(stderr, "[ERROR]: cannot open file '%s' -- %s\n", filePath, strerror(errno));
		return -1;
	}

	char *mimeType = NULL;
	char *fileExtension = MALLOC(MAX_EXT_SIZE);
	char *detectedExtension = MALLOC(MAX_EXT_SIZE);

	mimeType = mimeParsing(mimeType, filePath);

	if (mimeType == NULL)
		return -1;

	if (getFileExtension(fileExtension, filePath))
		return -1;

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

	FREE(detectedExtension);
	FREE(fileExtension);
	FREE(mimeType);

	return 0;
}

// Validate dir permissions, existance, etc
int dirProcessing(char *dirPath)
{

	DIR *dir = opendir(dirPath);
	struct dirent *dir_entry;
	int finito = 0;
	char *fullPath = NULL;

	if (dir == NULL)
	{
		fprintf(stderr, "[ERROR]: cannot open dir '%s' -- %s\n", dirPath, strerror(errno));
		return -1;
	}

	// Checking last char of dirPath
	// Using values of ASCII table to avoid calling strcomp because
	// I'm just comparing char
	if (dirPath[strlen(dirPath) - 1] != (int)'/')
	{
		// Adding space for '/' to the string
		dirPath = realloc(dirPath, 1);
		if (dirPath == NULL)
		{
			closedir(dir);
			fprintf(stderr, "[ERROR]: ");
		}

		strcat(dirPath, "/");
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
			// +1 for the terminator '\0'
			fullPath = MALLOC(strlen(dirPath) + strlen(dir_entry->d_name) + 1);
			if (fullPath == NULL)
			{
				fprintf(stderr, "[ERROR]: cannot allocate memory\n");
				return -1;
			}

			// Initializing fullPath
			strcpy(fullPath, "");
			strcat(fullPath, dirPath);
			strcat(fullPath, (dir_entry->d_name));
			fileProcessing(fullPath);
			FREE(fullPath);
		}
	}

	if (closedir(dir) == -1)
	{
		fprintf(stderr, "[ERROR]: cannot close dir '%s' -- %s\n",
				dirPath, strerror(errno));

		return -1;
	}

	return 0;
}

// Validate if file have file paths in it
int batchProcessing(const char *batchPath)
{

	FILE *file = fopen(batchPath, "r");

	if (file == NULL)
	{
		fprintf(stderr, "[ERROR]: cannot open file '%s' -- %s\n", batchPath, strerror(errno));
		return -1;
	}

	char *fileToVal = MALLOC(MAX_FILENAME_SIZE);

	// Read from file until EOF
	while (!feof(file))
	{
		if (ferror(file))
		{
			fprintf(stderr, "[ERROR]: not hable to read from file\n");
			return -1;
		}

		if (fgets(fileToVal, MAX_FILENAME_SIZE, file) != NULL)
		{
			// if file has length of 1 it means that the only char is '\n'
			if (strlen(fileToVal) <= 1)
				continue;

			// Removes \n char from string
			if (strrchr(fileToVal, '\n') != NULL)
				strtok(fileToVal, "\n");

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
