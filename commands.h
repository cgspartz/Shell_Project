
#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_LINE 80
#define MAX_ARGS (MAX_LINE/2 + 1)
#define MAX_JOBS 50
#define REDIRECT_OUT_OP '>'
#define REDIRECT_IN_OP '<'
#define PIPE_OP '|'
#define BG_OP '&'

/* Holds a single command. */
typedef struct Cmd {
	/* The command as input by the user. */
	char line[MAX_LINE + 1];
	/* The command as null terminated tokens. */
	char tokenLine[MAX_LINE + 1];
	/* Pointers to each argument in tokenLine, non-arguments are NULL. */
	char* args[MAX_ARGS];
	/* Pointers to each symbol in tokenLine, non-symbols are NULL. */
	char* symbols[MAX_ARGS];
	/* The process id of the executing command. */
	pid_t pid;

	// int jobId;
	// int state;
	/* TODO: Additional fields may be helpful. */

} Cmd;

typedef struct job {
    int jid;
    pid_t pid;
	//Holds the input from the user
	char line[MAX_LINE+1];
	// 1==Running
	// 2==Stopped
	// 3==Done
	// 4==stopped and restarted
    int state;
} job;

void findCommand(Cmd* cmd,pid_t* fpid,job* Jobs,int* nextJid);
void redirection(Cmd* cmd, pid_t* fpid,job* Jobs,int* nextJid);
void execPiped(Cmd* cmd,pid_t* fpid);
int findSymbol(Cmd* cmd, char symbol);

int addjob(job *jobs, int state, Cmd *cmd,int* nextJid);
int deletejob(job *jobs, pid_t pid, int* nextJid);
void clearjob(job *job);
void initjobs(job *jobs);
int maxJid(job *jobs);
int findJobByJID(job *jobs,int jid);
int findJobByPID(job *jobs,pid_t pid);
void printJob(job* job, int index);
void printJobIdPid(job* job, int index);

#endif