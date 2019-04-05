#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdbool.h>
#include <time.h>

#include "tokenizer.h"
#include "path_token.h"

/* Convenience macro to silence compiler warnings about unused function parameters. */
#define unused __attribute__((unused))

/* Max length of path */
#define MAX_PATH 255

/* Whether the shell is connected to an actual terminal or not. */
bool shell_is_interactive;

/* File descriptor for the shell input */
int shell_terminal;

/* Terminal mode settings for the shell */
struct termios shell_tmodes;

/* Process group id for the shell */
pid_t shell_pgid;

int cmd_exit(struct tokens *tokens);
int cmd_help(struct tokens *tokens);
int cmd_cd(struct tokens *tokens);
int cmd_pwd(struct tokens *tokens);
int cmd_wait(struct tokens *tokens);

/* Built-in command functions take token array (see parse.h) and return int */
typedef int cmd_fun_t(struct tokens *tokens);

/* Built-in command struct and lookup table */
typedef struct fun_desc {
  cmd_fun_t *fun;
  char *cmd;
  char *doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
  {cmd_help, "?", "show this help menu"},
  {cmd_exit, "exit", "exit the command shell"},
  {cmd_cd, "cd", "change directory"},
  {cmd_pwd, "pwd", "prints the current working directory"},
	{cmd_wait, "wait", "waits until all background jobs have terminated"},
};
/* Prints a helpful description for the given command */
int cmd_help(unused struct tokens *tokens) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
  return 1;
}

/* Exits this shell */
int cmd_exit(unused struct tokens *tokens) {
  exit(0);
}

/* Changes the current working directory to the given directory. */
int cmd_cd(unused struct tokens *tokens){
  if (chdir(tokens->tokens[1]) == -1) {
    printf("%s : %s\n", tokens->tokens[0], strerror(errno));
	return 1;
  }
  return 0;
}

/* Prints the current working directory */
int cmd_pwd(unused struct tokens *tokens) {
  char* cwd = (char*) malloc(MAX_PATH);
  getcwd(cwd, MAX_PATH);
  if (cwd != NULL) {
  	  printf("%s\n", cwd);
      return 0;
	  }				  
  return 1;
}

/**
	 * Waits until all background jobs have terminated
	  */
int cmd_wait(unused struct tokens *tokens) {
  int status;
	pid_t pid;
	while ((pid = waitpid(-1, &status, 0)) > 0) {
	  if (WIFEXITED(status)) {
		  printf("child [%d] has terminted with exit status = %d\n", pid, WEXITSTATUS(status));
		} else {
		  printf("child [%d] terminated with error\n", pid);
		}
	}
	  if (errno != ECHILD) {
	  fprintf(stderr, "waitpid error\n");
	}
	return 0;
}

/* IO redirection */
int io_redirect(unused struct tokens *tokens) {
  int fd;
  int in_redir = is_direct_tok(tokens, "<");
  int out_redir = is_direct_tok(tokens, ">");
  if (in_redir != 0) {
	tokens->tokens[in_redir] = NULL;
    if (tokens->tokens[in_redir + 1] == NULL ||
		strcmp(tokens->tokens[in_redir + 1], ">") == 0 ||
		strcmp(tokens->tokens[in_redir + 1], "<") == 0){
	  fprintf(stderr, "%s : Syntax error.\n", tokens->tokens[0]);
	  return -1;
	} 
	if ((fd = open(tokens->tokens[in_redir + 1], O_RDONLY, 0)) < 0) {
	  fprintf(stderr, "%s :No such file or derectory.\n", tokens->tokens[in_redir + 1]);
	  return -1;
	}
    dup2(fd, STDIN_FILENO);
  return 0;
  }
  if (out_redir != 0) {
	tokens->tokens[out_redir] = NULL;
    if (tokens->tokens[out_redir + 1] == NULL ||
		strcmp(tokens->tokens[out_redir + 1], ">") == 0 ||
		strcmp(tokens->tokens[out_redir + 1], "<") == 0){
	  fprintf(stderr, "%s : Syntax error.\n", tokens->tokens[0]);
	  return -1;
	} 
	if ((fd = open(tokens->tokens[out_redir + 1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) {
	  fprintf(stderr, "%s :No such file or derectory.\n", tokens->tokens[out_redir + 1]);
	  return -1;
	}
    dup2(fd, STDOUT_FILENO);
  return 0;
 }
 else{
   return 0;
 }
}
/* undo the default singal action for child job */
void undo_signal() { 
  signal(SIGINT, SIG_DFL);
  signal(SIGQUIT, SIG_DFL);
  signal(SIGTSTP, SIG_DFL);
  signal(SIGTTIN, SIG_DFL);
  signal(SIGTTOU, SIG_DFL);
}

/* Looks up the built-in command, if it exists. */
int lookup(char cmd[]) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0))
      return i;
  return -1;
}

/* Intialization procedures for this shell */
void init_shell() {
  /* Our shell is connected to standard input. */
  shell_terminal = STDIN_FILENO;

  /* Check if we are running interactively */
  shell_is_interactive = isatty(shell_terminal);

  if (shell_is_interactive) {
    /* If the shell is not currently in the foreground, we must pause the shell until it becomes a
     * foreground process. We use SIGTTIN to pause the shell. When the shell gets moved to the
     * foreground, we'll receive a SIGCONT. */
    while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
      kill(-shell_pgid, SIGTTIN);

    /* Ignore interactive and job-control signals */
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);

    /* Saves the shell's process id */
    shell_pgid = getpid();
		/* Put shell in its own process group */
		if (setpgid(shell_pgid, shell_pgid) < 0) {
	    fprintf(stderr, "Couldn’t put the shell in its own process group");
		  exit(1);
		}
	
    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* Save the current termios to a variable, so it can be restored later. */
    tcgetattr(shell_terminal, &shell_tmodes);
  }
}

int main(unused int argc, unused char *argv[]) {
  init_shell();

  static char line[4096];
  int line_num = 0;

  /* Please only print shell prompts when standard input is not a tty */
  if (shell_is_interactive)
    fprintf(stdout, "%d: ", line_num);

  while (fgets(line, 4096, stdin)) {
    /* Split our line into words. */
    struct tokens *tokens = tokenize(line);
		bool is_bg = is_bg_tok(tokens);

    /* Find which built-in function to run. */
    int fundex = lookup(tokens_get_token(tokens, 0));

    if (fundex >= 0) {
      cmd_table[fundex].fun(tokens);
    } else {
      /* REPLACE this to run commands as programs. */
//fprintf(stdout, "This shell doesn't know how to run programs.\n");
	  /* Execute programs that isn't a built-in command. */
	  pid_t pid = fork();
	  if (pid == 0) {
		if (io_redirect(tokens) < 0) {
		  exit(0);
		}
        // It seems that the first string in the second parameter will be ignored.
		char *path_token = NULL;
		struct path_tokens *path_tokens = (struct path_tokens*) malloc(sizeof(struct path_tokens));  
		init_pathtoks(path_tokens);
		path_token = next_path_token(path_tokens);//返回分割后带'/'的单个PATH
		while (path_token != NULL) {
	      path_token = strcat(path_token, tokens->tokens[0]);	
				undo_signal();
	      if (execv(path_token, tokens->tokens) < 0) {    
		    path_token = next_path_token(path_tokens);
		  }else{
	        path_tokens_destroy(path_tokens);
		    exit(0);
		  }
	    }
        fprintf(stderr, "%s : Command not found\n", tokens->tokens[0]);
	    path_tokens_destroy(path_tokens);
	 	exit(0);
	  }else{
			if(!is_bg) {
				int child_status;
				if (waitpid(pid, &child_status, 0) < 0) {
			    fprintf(stderr, "waitpid error\n");
				}
			}
	   }
    }

    if (shell_is_interactive)
      /* Please only print shell prompts when standard input is not a tty */
      fprintf(stdout, "%d: ", ++line_num);

    /* Clean up memory */
    tokens_destroy(tokens);
  }
  return 0;
}
