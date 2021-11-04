/**
 * @file mime.c
 * @brief File with mime type related operations
 * @date 2021-10-5
 * @author Ricardo dos Santos Franco 2202314
 */

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include "debug.h"
#include "memory.h"
#include "mime.h"

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
    if ((ptr = strrchr(file_path, (int)'.')) == NULL)
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
    if (fork() == 0)
    {
        // Redirecting the child output to out.txt
        dup2(fileno(output_file), STDOUT_FILENO);
        if (execlp("file", "file", "--mime-type", file_path, NULL))
            ERROR(2, "Error executing 'file' bash program -- ");
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
    char *types[EXT_NUMBER] = {
        "application/pdf",
        "image/gif",
        "image/jpeg",
        "image/png",
        "video/mp4",
        "application/x-7z-compressed",
        "text/html",
    };
    char *extensions[EXT_NUMBER] = {
        "pdf",
        "gif",
        "jpeg/jpg/jpe/jfif",
        "png",
        "mp4",
        "7z/cb7",
        "html",
    };

    strcpy(detected_extension, "");

    for (size_t i = 0; i < EXT_NUMBER; i++)
        // Check if mime_type is supported by checkFile
        if (!strcmp(types[i], mime_type))
        {
            strcpy(detected_extension, extensions[i]);
            if (strstr(detected_extension, file_extension) != NULL)
                return 0;

            return -1;
        }

    // mime type extracted isn't supported
    return -2;
}

/**
 * Analyzes the mime of file_path
 * @param mime_type string where the mime type detected by the bash program "file" will be stored
 * @param file_path path to the file
 * @return 	pointer to memory for string with the mime type or NULL
 */
char *mimeParsing(char *mime_type, const char *file_path)
{
    FILE *output_file = fopen(OUTPUT_FILENAME, "w+");

    if (output_file == NULL)
        return NULL;

    char string[MAX_FILENAME_SIZE];

    extractMimeTypeTo(output_file, file_path);

    // saves the file size in bytes
    size_t file_size = lseek(fileno(output_file), 0, SEEK_END);

    lseek(fileno(output_file), 0, SEEK_SET);

    mime_type = MALLOC(file_size * sizeof(char));

    if (mime_type == NULL)
    {
        fclose(output_file);
        fprintf(stderr, "[ERROR] not hable to allocate memory\n");
        exit(2);
    }

    // gets the fist line
    fgets(mime_type, (int)file_size, output_file);

    // Using aux string so source and destination of strcpy won't overlap.
    // Find the last occurence of ':' and adds 2
    // so mime_type wont have ': '
    strcpy(string, strrchr(mime_type, ':') + 2);
    strcpy(mime_type, string);
    fclose(output_file);

    // not verifying if file was successful removed because I don't want to notify
    // the user about that
    remove(OUTPUT_FILENAME);

    return mime_type;
}
