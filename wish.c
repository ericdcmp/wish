#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

void interactive(void);
void batch(char * file);
int execute_command(char *args[], char * filename);
void tokenize(char * input, char* args[]);
int handleredirect(char * input);
void runParrallel(char* input);
static char error_message[30] = "An error has occurred\n";
static char * path[15];

int main(int argc, char *argv[]) {
	path[0] = "/bin";
	if(argc == 1) interactive();
	else if(argc == 2) batch(argv[1]);
	else if(argc > 2) {
		write(STDERR_FILENO, error_message, strlen(error_message));
		exit(1);
	}
	else write(STDERR_FILENO, error_message, strlen(error_message));
	return 0;
}

void interactive(void) {
	// Interactive mode
	int i;
	while(1) {
		char * input = (char *) calloc(128,sizeof(char));
        	size_t size = 128;
		printf("wish> ");
		fflush(stdout);
		i = getline(&input, &size, stdin);
		if(i == -1) {
			break;
		}
		if(strcmp(input, "\n") == 0) {
			continue;
		}
		if(strchr(input, '&') != NULL) {
                        runParrallel(input);
                        continue;
                }
		if(strchr(input, '>') != NULL) {
			handleredirect(input);
			continue;
		}
		// Initialize command input array
		char* args[20];
		tokenize(input, args);
		// Built in command exit
		execute_command(args, NULL);
	}
}

void batch(char * file) {
	FILE * fp = fopen(file, "r");
	if(fp == NULL) {
		write(STDERR_FILENO, error_message, strlen(error_message));
		exit(1);
	}

	char * input = (char *) calloc(128,sizeof(char));
	size_t size = 128;

	while( getline(&input, &size, fp) > -1) {
		// Initialize command input array
		if(strchr(input, '&') != NULL) {
                        runParrallel(input);
                        continue;
                }
		if(strchr(input, '>') != NULL) {
			handleredirect(input);
			continue;
		}
		char* args[10];
		tokenize(input, args);
		execute_command(args, NULL);
	}
	fclose(fp);
}

int execute_command(char *args[], char * filename) {
	// Built in command exit
	if(args[0] == NULL) return 1;

	if(strcmp(args[0], "exit") == 0) {
		if(args[1] != NULL) {
			write(STDERR_FILENO, error_message, strlen(error_message));
			return 1;
		}
		else {
		exit(0);
		}
	}
	// Built in command cd
	if(strcmp(args[0], "cd") == 0) {
		if(args[1] == NULL || args[2] != NULL) {
			write(STDERR_FILENO, error_message, strlen(error_message));
			return 1;
		}
		int chd = chdir(args[1]);
		if(chd != 0) {
			write(STDERR_FILENO, error_message, strlen(error_message));
		}
		return 1;
	}
	if(strcmp(args[0], "path") == 0) {
		if(args[1] == NULL) {
			for(int i = 0 ;i < 15 ; i++) {
				path[i] = NULL;
			}
		}
		else {
			for(int i = 1 ; args[i] != NULL ; i++ ) {
				path[i-1] = args[i];
				//printf("%s\n", path[i-1]);
			}
		}
		return 1;
	}
	if(path[0] == NULL) {return 0;}
	int pc = fork();

	// Fork process failed
	if(pc < 0) {
		exit(1);
	}
	// Execute command
	if(pc == 0) {
		if(filename != NULL) {
			FILE * fp = freopen(filename, "w+", stdout);
			if(fp == NULL) {
				write(STDERR_FILENO, error_message, strlen(error_message));
				return 1;
			}
		}
		for(int i = 0 ; path[i] != NULL ; i++) {
			char *command = args[0];
			char * abspath = (char *) calloc(128,sizeof(char));
			strcat(abspath, path[i]);
			strcat(abspath, "/");
			strcat(abspath, command);
			if((access(abspath, X_OK)) == 0) {
				execv(abspath, args);
			}

		}
		write(STDERR_FILENO, error_message, strlen(error_message));
		exit(0);
	}

	else {
		int wc = wait(NULL);
		return wc;
	}
}

void tokenize(char * input, char* args[]) {
	int i = 0;
	char * token = strtok(input, " \n\t");
	while(token != NULL) {
		args[i] = token;
		token = strtok(NULL, " \n\t");
		i++;
	}
	args[i] = NULL;
}

int handleredirect(char * input) {

	char *args[10];
	char * token = strtok(input, ">");
	args[0] = token;
	char * filename = strtok(NULL, "> \n");
	if(filename == NULL) {
	write(STDERR_FILENO, error_message, strlen(error_message));
	return 1;
	}
	char * extra = strtok(NULL, ">");
	if(extra != NULL) {
		write(STDERR_FILENO, error_message, strlen(error_message));
		return 1;
	}
	char * remainingtokens = strtok(NULL, "> \n");
	if(remainingtokens != NULL) {
		write(STDERR_FILENO, error_message, strlen(error_message));
		return 1;
	}
	tokenize(args[0], args);

	execute_command(args, filename);
	return 0;
}

void runParrallel(char* string) {
	char * args[10];
	char * rest = string;
	char * token;
	while((token = strtok_r(rest, "&\n", &rest))) {
			//char * args[10];
			if(strchr(token, '>') != NULL) {
                                handleredirect(token);
                                continue;
                        }
			char * command = token;
			tokenize(command, args);
			execute_command(args, NULL);
	}

}
