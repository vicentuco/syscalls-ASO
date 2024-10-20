//Vicente Gonzalez Morales y Alvaro Cutillas Florido Subgrupo 3.3

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>

#define MAX_BUF_SIZE 8192
#define MIN_BUF_SIZE 1
#define DEFAULT_BUF_SIZE 16
#define MAX_LINE_SIZE 1024
#define DEFAULT_LINE_SIZE 32
#define MIN_LINE_SIZE 16

#define SEPARATOR " "
#define CONTROL_CHAR '\n'

struct line
{
    int number;
    int max_size;
    char *content;
};

struct command
{
    char *bin;
    char **arguments;
};


void print_help(char* nombre_programa);

typedef struct line *line_t;

typedef struct command *command_t;

void execute_line(line_t line);

void parse_line(line_t line);

void execute_command(char** arguments);


void print_line(line_t line) {
    printf("Línea número: %d\n", line->number);
    printf("Tamaño máximo de la línea: %d\n", line->max_size);
    printf("Contenido de la línea: %s\n\n", line->content);
}


int main(int argc, char *argv[]) 
{
    int opt;
    int buf_size = DEFAULT_BUF_SIZE;
    int line_size = DEFAULT_LINE_SIZE;
    int line;

    optind = 1;
    while ((opt = getopt(argc, argv, "l:b:h")) != -1)
    {
        switch (opt)
        {
        case 'b':
            buf_size = -1;
            buf_size = atoi(optarg);
            break;
		case 'l':
            line_size = -1;
			line_size = atoi(optarg);
			break;
        case 'h':
            print_help(argv[0]);
            exit(EXIT_SUCCESS);
        default:
            print_help(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if(buf_size == -1)
    {
        fprintf(stderr, "%s: option requires an argument -- 'b'\n", argv[0]);
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    if(line_size == -1)
    {
        fprintf(stderr, "%s: option requires an argument -- 'l'\n", argv[0]);
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    if ((buf_size < MIN_BUF_SIZE) || (buf_size > MAX_BUF_SIZE)) 
    {
        fprintf(stderr, "Error: El tamaño de buffer tiene que estar entre 1 y 8192.\n");
        fprintf(stderr, "%s: option requires an argument -- 'b'\n", argv[0]);
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }

	if ((line_size < MIN_LINE_SIZE) || (line_size > MAX_LINE_SIZE)){
		fprintf(stderr, "Error: El tamaño máximo de linea tiene que estar entre 16 y 1024.\n");
		fprintf(stderr, "%s: option requires an argument -- 'l'\n", argv[0]);
		print_help(argv[0]);
		exit(EXIT_FAILURE);
	}

    //--------------------------------------------------------------//
    ssize_t num_read;
    ssize_t bytesTotal = 0;
    char *buffer;
    char *bufferComando;
    int num_line = 1;
    int cont = 0;
    line_t linea;

    linea = malloc(sizeof(struct line));
    if (linea == NULL) {
        perror("malloc()");
        exit(EXIT_FAILURE);
    }

    /* Reserva memoria dinámica para buffer de lectura */
    if ((buffer = malloc(buf_size * sizeof(char))) == NULL)
    {
        perror("malloc()");
        exit(EXIT_FAILURE);
    }

    if ((bufferComando = malloc(line_size * sizeof(char))) == NULL)
    {
        perror("malloc()");
        exit(EXIT_FAILURE);
    }

    while ((num_read = read(STDIN_FILENO, buffer, buf_size)) > 0)
    {
        if(cont > line_size)
        {
            fprintf(stderr, "Error: línea %d demasiado larga: \n", num_line);
            exit(EXIT_FAILURE);
        }

        for (ssize_t i = 0; i < num_read; i++)
        {
            if (buffer[i] == CONTROL_CHAR)
            {
                bufferComando[cont] = '\0';
                linea->number = num_line;
                linea->max_size = line_size;
                linea->content = bufferComando;
                parse_line(linea);
                cont = 0;
                num_line++;
            }
            else
            {
                bufferComando[cont] = buffer[i];
                cont++;
            }
        }
    }
}

void print_help(char* nombre_programa)
{
	fprintf(stderr, "Uso: %s [-b BUF_SIZE] [-l MAX_LINE_SIZE]\nLee de la entrada estándar una secuencia de líneas conteniendo órdenes\npara ser ejecutadas y lanza los procesosnecesarios para ejecutar cada línea, esperando a su terminacion para ejecutar la siguiente.\n-b BUF_SIZE\tTamaño del buffer de entrada 1<=BUF_SIZE<=8192\n-l MAX_LINE_SIZE\tTamaño máximo de línea 16<=MAX_LINE_SIZE<=1024\n", nombre_programa);
}

void parse_line(line_t line) 
{
    print_line(line);

    char **tokens = malloc((line->max_size/2 + 1) * sizeof(char *));
    int i = 0;

    char *saveptr;
    char *token_read = strtok_r(line->content, SEPARATOR, &saveptr);
    while(token_read != NULL) 
    {
        tokens[i] = token_read;
        i++;
        token_read = strtok_r(NULL, SEPARATOR, &saveptr);
    }
    
    tokens[i] = NULL;

    for (int j = 0; j < i; j++)
    {
        printf("Token %d: %s\n", j + 1, tokens[j]);
    }

    printf("\n");

    execute_command(tokens);
}

void execute_command(char** arguments)
{
    char *bin1 = arguments[0];
    char *bin2 = NULL;
    int fd = -1;
    int trunc = 0;
    char *file_name = NULL;

    int i = 0;
    while(arguments[i] != NULL)
    {
        if(strcmp(arguments[i], "<") == 0)
        {
            fd = STDIN_FILENO;
            file_name = arguments[i+1];
        }
        else if(strcmp(arguments[i], ">") == 0)
        {
            fd = STDOUT_FILENO;
            trunc = 0;
            file_name = arguments[i+1];
        }
        else if(strcmp(arguments[i], ">>") == 0)
        {
            fd = STDERR_FILENO;
            trunc = 1;
            file_name = arguments[i+1];
        }
        else if(strcmp(arguments[i], "|") == 0)
        {
            bin2 = file_name = arguments[i+1];
        }

        i++;
    }

}
