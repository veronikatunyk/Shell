#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#define HISTORY_SIZE 100
#define MAX 1024
#define MAX_ARG 100
char* history[HISTORY_SIZE]; //array of strings used to store all commands
int head = 0; //the head of the circular array stores the oldest command
int cmdNum = 0; //track number of commands within history

//prototypes
void executePipe();
int parse();
void runCmd();


//function that increments history when a new command is added. If the buffer is full, it will wrap around and override oldest command.
void history_sum(const char* newCmd) {
if (cmdNum < HISTORY_SIZE) { //check if buffer is full...

history[cmdNum] = strdup(newCmd); //strdup allocates memory to command so we do not override previous line (must allocate memory in fgets())
cmdNum++;
}
else { //if history array is full, we have to override oldest command
// char* freedCmd = strdup(history[head]); //we allocate memory to the oldest command so we do not override previous "oldest" command

free(history[head]); //free oldest command
//free(freedCmd); //as we dynamically added freeCmd via strdup, we have to free it
history[head] = strdup(newCmd); //override oldest command, with our new command
head = (head+1) % HISTORY_SIZE; //traversal in circular array requires mod
 //printf("New head after overwrite: %d\n", head);
}
 
}
void exit_cmd() {
printf("Session stopped\n");
   exit(0);
}


void cd(char **cmd) {
    if (cmd[1] != NULL) {
	  if (chdir(cmd[1]) != 0) {
	       perror("chdir failed in cd");
	     }
	}
	else
	printf("cd failed: insert argument\n");

}

void history_clear() {
for (int i = 0; i < cmdNum; i++) {
free(history[i]); //when we use free, even if memory is freed, pointer is not
history[i] = NULL; //free up the pointer
}
cmdNum = 0; // Reset command count
head = 0; // Reset head to start position
}

void history_noarg() {
int index = 0;
for (int i = 0; i < cmdNum; i++) {
        index = (head + i) % HISTORY_SIZE; //the history starts from the oldest command (head), and then increases by i by wrapping around
        if (history[index] != NULL) //we only print the space that is not full
        printf("%d: %s\n", i, history[index]);
        
    }
 
}

void history_offset(const char* num, const char* line) { //we take in a string argument that represents a offset number
int userOffset = atoi(num); //using atoi, we can turn the string representation into a int
int actualOffset = (head + userOffset) % HISTORY_SIZE; 
//if we have to wrap around, what is stored is different than what is displayed from history command
if (actualOffset >= cmdNum || actualOffset < 0 ) {
printf("offset not valid");
return;
}
else {
char *argv[MAX_ARG]; //create new array for command line to go
char* cmdCopy = strdup(history[actualOffset]); //we store the command at offset that we have to execute
//strdup must be used so no changes occur to the original command, without it, unexpected parse occured.
int cmdNum = parse(cmdCopy, argv);
        
    if (strcmp(argv[0], "history") == 0) {
        history_noarg();
        
        }
		if (strcmp(argv[0], "cd") == 0) {
       cd(argv);
       }   
    
		else if(cmdNum <= 1 && *argv != NULL) {		//if there is no piping, run the entered command
   //printf("from offset history, the command we have to execute is done through execvp\n");
			runCmd(argv);
		}
free(cmdCopy);   
 history_sum(line);  
}
}








int main() {
	//int pid, status;
	char line[MAX];
  char originalLine[MAX];
	char *argv[64];
	
	while(1){
		printf("sish> ");
		if(fgets(line,MAX,stdin) != 0) {
			line[strcspn(line, "\n")] = '\0';	//replaces the newline from user input with null
			//printf("\n");
      //printf("Debug: Adding command to history_Sum: '%s'\n", line);
    strcpy(originalLine, line);
		int cmdNum;
		cmdNum = parse(line, argv);			//parse method and returns number of entered commands

		if(strcmp(argv[0],"exit") == 0) {
       history_sum(originalLine);
				exit_cmd();			
				}
        
      if (strcmp(argv[0], "cd") == 0) {
       cd(argv);
       continue;
       }   
        
    if(strcmp (argv[0], "history") == 0) {
       if (argv[1] != NULL && strcmp(argv[1], "-c") == 0) {
       history_clear();
       continue;
      }
      else if (argv[1] == NULL) {
      history_sum(originalLine);
       history_noarg();
       continue;
   }
   else {
   history_offset(argv[1], originalLine);
   continue;
   }}
		
    
		if(cmdNum <= 1 && *argv != NULL){		//if there is no piping, run the entered command
      history_sum(originalLine);
			runCmd(argv);
		}
		else {						//if more than one command(pipes), run pipes
			
			char **cmdList[64];
			int curCmd = 0;
			int j;
			for(j = 0; j < cmdNum; j++) {
				cmdList[j] = &argv[curCmd];	//loads commands into commandlist from parsed argv
        //printf(argv[curCmd]);
        //printf("\n");
				while(argv[curCmd] != NULL) {	//itterates until sees null, which is a flag for next cmd
					curCmd++;
				}
				curCmd++;
			}
			//printf("%d",cmdNum);
     // printf("\n");

			//Executes piped cmds
			int k;
			int pfd[2];				//creates pipe
			int prevPipe = 0;			//fd of previous pipe - initialize to 0 (stdin)
			for(k = 0; k < cmdNum-1; k++){		//executes the command from cmdList 
				pipe(pfd);
				executePipe(cmdList[k], prevPipe, pfd[1]);	
				close(pfd[1]);
				prevPipe = pfd[0];		//after stdin, previous pipe is now reading from previous command		

			}
			executePipe(cmdList[k], prevPipe, 1);	//final method that prints to stdout
		}

	}
}
}
int parse(char *line, char **argv)
{
	int cmdNum = 0;
  char *saveptr1, *saveptr2;					//vars used for strtok
  char *str1, *str2, *token, *subtoken;
  int j;
  
 
  for (j = 1, str1 = line; j++; str1 = NULL) {

                token = strtok_r(str1, "|", &saveptr1);		//first parses the input using pipes to seperate the commands
                if (token == NULL) {
                    break;
                }
                cmdNum++;					//counts number of entered commands based on numebr of tokens (pipe seperated)
                for (str2 = token;; str2 = NULL) {
                    subtoken = strtok_r(str2, " ", &saveptr2);	//second, parses each command and removes spaces
                    if (subtoken == NULL) {
                        break;
                    }
                    *argv++ = subtoken;				//adds to argv
                }
                *argv++ = NULL;					//puts NULL in argv - NULL is the flag that seperates commands
            }   
  *argv = '\0';          					//puts final NULL
  return cmdNum;
}
void runCmd (char **argv)					//method for no piping
{
	pid_t pid;
	int status;
	
	if((pid=fork()) < 0) 					//Fork Error
	{
		printf("Fork Error in runCmd");
	}
	else if(pid == 0) 					//Child process
	{
		if(execvp(*argv, argv) < 0) 
		{
			printf("Failure in execvp\n");
			exit(1);
		}
	}
	else 							//Parent waits for child to finish
	{
		while(wait(&status) != pid);
	}
}

void executePipe (char **cmd, int in_fd, int out_fd) 
{
  pid_t pid;
  int status;
  
  if ((pid = fork()) == -1) {
    printf("Error forking\n");
    exit(1);
  }
  else if (pid == 0) {
	
    if (in_fd != 0) {
      dup2(in_fd, 0);						//redirects input
      close(in_fd);
    }
    if (out_fd != 1) {
      dup2(out_fd, 1);						//redirects output
      close(out_fd);
    }
    if (execvp(*cmd, cmd) < 0) {     				//executes passed cmd
      printf("Failure in execvp\n");
      exit(1);
    }
  }
  else {
    while (wait(&status) != pid); 				//parent waits for child to complete
  }
}

