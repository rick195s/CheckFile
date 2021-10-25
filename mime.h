/**
 * @file mime.h
 * @date 2021-10-5
 * @author Ricardo dos Santos Franco 2202314
 */

#define EXT_NUMBER 7
#define OUTPUT_FILENAME "out.txt"
#define MAX_EXT_SIZE 30
#define MAX_FILENAME_SIZE 100

int getFileExtension(char *file_extension, const char *file_path);
void extractMimeTypeTo(FILE *output_file, const char *file_path);
int mimeValidation(const char *mime_type, const char *file_extension, char *detected_extension);
char *mimeParsing(char *mime_type, const char *file_path);