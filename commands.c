#include "commands.h"

/**
 * This File contains the implementation for all kinds of foreground commands
 * The foreground commands range from simple commands to file redirection and to piped instructions
 * The file then contains all of the helper functions in setting up and accessing the Job array
 */


/**
 * Finds and executes simple commands in the foreground
 */ 
void findCommand(Cmd* cmd, pid_t* fpid,job* Jobs,int* nextJid)
{
  *fpid = fork();
	cmd->pid=*fpid;
  int status=0;
  addjob(Jobs,1,cmd,nextJid);
  if(*fpid==-1){
    printf("fork failed");
    return;
  }
  else if(*fpid==0)
  {
	  if (execvp(cmd->args[0],cmd->args) == -1){
		  perror("bad command");
      deletejob(Jobs,cmd->pid,nextJid);
      exit(-1);
    }
  }
  else
  {
    waitpid(*fpid,&status,WUNTRACED);
    //If the child isn't given a stop command it deletes the job
    if(!WIFSTOPPED(status))
      deletejob(Jobs,cmd->pid,nextJid);
    return;
  }
}

/**
 * Executes commands with redirection
 */ 
void redirection(Cmd* cmd, pid_t* fpid,job* Jobs,int* nextJid)
{
  *fpid = fork();
	cmd->pid=*fpid;
  int status=0;
  addjob(Jobs,1,cmd,nextJid);
  if(*fpid==-1){
    printf("fork failed");
    return;
  }
  else if(*fpid==0)
  {
    int inop=findSymbol(cmd,REDIRECT_IN_OP);
    int outop=findSymbol(cmd,REDIRECT_OUT_OP);
    if(inop!=-1)
    {
      int fd0 = open(cmd->args[inop+1], O_RDONLY);
      dup2(fd0, STDIN_FILENO);
      close(fd0);
    }
    else
    {
      int fd1 = creat(cmd->args[outop+1] , 0644) ;
      dup2(fd1, STDOUT_FILENO);
      close(fd1);
    }
	  if (execvp(cmd->args[0],cmd->args) == -1){
		  perror("bad command");
      deletejob(Jobs,cmd->pid,nextJid);
      exit(-1);
    }
  }
  else
  {
    waitpid(*fpid,&status,WUNTRACED);
    //If the child isn't given a stop command it deletes the job
    if(!WIFSTOPPED(status))
      deletejob(Jobs,cmd->pid,nextJid);
    return;
  }
}

/**
 * Executes piped commands
 */ 
void execPiped(Cmd* cmd,pid_t* fpid)
{
  int pipefd[2];
  pid_t pid2;

  //Saves the location of the pipe in the args for later use
  int pipeLoc=0;
  pipeLoc=findSymbol(cmd,PIPE_OP);
  char* cmd1[MAX_LINE];
  char* cmd2[MAX_LINE];
  int i=0;
  //Copy the command and switches of the first command
  while(cmd->args[i])
  {
    cmd1[i]=NULL;
    cmd1[i]=cmd->args[i];
    i++;
  }
  cmd1[i]=NULL;
  i++;
  //Copy the command and switches of the second command
  while(cmd->args[i])
  {
    cmd1[i]=NULL;
    cmd2[(i-1)-pipeLoc]=cmd->args[i];
    i++;
  }
  cmd2[(i-1)-pipeLoc]=NULL;

  if(pipe(pipefd)<0)
  {
    printf("Pipe couldn't be initialized");
    return;
  }

  *fpid = fork();
  cmd->pid=getpid();

  if(*fpid == -1)
  {
    printf("fork failed");
    return;
  }
  
  //forks for the first command
  if(*fpid==0) {
    //Sets up the pipe
    close(pipefd[0]); 
    dup2(pipefd[1], STDOUT_FILENO); 
    close(pipefd[1]);
    if (execvp(cmd1[0], cmd1) < 0) { 
      printf("\nCould not execute command 1.."); 
      exit(0); 
    }
  } else {
    pid2=fork();

    if(pid2 == -1)
    {
      printf("fork failed");
      return;
    }

    //forks for the second command
    if(pid2==0) {
    //Sets up the other end of the pipe
    close(pipefd[1]); 
    dup2(pipefd[0], STDIN_FILENO); 
    close(pipefd[0]);
      if (execvp(cmd2[0], cmd2) < 0) { 
        printf("\nCould not execute command 1.."); 
        exit(0); 
      }    
    } else {
      //Waits for both children to complete
      wait(NULL);
      return;
    }
  }
  
}

/**
 * Adds a job to the list
 */ 
int addjob(job *jobs, int state, Cmd *cmd,int* nextJid) 
{
  int i;

  if (cmd->pid < 1)
    return 0;

  for (i = 0; i < MAX_JOBS; i++) {
    //Finds an empty job spot in the array to add a new job
    if (jobs[i].pid == 0) {
      jobs[i].pid = cmd->pid;
      strcpy(jobs[i].line,cmd->line);
      jobs[i].state = state;
      jobs[i].jid = *nextJid;
      *nextJid= *nextJid+1;
      if (*nextJid > MAX_JOBS)
        *nextJid = 1;
      return 1;
    }
  }
  printf("Tried to create too many jobs\n");
  return 0;
}

/* Deletes a job whose PID=pid from the job list */
int deletejob(job *jobs, pid_t pid, int* nextJid) 
{
  int i;

  if (pid < 1)
    return 0;

  for (i = 0; i < MAX_JOBS; i++) {
    if (jobs[i].pid == pid) {
      clearjob(&jobs[i]);
      *nextJid = maxJid(jobs)+1;
      return 1;
    }
  }
  return 0;
}

/**
 * Clears a job by setting all values
 * to 0 or an equivalent
 */ 
void clearjob(job *job) {
  job->pid = 0;
  job->jid = 0;
  job->state = 0;
  strcpy(job->line,"");
}

/**
 * Initializes the jobs array by clearing each
 * spot in the array
 */ 
void initjobs(job *jobs) {
  int i;

  for (i = 0; i < MAX_JOBS; i++)
    clearjob(&jobs[i]);
}

//Finds the max Job Id currently in use
int maxJid(job *jobs)
{
	int i,max=0;
	for (i=0;i<MAX_JOBS;i++)
	{
		if(jobs[i].jid>max)
			max=jobs[i].jid;
	}
	return max;
}

//Finds a job by using the Job Id to search for it
int findJobByJID(job *jobs,int jid)
{
  for(int i=0;i<MAX_JOBS;i++)
  {
    if(jobs[i].jid==jid)
      return i;
  }
  return -1;
}

//Finds a job by using the Job's PID to search for it
int findJobByPID(job *jobs,pid_t pid)
{
  for(int i=0;i<MAX_JOBS;i++)
  {
    if(jobs[i].pid==pid)
      return i;
  }
  return -1;
}

/* Prints out a singular job
 */ 
void printJob(job* job, int index)
{
	char *status;
	if(job[index].state==1)	status="Running";
	else if(job[index].state==2)	status="Stopped";
	else if(job[index].state==3)	status="Done";
	else if(job[index].state==4)	status="Running";
	else	status="undefined";
	printf("[%d]	%s		%s",job[index].jid,status,job[index].line);
}

//Prints jobid and pid for background command start
void printJobIdPid(job* job, int index)
{
	printf("[%d] %d\n",job[index].jid, job[index].pid);
}