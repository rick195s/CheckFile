/**
 * @file main.c
 * @brief Main file of checkFile
 * @date 2021-10-5
 * @author Ricardo dos Santos Franco 2202314
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/wait.h>
#include <dirent.h>
#include "args.h"
#include "debug.h"
#include "memory.h"
#include "mime.h"

// Global variables changed by batchProcessing and being read by signalProcessing
// when program receives SIGUSR1 signal
int file_number = 0;
char *file_name = NULL;
time_t init_batch_time;

int fileProcessing(char *file_path, int *summary);
int dirProcessing(char *dir_path, int *summary);
int batchProcessing(const char *batch_path, int *summary);
void showSummary(const int *summary);
void signalProcessing(int signal, siginfo_t *siginfo, void *context);

/**
 * Show summary of processed files
 * @param summary array with 3 positions (OK, MISMATCH, ERROR)
 * @return Nothing returned
 */
void signalProcessing(int signal, siginfo_t *siginfo, void *context)
{
	(void)context;
	int aux;
	/* Cópia da variável global errno */
	aux = errno;

	if (signal == SIGQUIT)
	{
		printf("Captured SIGQUIT signal (sent by PID: %ld). Use SIGINT to terminate application.\n", (long)siginfo->si_pid);
	}
	else if (signal == SIGUSR1 && file_name != NULL)
	{
		struct tm *timeinfo;
		timeinfo = localtime(&init_batch_time);

		printf("Started Processing Batch file at: %d.%d.%d_", (timeinfo->tm_year + 1900), timeinfo->tm_mon, timeinfo->tm_mday);
		printf("%dh%d:%d\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
		printf("File being analyzed'%s', nº%d of List\n", file_name, file_number);
	}

	/* Restaura valor da variável global errno */
	errno = aux;
}

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

	printf("[SUMMARY] files analyzed: %d; files OK: %d;", total, *summary);
	printf(" Mismatch: %d;", *(summary + 1));
	printf(" Errors: %d\n", *(summary + 2));
}

/**
 * Start of processing the file
 * @param file_path path to the file
 * @param summary array with 3 positions (OK, MISMATCH, ERROR)
 * @return 	0 -> all ok;
 * 			-1 -> error detected
 */
int fileProcessing(char *file_path, int *summary)
{
	// Checking file
	FILE *fd = fopen(file_path, "r");
	if (fd == NULL)
	{
		fprintf(stderr, "[ERROR] cannot open file '%s' -- %s\n", file_path, strerror(errno));
		(*(summary + 2))++;

		return -1;
	}

	fseek(fd, 0, SEEK_END);

	// If file empty ftell will return 0
	if (!ftell(fd))
	{
		printf("[INFO] '%s': empty file cannot be classified\n", file_path);
		fclose(fd);
		return -1;
	}

	char *mime_type = NULL;
	char *file_extension = MALLOC(MAX_EXT_SIZE);
	char *detected_extension = MALLOC(MAX_EXT_SIZE);

	mime_type = mimeParsing(mime_type, file_path);

	if (mime_type == NULL)
	{
		printf("[INFO] '%s': not hable to detect mime type\n", file_path);
		fclose(fd);
		FREE(detected_extension);
		FREE(file_extension);
		return -1;
	}

	if (getFileExtension(file_extension, file_path))
	{
		printf("[INFO] '%s': file without extension\n", file_path);
		fclose(fd);
		FREE(detected_extension);
		FREE(file_extension);
		FREE(mime_type);
		return -1;
	}

	// Showing output information
	switch (mimeValidation(mime_type, file_extension, detected_extension))
	{
	case 0:
		printf("[OK] '%s': extension '%s' matches file type '%s'\n", file_path, file_extension, detected_extension);
		(*summary)++;
		break;

	case -1:
		printf("[MISMATCH] '%s': extension is '%s', file type is '%s'\n", file_path, file_extension, detected_extension);
		(*(summary + 1))++;
		break;

	case -2:
		printf("[INFO] '%s': type '%s' is not supported by checkFile\n", file_path, mime_type);
		break;

	default:
		printf("[INFO] '%s': type '%s' is not supported by checkFile\n", file_path, mime_type);
		break;
	}

	// Freeing memory
	fclose(fd);
	FREE(detected_extension);
	FREE(file_extension);
	FREE(mime_type);

	return 0;
}

/**
 * Analysing the directory files
 * @param dir_path string to the directory
 * @param summary array with 3 positions (OK, MISMATCH, ERROR)
 * @return 	0 -> all ok;
 * 			-1 -> error detected
 */
int dirProcessing(char *dir_path, int *summary)
{
	DIR *dir = opendir(dir_path);
	struct dirent *dir_entry;
	int stop = 0;
	char *full_path = NULL;

	// Checking dir
	if (dir == NULL)
	{
		fprintf(stderr, "[ERROR] cannot open dir '%s' -- %s\n", dir_path, strerror(errno));
		exit(2);
	}

	// Checking last char of dir_path
	// Using values of ASCII table to avoid calling strcomp
	if ((int)dir_path[strlen(dir_path) - 1] != (int)'/')
	{
		// Adding space for '/' to the string
		dir_path = realloc(dir_path, 1);

		// Returning because if the dir doesn't have the / char the path to the file will
		// not be successful
		if (dir_path == NULL)
		{
			closedir(dir);
			fprintf(stderr, "[ERROR] not hable to allocate memory\n");
			exit(3);
		}

		strcat(dir_path, "/");
	}

	printf("[INFO] analyzing files of directory '%s'\n", dir_path);

	// Read from dir
	while (!stop)
	{
		dir_entry = readdir(dir);
		if (dir_entry == NULL)
		{
			if (errno)
			{
				fprintf(stderr, "[ERROR] cannot read from directory '%s' -- %s\n", dir_path, strerror(errno));
				closedir(dir);
				exit(4);
			}
			else
				stop = 1;
		}
		else
		{
			// +1 for the terminator '\0'
			full_path = MALLOC(strlen(dir_path) + strlen(dir_entry->d_name) + 1);
			if (full_path == NULL)
			{
				fprintf(stderr, "[ERROR] cannot allocate memory\n");
				closedir(dir);
				exit(5);
			}

			// Initializing 'full_path'
			memset(full_path, '\0', 1);
			strcat(full_path, dir_path);
			strcat(full_path, (dir_entry->d_name));
			fileProcessing(full_path, summary);
			FREE(full_path);
		}
	}

	closedir(dir);

	return 0;
}

/**
 * Analysing the files listed inside btachPath
 * @param btachPath string to the file
 * @param summary array with 3 positions (OK, MISMATCH, ERROR)
 * @return 	0 -> all ok;
 * 			-1 -> error detected
 */
int batchProcessing(const char *batch_path, int *summary)
{
	FILE *file = fopen(batch_path, "r");

	if (file == NULL)
	{
		fprintf(stderr, "[ERROR] cannot open file '%s' -- %s\n", batch_path, strerror(errno));
		exit(4);
	}

	char *file_to_val = MALLOC(MAX_FILENAME_SIZE);
	int file_error = 0;

	if (file_to_val == NULL)
	{
		fclose(file);
		exit(5);
	}

	time(&init_batch_time);
	printf("[INFO] analyzing files listed in '%s'\n", batch_path);

	// Read from file until EOF
	while (!feof(file))
	{
		file_error = ferror(file);
		if (file_error)
		{
			fprintf(stderr, "[ERROR] cannot read from file or dir '%s' -- %s\n", batch_path, strerror(file_error));
			fclose(file);
			FREE(file_to_val);
			return -1;
		}

		if (fgets(file_to_val, MAX_FILENAME_SIZE, file) != NULL)
		{
			// if file has length of 1 it means that the only char is '\n'
			if (strlen(file_to_val) <= 1)
				continue;

			// Removes \n char from string
			if (strrchr(file_to_val, '\n') != NULL)
				strtok(file_to_val, "\n");

			file_number++;
			fileProcessing(file_to_val, summary);
		}
	}
	fclose(file);
	FREE(file_to_val);

	return 0;
}

int main(int argc, char *argv[])
{
	struct sigaction act_info;
	struct gengetopt_args_info args;
	int summary[3] = {0};

	if (cmdline_parser(argc, argv, &args))
		ERROR(1, "Error: cmdline_parser\n");

	// What function will process signals
	act_info.sa_sigaction = signalProcessing;

	sigemptyset(&act_info.sa_mask);

	// Setting the flags
	act_info.sa_flags = 0;
	act_info.sa_flags |= SA_SIGINFO;
	act_info.sa_flags |= SA_RESTART;

	// Signal Caputure
	if (sigaction(SIGQUIT, &act_info, NULL) < 0)
		ERROR(2, "Sigaction creation\n");

	if (sigaction(SIGUSR1, &act_info, NULL) < 0)
		ERROR(3, "Sigaction creation\n");

	// Individual File Processing start
	if (args.files_given > 0)
		for (size_t i = 0; i < args.files_given; i++)
			fileProcessing(args.files_arg[i], summary);

	// Directory Processing start
	if (args.dir_given > 0)
	{
		dirProcessing(args.dir_arg, summary);
		showSummary(summary);
	}

	// Batch File Processing start
	if (args.batch_given > 0)
	{
		batchProcessing(args.batch_arg, summary);
		showSummary(summary);
	}

	// Freeing allocated memory
	cmdline_parser_free(&args);

	return 0;
}
