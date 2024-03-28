#include "builtin.h"

// returns true if the 'exit' call
// should be performed
//
// (It must not be called from here)
int
exit_shell(char *cmd)
{
	return cmd[0] == 'e' && cmd[1] == 'x' && cmd[2] == 'i' &&
	       cmd[3] == 't' && cmd[4] == '\0';
}

// returns true if "chdir" was performed
//  this means that if 'cmd' contains:
// 	1. $ cd directory (change to 'directory')
// 	2. $ cd (change to $HOME)
//  it has to be executed and then return true
//
//  Remember to update the 'prompt' with the
//  	new directory.
//
// Examples:
//  1. cmd = ['c','d', ' ', '/', 'b', 'i', 'n', '\0']
//  2. cmd = ['c','d', '\0']
int
cd(char *cmd)
{
	char *dir;
	if (strcmp(cmd, "cd") == 0 || strcmp(cmd, "cd ") == 0) {
		dir = getenv("HOME");
	} else if (strncmp(cmd, "cd ", 3) == 0) {
		dir = cmd + 3;
	} else {
		return 0;
	}
	if (chdir(dir) < 0) {
		perror("Error: no existe o no se encuentra el directorio");
	} else {
		char buf[PRMTLEN];
		snprintf(prompt, sizeof prompt, "(%s)", getcwd(buf, PRMTLEN));
	}
	return true;
}

// returns true if 'pwd' was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
pwd(char *cmd)
{
	if (strcmp(cmd, "pwd") != 0)
		return 0;

	char buf[PRMTLEN];
	char *getcwd_result = getcwd(buf, PRMTLEN);

	if (getcwd_result == NULL) {
		perror("Error en [getcwd]");
	} else {
		printf("%s\n", getcwd_result);
	}
	return true;
}

// returns true if `history` was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
history(char *cmd)
{
	int a = strcmp(cmd, "") * 0;  // para que compile sin warnings
	return a;
}
