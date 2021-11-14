#include "commands.h"

/**
 * Project1.c contains all of the given code to parse commands
 * It also contains the main loop where commands are executed
 * The main loop checks for built in commands like exit jobs and bg
 * The code for executing foreground commands is in commands.c
 * The code for executing background commands is in the main loop
 * After the command is read and executed the loop checks for any background commands
 * that might have finished and prints out a done statment if there are any
 */ 

/* The process of the currently executing foreground command, or 0. */
pid_t foregroundPid = 0;

//Jobs array to store all current Jobs
job* Jobs;

/* Parses the command string contained in cmd->line.
 * * Assumes all fields in cmd (except cmd->line) are initailized to zero.
 * * On return, all fields of cmd are appropriatly populated. */
void parseCmd(Cmd* cmd) {
	char* token;
	int i=0;
	strcpy(cmd->tokenLine, cmd->line);
	strtok(cmd->tokenLine, "\n");
	token = strtok(cmd->tokenLine, " ");
	while (token != NULL) {
		if (*token == '\n') {
			cmd->args[i] = NULL;
		} else if (*token == REDIRECT_OUT_OP || *token == REDIRECT_IN_OP
				|| *token == PIPE_OP || *token == BG_OP) {
			cmd->symbols[i] = token;
			cmd->args[i] = NULL;
		} else {
			cmd->args[i] = token;
		}
		token = strtok(NULL, " ");
		i++;
	}
	cmd->args[i] = NULL;
}

/* Finds the index of the first occurance of symbol in cmd->symbols.
 * * Returns -1 if not found. */
int findSymbol(Cmd* cmd, char symbol) {
	for (int i = 0; i < MAX_ARGS; i++) {
		if (cmd->symbols[i] && *cmd->symbols[i] == symbol) {
			return i;
		}
	}
	return -1;
}

/* Signal handler for SIGTSTP (SIGnal - Terminal SToP),
 * which is caused by the user pressing control+z. */
void sigtstpHandler(int sig_num) {
	/* Reset handler to catch next SIGTSTP. */
	signal(SIGTSTP, sigtstpHandler);
	if (foregroundPid > 0) {
		/* Foward SIGTSTP to the currently running foreground process. */
		kill(foregroundPid, SIGTSTP);
		int index;
		if((index = findJobByPID(Jobs,foregroundPid))!=-1){
			Jobs[index].state=2;
			printJob(Jobs,index);
		}
	}
}

int main(void) {
	/* Listen for control+z (suspend process). */
	signal(SIGTSTP, sigtstpHandler);
	
	Jobs = (job*) malloc(MAX_JOBS*sizeof(job));
	initjobs(Jobs);
	int nextJid=1;
	while (1) {
		printf("352> ");
		fflush(stdout);
		Cmd *cmd = (Cmd*) calloc(1, sizeof(Cmd));
		fgets(cmd->line, MAX_LINE, stdin);
		parseCmd(cmd);
		if (!cmd->args[0]) {
			free(cmd);
		} else if (strcmp(cmd->args[0], "exit") == 0) {
			free(cmd);
			exit(0);
		} else if (strcmp(cmd->args[0], "jobs") == 0) {
			int i=0;
			for(i=0;i<MAX_JOBS;i++)
			{
				if(Jobs[i].pid!=0)
				{
					printJob(Jobs,i);
				}
			}
		} else if (strcmp(cmd->args[0], "bg") == 0) {
			int jobId=0;
			pid_t pid;
			//These if statements check for incorrect usage of bg
			if(cmd->args[2])
				printf("bg usage: bg (jobid)\n");
			else if(cmd->args[1]==NULL)
				printf("bg usage: bg (jobid)\n");
			else {
				jobId=atoi(cmd->args[1]);
				int index = findJobByJID(Jobs,jobId);
				pid=Jobs[index].pid;
				if(pid<0)
				{
					printf("No such job");
					continue;
				}
				kill(pid,SIGCONT);
				Jobs[index].state=4;
				printJob(Jobs,index);
			}
		} else {
			if (findSymbol(cmd, BG_OP) != -1) {
				cmd->pid=fork();
  				int status=0;
  				if(cmd->pid==-1){
    				printf("fork failed");
  				}
  				else if(cmd->pid==0)
  				{
					int inop=findSymbol(cmd,REDIRECT_IN_OP);
    				int outop=findSymbol(cmd,REDIRECT_OUT_OP);
					//Checks for file redirection in the background process
    				if(inop!=-1)
    				{
      					int fd0 = open(cmd->args[inop+1], O_RDONLY);
      					dup2(fd0, STDIN_FILENO);
      					close(fd0);
    				}
    				else if(outop!=-1)
    				{
      					int fd1 = creat(cmd->args[outop+1] , 0644) ;
      					dup2(fd1, STDOUT_FILENO);
      					close(fd1);
    				}

	 				if (execvp(cmd->args[0],cmd->args) == -1){
		  				perror("bad command");
						exit(-1);
						deletejob(Jobs,cmd->pid,&nextJid);
					 }
  				}
  				else
  				{
					addjob(Jobs,1,cmd,&nextJid);
					int index = findJobByJID(Jobs,nextJid-1);
					//Prints the JID and PID after adding the new process to the Job array
					printJobIdPid(Jobs,index);
					if (setpgid(cmd->pid, 0) != 0) perror("setpgid() error");
    				waitpid(cmd->pid,&status,WNOHANG);
			  	}

			} else {
				//Checks for a pipe symbol in the command
				if(findSymbol(cmd,PIPE_OP)!=-1){
					execPiped(cmd,&foregroundPid);
				//Checks for a file redirection symbol in the command
				}else if(findSymbol(cmd,REDIRECT_OUT_OP)!=-1 || findSymbol(cmd,REDIRECT_IN_OP)!=-1){
					redirection(cmd,&foregroundPid,Jobs,&nextJid);
				}else{
					//Executes a simple command
					findCommand(cmd,&foregroundPid,Jobs,&nextJid);
				}
			}
		}

		pid_t pid;
		int status;
		while((pid=waitpid(-1, &status, WNOHANG|WUNTRACED))>0)
		{
			if (WIFEXITED(status)) {
				int i;
				int exitStatus = WEXITSTATUS(status);
				//Finds complete background jobs and prints a done statement and deletes the job
				if((i=findJobByPID(Jobs,pid))!=-1){
					//Checks for a non-zero exit code and prints it out with the command
					if(exitStatus>0){
						printf("[%d] Exit %d %s",Jobs[i].jid,exitStatus,Jobs[i].line);
					}else{
						Jobs[i].state=3;
						printJob(Jobs,i);
					}
					deletejob(Jobs,pid,&nextJid);
				}
				else{
					printf("No Job Found for pid= %d\n",pid);
				}
        	} else if (WIFSTOPPED(status)) {
            	int i;
				if((i=findJobByPID(Jobs,pid))!=-1){
					Jobs[i].state=2;
					printJob(Jobs,i);
				}
        	} else if(WIFSIGNALED(status)) {
				int i;
				//If the process is terminated prints something and deletes the job
				if((i=findJobByPID(Jobs,pid))!=-1){
					printf("[%d] Terminated %s",Jobs[i].jid,Jobs[i].line);
				}
				deletejob(Jobs,pid,&nextJid);
			}
		}

	}
	free(Jobs);
	return 0;
}