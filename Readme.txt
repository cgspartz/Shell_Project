Project 1
Christopher Spartz

Source Files:

Project1.c:
This file contains most of the code provided in the starter code
and the main loop that executes all commands

Commands.c:
This file contains the implementation of foregrounds commands like:
Simple commands
Commands containing file redirection
Piped commands

This file also contains helper functions for the Job array like:
Adding new jobs
Deleting jobs
Initializing the job array
Clearing out a single job
Finding the max Job ID
Finding a job by its JID or PID
Printing jobs

Commmands.h:
This file contains all of the include statements necessary for both Files
It also includes the define declarations included in the starter code
This file has the declarations of both the cmd struct and the job struct
Finally this file contains all of the function declarations so that the functions can be used in either file