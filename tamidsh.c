#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h> 
#include <limits.h> 
#include "stockutils.c"
#include "stockutils.h"

#define EXT_SUCCESS 0
#define EXT_ERR_TOO_MANY_ARGUMENTS 1
#define EXT_ERR_SOME_ERROR 2

#define CONFIG_GLOBAL_PATH "/etc/tamidshell"
#define CONFIG_XDG_PATH "$XDG_CONFIG_HOME/tamidshell"
#define CONFIG_HOME_PATH "$HOME/tamidshell"
#define LOG_FILE_PATH "/var/log/tamid_sh.log" 

struct Config {
    bool interactive;
    bool login;
    char prompt[256]; 
    // I will add more to this section later
};

void config_init(struct Config *config) {
    config->interactive = false;
    config->login = false;
    strcpy(config->prompt, "> "); 
}

/*
Helper function to split the input strings into an array with each index being a subcommand
This will be used to help with piping
*/
int split_commands(char *input, char **commands, int max_commands) {
    int i = 0;
    char *command = strtok(input, "|"); // Splitting by pipes

    while (command != NULL && i < max_commands) {
        commands[i++] = command;
        command = strtok(NULL, "|");
    }

    return i; // Number of commands
}


void log_command(const char *command) {
    char log_file_path[PATH_MAX];
    char *home = getenv("HOME");
    if (home != NULL) {
        snprintf(log_file_path, PATH_MAX, "%s/.tamid_sh.log", home);
    } else {
        fprintf(stderr, "Error: HOME environment variable not set.\n");
        return;
    }
    
    FILE *log_file = fopen(log_file_path, "a");
    if (log_file == NULL) {
        perror("Unable to open log file");
        return;
    }
    time_t now = time(NULL);
    char *timestamp = ctime(&now);
    timestamp[strcspn(timestamp, "\n")] = 0; // Remove \n
    fprintf(log_file, "[%s] %s\n", timestamp, command);
    fclose(log_file);
}

int parse_config(struct Config *config, char* path) {
    FILE *file = fopen(path, "r");
    if (!file) {
        // Instead of printing an error, log
        log_command("Config file not found: ");
        log_command(path);
        return -1;
    }

    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || line[0] == '\n') continue; // Skip comments and empty lines
    }
    fclose(file);
    return 0;
}

int parse_args(struct Config *config, int argc, char** argv) {
    // TODO
    return 1;
}

int run_repl(struct Config *config) {
    char input[256]; // User input
    char *arr[256]; // Parsed command and arguments
    char *commands[256]; // Separate commands
    int num_commands; // Number of commands
    int pipe_fds[255][2]; // To hold the file descriptors for each pipe
    char *token;

    while (true) {
        printf("%s", config->prompt);
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        input[strcspn(input, "\n")] = 0; // Just removing \n

        // Split input for the "exit" and "cd" commands
        token = strtok(input, " ");

        // Checking if the user wants to exit
        if (token != NULL && strcmp(token, "exit") == 0) {
            log_command("exit command issued");
            break;
        }

        // Checking if user wants to change directories
        if (token != NULL && strcmp(token, "cd") == 0) {
            token = strtok(NULL, " "); // Get the path
            if (token == NULL) {
                fprintf(stderr, "cd: expected argument\n");
            } else {
                if (chdir(token) != 0) {
                    perror("cd");
                }
            }
            continue; 
        }

        num_commands = split_commands(input, commands, 256); // Used for when the user uses mulitple pipes

        // Iterating through the number of commands
        for (int i = 0; i < num_commands - 1; i++) {
            if (pipe(pipe_fds[i]) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
        }

        for (int i = 0; i < num_commands; i++) {
            pid_t pid = fork();
            if (pid == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            } else if (pid == 0) { // Child process
                if (i > 0) { // Not the first command --> get input from previous pipe
                    dup2(pipe_fds[i - 1][0], STDIN_FILENO);
                }
                if (i < num_commands - 1) { // Not the last command --> output to next pipe
                    dup2(pipe_fds[i][1], STDOUT_FILENO);
                }
                
                for (int j = 0; j < num_commands - 1; j++) { // Close
                    close(pipe_fds[j][0]);
                    close(pipe_fds[j][1]);
                }

                // Parse and execute current subcommand
                int k = 0;
                char *token = strtok(commands[i], " "); // Splitting by spaces for first command
                while (token != NULL) {
                    arr[k++] = token;
                    token = strtok(NULL, " "); // Splitting following subcommands by same " "
                }
                arr[k] = NULL;
                execvp(arr[0], arr);
                perror("execvp");
                exit(EXIT_FAILURE);
            }
        }

        // Close pipes
        for (int i = 0; i < num_commands - 1; i++) {
            close(pipe_fds[i][0]);
            close(pipe_fds[i][1]);
        }

        // Have to wait for child process to end
        for (int i = 0; i < num_commands; i++) {
            wait(NULL);
        }
    }
    return EXT_SUCCESS;
}

int main(int argc, char** argv, char** envp) {
    // Check for the log file and create it if it doesn't exist
    char log_file_path[PATH_MAX];
    char *home = getenv("HOME");
    if (home != NULL) {
        snprintf(log_file_path, PATH_MAX, "%s/.tamid_sh.log", home);
        FILE *log_file = fopen(log_file_path, "a");
        if (log_file == NULL) {
            perror("Unable to create log file");
            return EXT_ERR_SOME_ERROR;
        }
        fclose(log_file);
    } else {
        fprintf(stderr, "Error: HOME environment variable not set.\n");
        return EXT_ERR_SOME_ERROR;
    }

    if (argc != 1) {
        fprintf(stderr, "USAGE: tamidsh");
        return EXT_ERR_TOO_MANY_ARGUMENTS;
    }
    struct Config config;
    config_init(&config);
    parse_config(&config, CONFIG_GLOBAL_PATH);
    parse_config(&config, CONFIG_XDG_PATH);
    parse_config(&config, CONFIG_HOME_PATH);
    parse_args(&config, argc, argv);

    // Repl logic
    if (run_repl(&config) == 0) {
        return EXT_ERR_SOME_ERROR;
    }

    return EXT_SUCCESS;
}