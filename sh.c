#include "defs.h"
#include "types.h"
#include "readline.h"
#include "runcmd.h"

#define STACK_SIZE 8192

char prompt[PRMTLEN] = { 0 };


static void
sigchldHandler()
{
	pid_t p;
	if ((p = waitpid(0, NULL, WNOHANG)) > 0) {
		;
		char message[100];
		snprintf(message,
		         sizeof(message),
		         "%s==> terminado: PID: %d %s\n",
		         COLOR_BLUE,
		         p,
		         COLOR_RESET);
		if (write(STDOUT_FILENO, message, strlen(message)) == -1)
			perror("Write Error");
	}
}

// runs a shell command
static void
run_shell()
{
	char *cmd;

	while ((cmd = read_line(prompt)) != NULL)
		if (run_cmd(cmd) == EXIT_SHELL)
			return;
}

// initializes the shell
// with the "HOME" directory
static void
init_shell()
{
	char buf[BUFLEN] = { 0 };
	char *home = getenv("HOME");

	if (chdir(home) < 0) {
		snprintf(buf, sizeof buf, "cannot cd to %s ", home);
		perror(buf);
	} else {
		snprintf(prompt, sizeof prompt, "(%s)", home);
	}
}

int
main(void)
{
	init_shell();


	struct sigaction sa;
	stack_t ss;

	ss.ss_sp = malloc(STACK_SIZE);
	if (ss.ss_sp == NULL) {
		return 1;
	}

	ss.ss_size = STACK_SIZE;
	ss.ss_flags = 0;

	sa.sa_flags = SA_RESTART | SA_NOCLDSTOP | SA_ONSTACK;
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = sigchldHandler;

	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		free(ss.ss_sp);
		return 1;
	}


	run_shell();

	free(ss.ss_sp);

	return 0;
}
