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
#define EXT_NUMBER 3

int getFileExtension(char *fileExtension, const char *filePath);
void extractFileType(FILE *outputFile, const char *filePath);
int mimeValidation(const char *mimeType, const char *fileExtension, char *detectedExtension);
char *mimeParsing(char *mimeType, const char *filePath);
int fileProcessing(const char *filePath, int *summary);
int dirProcessing(char *dirPath, int *summary);
int batchProcessing(const char *batchPath, int *summary);
void showSummary(const int *summary);

/**
 * Show summary of processed files
 * @param summary array with 3 positions (OK, MISMATCH, ERROR)
 * @return Nothing returned
 */
void showSummary(const int *summary)
{
	int total = 0;

	for (size_t i = 0; i < 3; i++)
	{
		total += *(summary + i);
	}

	printf("[SUMMARY] files analyzed: %d; files OK: %d; Mismatch: %d; Errors: %d\n", total, *summary, *(summary + 1), *(summary + 2));
}

/**
 * Gets the file extension
 * @param fileExtension string where the extension will be copied
 * @param filePath path to the file 
 * @return	0 -> extension was detected;
 * 			-1 -> extension not detected
 */
int getFileExtension(char *fileExtension, const char *filePath)
{
	char *ptr;

	// returns ptr if found '.' or NULL if not
	if ((ptr = strrchr(filePath, '.')) == NULL)
		return -1;

	// I want what's after the '.' character so +1
	strcpy(fileExtension, ptr + 1);
	return 0;
}

/**
 * Extracts the original file type and stores the output inside outputFile
 * @param outputFile pointer to FILE struct where file type of filePath will be stored
 * @param filePath path to the file 
 * @return Nothing returned
 */
void extractFileType(FILE *outputFile, const char *filePath)
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
}

/**
 * Validates the file extension with the actual file type
 * @param mimeType string where the mime type detected by the bash program "file" is stored
 * @param fileExtension string where the extracted extension from the file path string is stored
 * @param detectedExtension string where actual extension, if detected, will be stored
 * @return 	0 -> extensions are compatible;
 * 			-1 -> mime supported but is a mismatch;
 * 			-2 -> mime not supported
 */
int mimeValidation(const char *mimeType, const char *fileExtension, char *detectedExtension)
{
	// define the supported mime types and extensions
	char *types[EXT_NUMBER] = {"application/pdf", "image/png", "image/jpeg"};
	char *extensions[EXT_NUMBER] = {"pdf", "png", "jpeg/jpg/jpe/jfif"};

	strcpy(detectedExtension, "");

	for (size_t i = 0; i < EXT_NUMBER; i++)
		// Check if mimeType is supported by checkFile
		if (!strcmp(types[i], mimeType))
		{
			strcpy(detectedExtension, extensions[i]);
			if (strstr(extensions[i], fileExtension) != NULL)
				return 0;
		}

	// if detectedExtension is empty string then the mime type isn't supported
	if (!strcmp(detectedExtension, ""))
		return -2;

	return -1;
}

/**
 * Analyzes the mime of filePath
 * @param mimeType string where the mime type detected by the bash program "file" will be stored
 * @param filePath path to the file 
 * @return 	pointer to memory for string with the mime type or NULL
 */
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
		fprintf(stderr, "[ERROR] not hable to allocate memory\n");
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

/**
 * Start of processing the file
 * @param filePath path to the file 
 * @param summary array with 3 positions (OK, MISMATCH, ERROR)
 * @return 	0 -> all ok;
 * 			-1 -> error detected
 */
int fileProcessing(const char *filePath, int *summary)
{
	// Checking file permissions
	int asPerm = access(filePath, R_OK);
	if (asPerm != 0)
	{
		fprintf(stderr, "[ERROR] cannot open file '%s' -- %s\n", filePath, strerror(errno));
		(*(summary + 2))++;

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
		(*summary)++;
		break;

	case -1:
		printf("[MISMATCH] '%s': extension is '%s', file type is '%s'\n", filePath, fileExtension, detectedExtension);
		(*(summary + 1))++;
		break;

	case -2:
		printf("[INFO] '%s': type '%s' is not supported by checkFile\n", filePath, mimeType);
		break;

	default:
		break;
	}

	// Freeing memory
	FREE(detectedExtension);
	FREE(fileExtension);
	FREE(mimeType);

	return 0;
}

/**
 * Analysing the directory files 
 * @param dirPath string to the directory
 * @param summary array with 3 positions (OK, MISMATCH, ERROR)
 * @return 	0 -> all ok;
 * 			-1 -> error detected
 */
int dirProcessing(char *dirPath, int *summary)
{
	DIR *dir = opendir(dirPath);
	struct dirent *dir_entry;
	int fim = 0;
	char *fullPath = NULL;

	if (dir == NULL)
	{
		fprintf(stderr, "[ERROR] cannot open dir '%s' -- %s\n", dirPath, strerror(errno));
		return -1;
	}

	// Checking last char of dirPath
	// Using values of ASCII table to avoid calling strcomp
	if ((int)dirPath[strlen(dirPath) - 1] != (int)'/')
	{
		// Adding space for '/' to the string
		dirPath = realloc(dirPath, 1);

		if (dirPath == NULL)
		{
			closedir(dir);
			fprintf(stderr, "[ERROR] not hable to allocate memory\n");
			return -1;
		}

		strcat(dirPath, "/");
	}

	printf("[INFO] analyzing files of directory '%s'\n", dirPath);

	// Read from dir
	while (fim == 0)
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
				fim = 1;
		}
		else
		{
			// +1 for the terminator '\0'
			fullPath = MALLOC(strlen(dirPath) + strlen(dir_entry->d_name) + 1);
			if (fullPath == NULL)
			{
				fprintf(stderr, "[ERROR] cannot allocate memory\n");
				return -1;
			}

			// Initializing 'fullPath'
			memset(fullPath, '\0', 1);
			//strcpy(fullPath, "");
			strcat(fullPath, dirPath);
			strcat(fullPath, (dir_entry->d_name));
			fileProcessing(fullPath, summary);
			FREE(fullPath);
		}
	}

	if (closedir(dir) == -1)
	{
		fprintf(stderr, "[ERROR] cannot close dir '%s' -- %s\n",
				dirPath, strerror(errno));

		return -1;
	}

	return 0;
}

/**
 * Analysing the files listed inside btachPath 
 * @param btachPath string to the file
 * @param summary array with 3 positions (OK, MISMATCH, ERROR)
 * @return 	0 -> all ok;
 * 			-1 -> error detected
 */
int batchProcessing(const char *batchPath, int *summary)
{
	FILE *file = fopen(batchPath, "r");

	if (file == NULL)
	{
		fprintf(stderr, "[ERROR] cannot open file '%s' -- %s\n", batchPath, strerror(errno));
		return -1;
	}

	printf("[INFO] analyzing files listed in '%s'\n", batchPath);

	char *fileToVal = MALLOC(MAX_FILENAME_SIZE);
	int fileError = 0;

	// Read from file until EOF
	while (!feof(file))
	{
		fileError = ferror(file);
		if (fileError)
		{
			fprintf(stderr, "[ERROR] cannot read from file or dir '%s' -- %s\n", batchPath, strerror(fileError));
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

			fileProcessing(fileToVal, summary);
		}
	}

	fclose(file);
	FREE(fileToVal);

	return 0;
}

int main(int argc, char *argv[])
{

	struct gengetopt_args_info args;
	int summary[3] = {0};

	// gengetopt parser: deve ser a primeira linha de código no main
	if (cmdline_parser(argc, argv, &args))
		ERROR(1, "Erro: execução de cmdline_parser\n");

	if (args.files_given > 0)
		for (size_t i = 0; i < args.files_given; i++)
			fileProcessing(args.files_arg[i], summary);

	if (args.dir_given > 0)
	{
		dirProcessing(args.dir_arg, summary);
		showSummary(summary);
	}

	if (args.batch_given > 0)
	{
		batchProcessing(args.batch_arg, summary);
		showSummary(summary);
	}

	// gengetopt: libertar recurso (assim que possível)
	cmdline_parser_free(&args);

	return 0;
}
