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
typedef struct line *line_t;

void print_help(char* nombre_programa);
void parse_line(line_t line);
void execute_command(char** arguments);
void redir_command(char *bin, char **args, char *file_name, int fd, int trunc, int type);
void pipe_command(char *bin_out, char **args_out, char *bin_in, char **args_in);


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

    ssize_t num_read;
    ssize_t bytesTotal = 0;
    char *buffer;
    char *bufferComando;
    int num_line = 1;
    int cont = 0;
    line_t linea;

    linea = malloc(sizeof(line_t));
    if (linea == NULL) {
        perror("malloc()");
        exit(EXIT_FAILURE);
    }

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

void print_line(line_t line) 
{
    printf("Línea número: %d\n", line->number);
    printf("Tamaño máximo de la línea: %d\n", line->max_size);
    printf("Contenido de la línea: %s\n\n", line->content);
}

void print_help(char* nombre_programa)
{
	fprintf(stderr, "Uso: %s [-b BUF_SIZE] [-l MAX_LINE_SIZE]\nLee de la entrada estándar una secuencia de líneas conteniendo órdenes\npara ser ejecutadas y lanza los procesosnecesarios para ejecutar cada línea, esperando a su terminacion para ejecutar la siguiente.\n-b BUF_SIZE\tTamaño del buffer de entrada 1<=BUF_SIZE<=8192\n-l MAX_LINE_SIZE\tTamaño máximo de línea 16<=MAX_LINE_SIZE<=1024\n", nombre_programa);
}

void parse_line(line_t line) 
{
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

    execute_command(tokens);
}

void execute_command(char** arguments)
{
    char *bin1 = arguments[0];
    char *bin2 = NULL;
    int trunc = 0;

    int i = 0;
    while(arguments[i] != NULL)
    {
        if(strcmp(arguments[i], "<") == 0)
        {
            arguments[i] = NULL;
            redir_command(bin1, arguments, arguments[i+1], STDIN_FILENO, 1, 1);
        }
        else if(strcmp(arguments[i], ">") == 0)
        {
            arguments[i] = NULL;
            redir_command(bin1, arguments, arguments[i+1], STDOUT_FILENO, 1, 0);
        }
        else if(strcmp(arguments[i], ">>") == 0)
        {
            arguments[i] = NULL;
            redir_command(bin1, arguments, arguments[i+1], STDOUT_FILENO, 0, 0);
        }
        else if(strcmp(arguments[i], "|") == 0)
        {
            arguments[i] = NULL;
            char **args1 = arguments;
            char **args2 = arguments + (i+1);
            char *bin2 = arguments[i+1];
            pipe_command(bin1, args1, bin2, args2);
        }

        i++;
    }

}

// type : tipo de fichero. 1 para entrada, 0 para salida.
void redir_command(char *bin, char **args, char *file_name, int fd, int trunc, int type) 
{
    if(close(fd) == -1) 
    {
        perror("close()");
        exit(EXIT_FAILURE);
    }

    int flags = -1;
    if(!type)    // Fichero de salida
    {
        if(trunc) 
        {
            flags = O_WRONLY | O_CREAT | O_TRUNC;
        }
        else 
        {
            flags = O_WRONLY | O_CREAT | O_APPEND;
        }

        if(open(file_name, flags, S_IWUSR | S_IRUSR) == -1) 
        {
            perror("open(fileout)");
            exit(EXIT_FAILURE);
        }
    }
    else    // Fichero de entrada
    {
        if(open(file_name, O_RDONLY) == -1) 
            {
                perror("open(filein)");
                exit(EXIT_FAILURE);
            }
    }

    switch (fork())
    {
    case -1:
        perror("fork(1)");
        exit(EXIT_FAILURE);
        break;
    case 0:
        execvp(bin, args);
        perror("execvp(redir)");
        exit(EXIT_FAILURE);
        break;
    default:
        break;
    }
}

void pipe_command(char *bin_left, char **args_left, char *bin_right, char **args_right) 
{
    int pipefds[2];
    if (pipe(pipefds) == -1)
    {
        perror("pipe()");
        exit(EXIT_FAILURE);
    }

    switch (fork())     // Hijo izquierdo de la tubería
    {
        case -1:
            perror("fork(1)");
            exit(EXIT_FAILURE);
            break;
        case 0: 
            if (close(pipefds[0]) == -1)
            {
                perror("close(1)");
                exit(EXIT_FAILURE);
            }
           
            if (dup2(pipefds[1], STDOUT_FILENO) == -1)
            {
                perror("dup2(1)");
                exit(EXIT_FAILURE);
            }
            
            if (close(pipefds[1]) == -1)
            {
                perror("close(2)");
                exit(EXIT_FAILURE);
            }
           
            execvp(bin_left, args_left);
            perror("execlp(izquierdo)");
            exit(EXIT_FAILURE);
            break;
        default:
            break;
    }

    switch (fork())     // Hijo derecho de la tubería
    {
        case -1:
            perror("fork(2)");
            exit(EXIT_FAILURE);
            break;
        case 0: 
            if (close(pipefds[1]) == -1)
            {
                perror("close(3)");
                exit(EXIT_FAILURE);
            }
            
            if (dup2(pipefds[0], STDIN_FILENO) == -1)
            {
                perror("dup2(2)");
                exit(EXIT_FAILURE);
            }
            
            if (close(pipefds[0]) == -1)
            {
                perror("close(4)");
                exit(EXIT_FAILURE);
            }
            
            execvp(bin_right, args_right);
            perror("execlp(derecho)");
            exit(EXIT_FAILURE);
            break;
        default: 
            break;
    }

    // Cerramos los descriptores de fichero del padre
    if (close(pipefds[0]) == -1)
    {
        perror("close(pipefds[0])");
        exit(EXIT_FAILURE);
    }
    if (close(pipefds[1]) == -1)
    {
        perror("close(pipefds[1])");
        exit(EXIT_FAILURE);
    }
}