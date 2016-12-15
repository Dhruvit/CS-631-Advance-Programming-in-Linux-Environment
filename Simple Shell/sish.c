/*
 * This is simple shell implemented by
 * Dhruvit(CWID:10404032) and Jignesh(CWID:10400165)
 *
 */

#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <bsd/stdlib.h>

int tracing_mode = 0;
int status_code_last_command ;
int parent_pid = 2;

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"

int number_of_builtins();
int sish_cd(char**);
int sish_echo(char**);
int sish_help(char**);
int sish_exit(char**);

int status;

struct cmdLine{
	char **arguments;
	struct cmdLine *next;
};

struct cmdLine *Head_node = NULL;

/*
 * List of I/O redirection commands
 */
char *redirection[] = {
  "<", //input redirection
  ">", //output redirection
  ">>", //output redirection, appends not truncates
  "|", //pipe
  "&" //background
};

/*
 * List of builtins commands, followed by their corresponding functions.
 */
char *builtin_str[] = {
		"cd",
		"echo",
		"exit"
};

int (*builtin_func[]) (char **) = {
		&sish_cd,
		&sish_echo,
		&sish_exit
};

/*
 * Interrupt handler function
 */
void handle_signal(int sig){
	printf("\nsish$ ");
	fflush(stdout);
}

/*
 * Builtin function implementation
 */
int sish_cd(char **args){
	if (args[1] == NULL) {
		fprintf(stderr, "sish: expected argument to \"cd\"\n");
	}else {
		if (chdir(args[1]) != 0) {
			perror("sish");
		}
	}

	return 1;
}

int sish_echo(char **args){

	pid_t pid;

	if (args[1] == NULL) {
		fprintf(stderr, "sish: expected argument to \"cd\"\n");
	}else{
		if( strcmp( args[1], "$?" ) == 0 ){
			printf("%d\n",status);
		}else if( strcmp( args[1], "$$" ) == 0 ){
			pid = fork();
			if(pid == 0)
				printf("%d\n",getppid());
			else if( pid < 0 )
				perror("sish");
		}else{
			printf("%s\n",args[1]);
		}
	}
	return 1;
}

int sish_exit(char **args){

	return 0;
}

int number_of_builtins(){
	return sizeof(builtin_str) / sizeof(char *);
}

int argsLength(char **args){

	int i = 0;
	int len = 0;
	while (args[i] != NULL) {
		len++;
		i++;
	}
	return len;
}

void create_cmdLine_pipe(char **args, int countPipes) {
	int bufsize = LSH_TOK_BUFSIZE;
	int len = argsLength(args);
	int i = 0, j = 0;
	int enable = 0;
	char **tokens_backup;

	struct cmdLine *current = Head_node;
	struct cmdLine *new_node = NULL;

	for (i = 0; i < countPipes + 1; i++) {

		new_node = malloc(sizeof(struct cmdLine)); /*create new node*/
		enable = 1;
		new_node->arguments = malloc(bufsize * sizeof(char*));

		int position = 0;
		while (enable) {
			if (strcmp(args[j], redirection[3]) == 0) {
				enable = 0;

			} else {
				new_node->arguments[position] = args[j];

				position++;
			}

			if (position >= bufsize) {
				bufsize += LSH_TOK_BUFSIZE;
				tokens_backup = new_node->arguments;
				new_node->arguments = realloc(new_node->arguments,bufsize * sizeof(char*));

				if (!new_node->arguments) {
					free(tokens_backup);
					fprintf(stderr, "sish: allocation error\n");
					exit(EXIT_FAILURE);
				}
			}

			j++;
			if (j == len)
				enable = 0;
		}
		new_node->next = NULL;

		if (Head_node == NULL) {
			Head_node = new_node;
			current = new_node;
		} else {
			current->next = new_node;
			current = new_node;
		}
	}
}

/*
 * execute multiple pipe
 */
int runPipe(struct cmdLine *command, int count_pipe, int io_amper){

	int numPipes = count_pipe;
	int status;
	int i = 0;
	pid_t pid;

	int pipefd[2 * numPipes];

	for (i = 0; i < (numPipes); i++) {
		if (pipe(pipefd + i * 2) < 0) {
			perror("Can not create pipe");
			exit(EXIT_FAILURE);
		}
	}
	int commandc = 0;

	while ( command ) {

		if(tracing_mode){
			int k = 0;
			int len = argsLength(command->arguments);
			printf("+ ");
			for(k=0;k<len;k++)
			{
				printf("%s ",command->arguments[k]);
			}
			printf("\n");
		}

		pid = fork();
		if (pid == 0) {

			if ( command->next ) {
				if (dup2(pipefd[commandc + 1], 1) < 0) {
					perror("dup2");
					exit(EXIT_FAILURE);
				}
			}

			if (commandc != 0) {
				if (dup2(pipefd[commandc -  2], 0) < 0) {
					perror("dup2: 2nd");
					exit(EXIT_FAILURE);

				}
			}

			for (i = 0; i < 2 * numPipes; i++) {
				close(pipefd[i]);
			}

			if (execvp(*command->arguments, command->arguments) < 0) {
				fprintf(stderr,"%s: command not found\n",*command->arguments);
				exit(EXIT_FAILURE);
			}
		} else if (pid < 0) {
			perror("error");
			exit(EXIT_FAILURE);
		}

		command = command->next;
		commandc+=2;
	}

	/* Parent closes the pipes and wait for children */
	for (i = 0; i < 2 * numPipes; i++) {
		close(pipefd[i]);
	}

	for (i = 0; i < numPipes + 1; i++){
		waitpid(pid, &status, WUNTRACED);

	}
	return status;
}

int sish_launch(char **args){

	pid_t pid;
	int len = 0;
	int i = 0;
	int io_output = 0;
	int io_input = 0;
	int io_amper = 0;
	int io_pipe = 0;
	int count_pipe = 0;
	int io_output2 = 0;
	int in, out;

	len = argsLength(args);

	for(i=0;i<len;i++) {
	    if(strcmp(args[i], redirection[0]) == 0) { io_input=i; }
	    else if(strcmp(args[i], redirection[1]) == 0) { io_output=i; }
	    else if(strcmp(args[i], redirection[2]) == 0) { io_output2=i; }
	    else if(strcmp(args[i], redirection[3]) == 0) {
	    	io_pipe=1;
	    	count_pipe++;
	    }
	    else if(strcmp(args[i], redirection[4]) == 0) { io_amper=i; }
	}

	if (io_output == 0 && io_input == 0 && io_output2 == 0 && io_pipe == 0) /* if no redirection */
	{
		if(tracing_mode){
			printf("+ ");
			int len = argsLength(args);
			int k = 0;
			for(k=0;k<len;k++){
				printf("%s", args[k]);
			}
			printf("\n");
		}

		if ((pid = fork()) < 0) {
			perror("fork");
		} else if (pid == 0) {
			if (io_amper > 0) {
				args[io_amper] = NULL;
			}
			if (execvp(args[0], args) == -1) {
				fprintf(stderr,"%s: command not found\n",args[0]);
				exit(EXIT_FAILURE);
			}
		} else {

			if (io_amper > 0) {
				;
			} else {
				do {
					pid = waitpid(pid, &status, WUNTRACED);
				} while (!WIFEXITED(status) && !WIFSIGNALED(status)); /* Wait until specified process is exited or killed */
			}
		}
	}
	else if (io_output > 0 && io_input == 0 && io_output2 == 0 && io_pipe == 0) //if '>'
	{
		if ((pid = fork()) < 0) {
			perror("fork");
		} else if (pid == 0) {
			args[io_output] = NULL;
			out = open(args[io_output + 1], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
			dup2(out, STDOUT_FILENO);
			close(out);
			if (execvp(args[0], args) == -1) {
				fprintf(stderr,"%s: command not found\n",args[0]);
			}
		} else {
			if (io_amper > 0) {
				;
			} else {
				do {
					pid = waitpid(pid, &status, WUNTRACED);
				} while (!WIFEXITED(status) && !WIFSIGNALED(status));  /* Wait until specified process is exited/killed */
			}
		}
	}
	else if (io_output == 0 && io_input > 0 && io_output2 == 0 && io_pipe == 0) //if '<'
	{
		if ((pid = fork()) < 0) {
			perror("fork");
		} else if (pid == 0) {
			args[io_input] = NULL;
			in = open(args[io_input + 1], O_RDONLY);
			dup2(in, STDIN_FILENO);
			close(in);
			if (execvp(args[0], args) == -1) {
				fprintf(stderr,"%s: command not found\n",args[0]);
			}
		} else {
			if (io_amper > 0) {
				;
			} else {
				do {
					pid = waitpid(pid, &status, WUNTRACED);
				} while (!WIFEXITED(status) && !WIFSIGNALED(status));  /* Wait until specified process is exited/killed */
			}
		}
	}
	else if (io_output == 0 && io_input == 0 && io_output2 > 0 && io_pipe == 0) //if '>>'
	{
		if ((pid = fork()) < 0) {
			perror("fork");
		} else if (pid == 0) {
			args[io_output2] = NULL;
			out = open(args[io_output2 + 1], O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
			dup2(out, STDOUT_FILENO);
			close(out);
			if (execvp(args[0], args) == -1) {
				fprintf(stderr,"%s: command not found\n",args[0]);
			}
		} else {
			if (io_amper > 0) {
				; //background, exit without waiting
			} else {
				do {
					pid = waitpid(pid, &status, WUNTRACED);
				} while (!WIFEXITED(status) && !WIFSIGNALED(status));			}
		}
	}
	else if (io_output > 0 && io_input > 0 && io_output2 == 0 && io_pipe == 0) //if '<' and '>'
	{
		if ((pid = fork()) < 0) { //error
			perror("fork");
		} else if (pid == 0) {
			args[io_output] = NULL;
			args[io_input] = NULL;
			in = open(args[io_input + 1], O_RDONLY);
			out = open(args[io_output + 1], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
			dup2(out, STDOUT_FILENO);
			dup2(in, STDIN_FILENO);
			close(in);
			close(out);
			if (execvp(args[0], args) == -1) {
				fprintf(stderr,"%s: command not found\n",args[0]);
			}
		} else {
			if (io_amper > 0) {
				; /* If Backgrounding of process, exit without waiting */
			} else {
				do {
					pid = waitpid(pid, &status, WUNTRACED);
				} while (!WIFEXITED(status) && !WIFSIGNALED(status));
			}
		}
	}
	else if (io_output == 0 && io_input > 0 && io_output2 > 0 && io_pipe == 0) //if '<' and '>>'
	{
		if ((pid = fork()) < 0) {
			perror("fork");
		} else if (pid == 0) {
			args[io_output2] = NULL;
			args[io_input] = NULL;
			in = open(args[io_input + 1], O_RDONLY);
			out = open(args[io_output2 + 1], O_WRONLY | O_APPEND | O_CREAT,S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
			dup2(out, STDOUT_FILENO);
			dup2(in, STDIN_FILENO);
			close(in);
			close(out);
			if (execvp(args[0], args) == -1) {
				fprintf(stderr,"%s: command not found\n",args[0]);
			}
		} else { //parent
			if (io_amper > 0) {
				; //background, dont wait for child process to finish
			} else {
				do {
					pid = waitpid(pid, &status, WUNTRACED);
				} while (!WIFEXITED(status) && !WIFSIGNALED(status));
			}
		}
	}
	if(io_pipe){
		create_cmdLine_pipe(args,count_pipe);
		runPipe(Head_node, count_pipe,io_amper);
	}
	return 1;
}

/*
 * Return the status code
 */
int get_status(char **args){
	int i;

	if (args[0] == NULL) {
		/* No command entered. */
	    return 1;
	}
	for (i = 0; i < number_of_builtins(); i++) {

		if (strcmp(args[0], builtin_str[i]) == 0) {

			if (tracing_mode) {
				printf("+ ");
				int len = argsLength(args);
				int k = 0;
				for (k = 0; k < len; k++) {
					printf("%s ", args[k]);
				}
				printf("\n");
			}

	      return (*builtin_func[i])(args);
	    }
	}
	return sish_launch(args);
}

/*
 * Split line into tokens
 */
char **get_args_from_command(char *line){

	int bufsize = LSH_TOK_BUFSIZE, position = 0;
	char **tokens = malloc(bufsize * sizeof(char*));
	char *token, **tokens_backup;

	if (!tokens) {
		fprintf(stderr, "sish: allocation error\n");
		exit(EXIT_FAILURE);
	}

	token = strtok(line, LSH_TOK_DELIM);
	while (token != NULL) {

		tokens[position] = token;
		position++;

		if (position >= bufsize) {
			bufsize += LSH_TOK_BUFSIZE;
			tokens_backup = tokens;
			tokens = realloc(tokens, bufsize * sizeof(char*));

			if (!tokens) {
				free(tokens_backup);
				fprintf(stderr, "sish: allocation error\n");
				exit(EXIT_FAILURE);
			}
	    }
		token = strtok(NULL, LSH_TOK_DELIM);
	  }
	  tokens[position] = NULL;
	  return tokens;
}

#define BUFSIZE 1024
char *read_line_from_shell(){
	int buffersize = BUFSIZE;
	int pointer = 0;
	char *buffer = malloc(sizeof(char) *buffersize);
	int c;

	if(!buffer){
		fprintf(stderr, "sish: allocation error\n");
		exit(EXIT_FAILURE);
	}

	while(1){

		/* Read a character */
	    c = getchar();

	    /* If EOF, replace it with a null character and return */
	    if (c == EOF || c == '\n'){
	      buffer[pointer] = '\0';
	      return buffer;
	    }else {
	      buffer[pointer] = c;
	    }
	    pointer++;

	    /* If buffer exceeds, reallocate. */
	    if (pointer >= buffersize) {
	      buffersize += BUFSIZE;
	      buffer = realloc(buffer, buffersize);
	      if (!buffer) {
	        fprintf(stderr, "sish: allocation error\n");
	        exit(EXIT_FAILURE);
	      }
	    }
	}
}

/*
 * Getting input from shell command line and executing it
 */
void loop_shell( void ){
	char *command_line;
	char **command_args;
	int status_code;

	do{
		printf("sish$ ");
		command_line = read_line_from_shell();
		command_args = get_args_from_command(command_line);
		status_code = get_status(command_args);

		free(command_line);
		free(command_args);
	}while(status_code);
}

/*
 * Main execution of simple shell
 */
int main(int argc, char **argv) {

	setprogname((char *) argv[0]);
	signal(SIGINT,SIG_IGN);
	signal(SIGINT,handle_signal);

	/* Setting the path of the executable of sish */
	char path_of_sish[1024];
	char src[50];
	char dst[50];

	bzero(src, sizeof(src));
	bzero(dst, sizeof(dst));
	bzero(path_of_sish, sizeof(path_of_sish));

	strcat(src,getprogname());
	strcat(dst,"/");
	strcat(dst,src);
	getcwd(path_of_sish,sizeof(path_of_sish));

	strcat(path_of_sish,dst);

	setenv("SHELL",path_of_sish,1);

	char ch;
	int loop_enable_shell = 1;

	/*Parse options*/
	while((ch = getopt(argc, argv, "c:x")) != -1){
		switch( ch ){
			case 'c':
				loop_enable_shell = 0;
				argv++;
				argv++;
				sish_launch(argv);
				break;
			case 'x':
				tracing_mode = 1;
				loop_enable_shell = 1;
				break;
			default:
				fprintf(stderr, "Usage: %s [-x] [-c command]\n", getprogname());
				exit(EXIT_FAILURE);
		}
	}
	if( loop_enable_shell ){
		loop_shell();
	}
	return EXIT_SUCCESS;
}