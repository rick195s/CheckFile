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
#include <time.h>
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

// Global variables changed by batchProcessing and being read by signalProcessing
// when program receives SIGUSR1 signal
int file_number = 0;
char *file_name = NULL;
time_t init_batch_time;

int getFileExtension(char *file_extension, const char *file_path);
void extractMimeTypeTo(FILE *output_file, const char *file_path);
int mimeValidation(const char *mime_type, const char *file_extension, char *detected_extension);
char *mimeParsing(char *mime_type, const char *file_path);
int fileProcessing(const char *file_path, int *summary);
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
 * Gets the file extension
 * @param file_extension string where the extension will be copied
 * @param file_path path to the file
 * @return	0 -> extension was detected;
 * 			-1 -> extension not detected
 */
int getFileExtension(char *file_extension, const char *file_path)
{
	char *ptr;

	// returns ptr if found '.' or NULL if not
	if ((ptr = strrchr(file_path, '.')) == NULL)
		return -1;

	// I want what's after the '.' character so +1
	strcpy(file_extension, ptr + 1);
	return 0;
}

/**
 * Extracts the original file type and stores the output inside output_file
 * @param output_file pointer to FILE struct where file type of file_path will be stored
 * @param file_path path to the file
 * @return Nothing returned
 */
void extractMimeTypeTo(FILE *output_file, const char *file_path)
{

	// Creates child process
	pid_t id = fork();

	if (id == 0)
	{
		// Redirecting the child output to out.txt
		dup2(fileno(output_file), STDOUT_FILENO);
		execlp("file", "file", "--mime-type", file_path, NULL);
	}
	else
		// Parent waits for Child
		wait(NULL);
}

/**
 * Validates the file extension with the actual file type
 * @param mime_type string where the mime type detected by the bash program "file" is stored
 * @param file_extension string where the extracted extension from the file path string is stored
 * @param detected_extension string where actual extension, if detected, will be stored
 * @return 	0 -> extensions are compatible;
 * 			-1 -> mime supported but is a mismatch;
 * 			-2 -> mime not supported
 */
int mimeValidation(const char *mime_type, const char *file_extension, char *detected_extension)
{
	// define the supported mime types and extensions
	char *types[EXT_NUMBER] = {"application/pdf", "image/png", "image/jpeg"};
	char *extensions[EXT_NUMBER] = {"pdf", "png", "jpeg/jpg/jpe/jfif"};

	strcpy(detected_extension, "");

	for (size_t i = 0; i < EXT_NUMBER; i++)
		// Check if mime_type is supported by checkFile
		if (!strcmp(types[i], mime_type))
		{
			strcpy(detected_extension, extensions[i]);
			if (strstr(extensions[i], file_extension) != NULL)
				return 0;
		}

	// if detected_extension is empty string then the mime type isn't supported
	if (!strcmp(detected_extension, ""))
		return -2;

	return -1;
}

/**
 * Analyzes the mime of file_path
 * @param mime_type string where the mime type detected by the bash program "file" will be stored
 * @param file_path path to the file
 * @return 	pointer to memory for string with the mime type or NULL
 */
char *mimeParsing(char *mime_type, const char *file_path)
{
	FILE *output_file = fopen("out.txt", "w+");

	if (output_file == NULL)
		return NULL;

	extractMimeTypeTo(output_file, file_path);

	// saves the file size in bytes
	size_t file_size = lseek(fileno(output_file), 0, SEEK_END);

	lseek(fileno(output_file), 0, SEEK_SET);

	// sizeof(char) = 1
	mime_type = MALLOC(file_size * sizeof(char));

	if (mime_type == NULL)
	{
		fclose(output_file);
		fprintf(stderr, "[ERROR] not hable to allocate memory\n");
		exit(2);
	}

	// gets the fist line
	fgets(mime_type, (int)file_size, output_file);

	// finds the last occurence of ':' and adds 2
	// so mime_type wont have ': '
	strcpy(mime_type, strrchr(mime_type, ':') + 2);
	fclose(output_file);

	return mime_type;
}

/**
 * Start of processing the file
 * @param file_path path to the file
 * @param summary array with 3 positions (OK, MISMATCH, ERROR)
 * @return 	0 -> all ok;
 * 			-1 -> error detected
 */
int fileProcessing(const char *file_path, int *summary)
{
	// Checking file permissions
	int as_perm = access(file_path, R_OK);
	if (as_perm != 0)
	{
		fprintf(stderr, "[ERROR] cannot open file '%s' -- %s\n", file_path, strerror(errno));
		(*(summary + 2))++;

		return -1;
	}

	char *mime_type = NULL;
	char *file_extension = MALLOC(MAX_EXT_SIZE);
	char *detected_extension = MALLOC(MAX_EXT_SIZE);

	mime_type = mimeParsing(mime_type, file_path);

	if (mime_type == NULL)
		return -1;

	if (getFileExtension(file_extension, file_path))
		return -1;

	// Show output information
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
		break;
	}

	// Freeing memory
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
	int fim = 0;
	char *full_path = NULL;

	if (dir == NULL)
	{
		fprintf(stderr, "[ERROR] cannot open dir '%s' -- %s\n", dir_path, strerror(errno));
		return -1;
	}

	// Checking last char of dir_path
	// Using values of ASCII table to avoid calling strcomp
	if ((int)dir_path[strlen(dir_path) - 1] != (int)'/')
	{
		// Adding space for '/' to the string
		dir_path = realloc(dir_path, 1);

		if (dir_path == NULL)
		{
			closedir(dir);
			fprintf(stderr, "[ERROR] not hable to allocate memory\n");
			return -1;
		}

		strcat(dir_path, "/");
	}

	printf("[INFO] analyzing files of directory '%s'\n", dir_path);

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
			full_path = MALLOC(strlen(dir_path) + strlen(dir_entry->d_name) + 1);
			if (full_path == NULL)
			{
				fprintf(stderr, "[ERROR] cannot allocate memory\n");
				return -1;
			}

			// Initializing 'full_path'
			memset(full_path, '\0', 1);
			// strcpy(full_path, "");
			strcat(full_path, dir_path);
			strcat(full_path, (dir_entry->d_name));
			fileProcessing(full_path, summary);
			FREE(full_path);
		}
	}

	if (closedir(dir) == -1)
	{
		fprintf(stderr, "[ERROR] cannot close dir '%s' -- %s\n",
				dir_path, strerror(errno));

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
int batchProcessing(const char *batch_path, int *summary)
{
	FILE *file = fopen(batch_path, "r");

	if (file == NULL)
	{
		fprintf(stderr, "[ERROR] cannot open file '%s' -- %s\n", batch_path, strerror(errno));
		return -1;
	}

	file_name = MALLOC(MAX_FILENAME_SIZE * sizeof(char));
	char *file_to_val = MALLOC(MAX_FILENAME_SIZE);
	int file_error = 0;

	if (file_name == NULL || file_to_val == NULL)
		return -1;

	// int number_of_files = 0;
	// char **list_of_files = NULL;
	time(&init_batch_time);
	printf("[INFO] analyzing files listed in '%s'\n", batch_path);

	// Read from file until EOF
	while (!feof(file))
	{
		file_error = ferror(file);
		if (file_error)
		{
			fprintf(stderr, "[ERROR] cannot read from file or dir '%s' -- %s\n", batch_path, strerror(file_error));
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
			strcpy(file_name, file_to_val);
			fileProcessing(file_to_val, summary);
			// listOfFiles = (char **)realloc(listOfFiles, (numberOfFiles + 1) * sizeof(char *));
			//*(listOfFiles + numberOfFiles) = (char *)MALLOC(MAX_FILENAME_SIZE * sizeof(char));
			// strcpy(*(listOfFiles + numberOfFiles), file_to_val);
			// numberOfFiles++;
		}
	}

	// Freeing used memory
	// for (int i = 0; i < numberOfFiles; i++)
	//	FREE(*(listOfFiles + i));

	// FREE(listOfFiles);
	fclose(file);
	FREE(file_to_val);

	return 0;
}

int main(int argc, char *argv[])
{
	struct sigaction act_info;
	struct gengetopt_args_info args;
	int summary[3] = {0};

	printf("Meu PID: %d\n", getpid());

	if (cmdline_parser(argc, argv, &args))
		ERROR(1, "Error: cmdline_parser\n");

	// What function will process signals
	act_info.sa_sigaction = signalProcessing;

	sigemptyset(&act_info.sa_mask);

	// Setting the flags
	act_info.sa_flags = 0;
	act_info.sa_flags |= SA_SIGINFO; // info adicional sobre o sinal
	act_info.sa_flags |= SA_RESTART; // recupera chamadas bloqueantes

	// Signal Caputure
	if (sigaction(SIGQUIT, &act_info, NULL) < 0)
		ERROR(4, "sigaction (sa_sigaction) - ???");

	if (sigaction(SIGUSR1, &act_info, NULL) < 0)
		ERROR(4, "sigaction (sa_sigaction) - ???");

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
		// Signal Caputure
		if (sigaction(SIGUSR1, &act_info, NULL) < 0)
			ERROR(4, "sigaction (sa_sigaction) - ???");

		batchProcessing(args.batch_arg, summary);
		showSummary(summary);
	}

	// Freeing allocated memory
	cmdline_parser_free(&args);

	return 0;
}
