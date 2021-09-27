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

int main(int argc, char *argv[]) {

    struct gengetopt_args_info args;

	// gengetopt parser: deve ser a primeira linha de código no main
	if(cmdline_parser(argc, argv, &args))
		ERROR(1, "Erro: execução de cmdline_parser\n");



	// gengetopt: libertar recurso (assim que possível)
	cmdline_parser_free(&args);

    return 0;
}
