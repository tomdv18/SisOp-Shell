#include "exec.h"

#define ERROR -1
#define SIZE_PIPE 2

const char *out_and_error = "&1";


void redirection(int fd_from, int fd_to);
// sets "key" with the key part of "arg"
// and null-terminates it
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  key = "KEY"
//
static void
get_environ_key(char *arg, char *key)
{
	int i;
	for (i = 0; arg[i] != '='; i++)
		key[i] = arg[i];

	key[i] = END_STRING;
}

// sets "value" with the value part of "arg"
// and null-terminates it
// "idx" should be the index in "arg" where "=" char
// resides
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  value = "value"
//
static void
get_environ_value(char *arg, char *value, int idx)
{
	size_t i, j;
	for (i = (idx + 1), j = 0; i < strlen(arg); i++, j++)
		value[j] = arg[i];

	value[j] = END_STRING;
}

// sets the environment variables received
// in the command line
//
// Hints:
// - use 'block_contains()' to
// 	get the index where the '=' is
// - 'get_environ_*()' can be useful here
static void
set_environ_vars(char **eargv, int eargc)
{
	for (int i = 0; i < eargc; i++) {
		int idx = block_contains(eargv[i], '=');

		char val[(strlen(eargv[i]) - idx) * sizeof(char)];
		char name[idx];
		get_environ_key(eargv[i], name);
		get_environ_value(eargv[i], val, idx);

		int err = setenv(name, val, 1);
		if (err != 0) {
			perror("Set environment variable error");
			exit(-1);
		}
	}
}

// opens the file in which the stdin/stdout/stderr
// flow will be redirected, and returns
// the file descriptor
//
// Find out what permissions it needs.
// Does it have to be closed after the execve(2) call?
//
// Hints:
// - if O_CREAT is used, add S_IWUSR and S_IRUSR
// 	to make it a readable normal file
static int
open_redir_fd(char *file, int flags)
{
	int fd = open(file, flags, S_IRUSR | S_IWUSR);
	if (fd == ERROR) {
		perror("Open error");
	}

	return fd;
}
// Calls dup function to redirect.
// If dups fails it closes the file that was triying to redirect

void
redirection(int fd_from, int fd_to)
{
	if (fd_to == ERROR) {
		exit(-1);
	}
	int dup_result = dup2(fd_to, fd_from);
	if (dup_result == ERROR) {
		close(fd_to);
		perror("Dup error");
		exit(-1);
	}
}


// executes a command - does not return
//
// Hint:
// - check how the 'cmd' structs are defined
// 	in types.h
// - casting could be a good option
void
exec_cmd(struct cmd *cmd)
{
	// To be used in the different cases
	struct execcmd *e;
	struct backcmd *b;
	struct execcmd *r;
	struct pipecmd *p;

	switch (cmd->type) {
	case EXEC:

		e = (struct execcmd *) cmd;
		// set temporary environment vars after fork but before execution
		set_environ_vars(e->eargv, e->eargc);
		if (execvp(e->argv[0], e->argv) == -1) {
			exit(-1);
		}
		break;

	case BACK: {
		b = (struct backcmd *) cmd;
		exec_cmd(b->c);
		break;
	}

	case REDIR: {
		// changes the input/output/stderr flow
		//
		// To check if a redirection has to be performed
		// verify if file name's length (in the execcmd struct)
		// is greater than zero
		//
		r = (struct execcmd *) cmd;

		if (strlen(r->out_file) > 0) {
			int fd_out = open_redir_fd(r->out_file,
			                           O_CLOEXEC | O_CREAT |
			                                   O_TRUNC | O_WRONLY);
			redirection(STDOUT_FILENO, fd_out);
			close(fd_out);
		}
		if (strlen(r->in_file) > 0) {
			int fd_in = open_redir_fd(r->in_file, O_CLOEXEC);
			redirection(STDIN_FILENO, fd_in);
			close(fd_in);
		}
		if (strlen(r->err_file) > 0) {
			if (strcmp(r->err_file, out_and_error) != 0) {
				int fd_err = open_redir_fd(r->err_file,
				                           O_CLOEXEC | O_CREAT |
				                                   O_TRUNC |
				                                   O_WRONLY);
				redirection(STDERR_FILENO, fd_err);
				close(fd_err);
			} else {
				if (dup2(STDOUT_FILENO, STDERR_FILENO) == ERROR) {
					perror("Dup error");
					exit(-1);
				}
			}
		}

		cmd->type = EXEC;
		exec_cmd(cmd);


		break;
	}

	case PIPE: {
		// pipes two commands


		int fd[SIZE_PIPE];
		p = (struct pipecmd *) cmd;

		if (pipe(fd) == ERROR) {
			perror("Pipe Error");
			exit(-1);
		}


		pid_t pid1 = fork();
		if (pid1 == ERROR) {
			close(fd[WRITE]);
			close(fd[READ]);
			perror("Fork Error");
			exit(-1);
		}

		if (pid1 == 0) {  // HIJO IZQ
			close(fd[READ]);
			if (dup2(fd[WRITE], STDOUT_FILENO) == ERROR) {
				close(fd[WRITE]);
				perror("Dup error");
				exit(-1);
			}
			close(fd[WRITE]);
			exec_cmd(p->leftcmd);
		}

		else {  // HIJO DER
			close(fd[WRITE]);
			pid_t pid2 = fork();
			if (pid2 == ERROR) {
				close(fd[READ]);
				perror("Fork Error");
				exit(-1);
			}

			if (pid2 == 0) {
				if (dup2(fd[READ], READ) == ERROR) {
					close(fd[READ]);
					perror("Dup error");
					exit(-1);
				}
				close(fd[READ]);
				exec_cmd(p->rightcmd);

			} else {
				close(fd[READ]);
				wait(NULL);
				wait(NULL);
				exit(0);
			}
		}

		// free the memory allocated for the pipe tree structure
		// free_command(parsed_pipe);		//Tira doble free


		break;
	}
	}
}
