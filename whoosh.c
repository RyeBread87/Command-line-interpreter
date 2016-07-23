#include "stdio.h"
#include "unistd.h"
#include "stdlib.h"
#include "string.h"
#include "sys/types.h"
#include "sys/wait.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdint.h"

void parseInput(char* cmd, char** params);
int executeCommand(char** params);
int builtInCommand(char ** params);
int updatePaths(char ** params);

#define MAX_LENGTH 128
#define MAX_NUMBER_OF_PARAMS 128
#define MAX_NUMBER_OF_PATHS 128

char ** paths;     // = malloc(1536);  //(MAX_NUMBER_OF_PATHS * MAX_LENGTH);
int numberOfPieces = 0;
int numberOfPaths = 1;
int usingRedirection = 0;
char * destination;

int
main(int argc, char *argv[])
{
  if (argv[1] != NULL) {
      char error_message[30] = "An error has occurred\n";
      write(STDERR_FILENO, error_message, strlen(error_message));
      return 1;
    }

  // initial variable setup
  char cmd[MAX_LENGTH + 1];
  char * params[MAX_NUMBER_OF_PARAMS + 1];
  paths = malloc(MAX_NUMBER_OF_PATHS * MAX_LENGTH);
  paths[0] = "/bin/";

  char * prompt = "whoosh> ";

  // start forever loop
  for ( ; ; ) {

    int badStuff = 0;
    write(1, prompt, 8);

    // get and fix up input
    if (fgets(cmd, sizeof(cmd) + 1, stdin) == NULL) break;           
    if(cmd[strlen(cmd) - 1] == '\n') {
      cmd[strlen(cmd) - 1] = '\0';
    }

    if (strlen(cmd) == 129) {
      int ch;
      char error_message[30] = "An error has occurred\n";
      //char error_message[30] = "An error has occurred 1\n";
      write(STDERR_FILENO, error_message, strlen(error_message));
      badStuff = 1;
      while ((ch = getchar()) != '\n' && ch != EOF);
    }

    if (badStuff == 1) continue;

    // parse input
    parseInput(cmd, params);

    if (params[0] == NULL) continue;

    if(executeCommand(params) == 0) break;
  }

  return 0;
}

// split command into array of parameters
void parseInput(char* cmd, char** params)
{ 
  int i = 0;
  //int Length = 0;
  int tokenLength = 0;

  char * token = strtok((char *) cmd, " ");

  if (token == NULL) {
    params[0] = NULL;
    return;
  }

  char gt = '>';
  //char slash = '/';

  while (token != NULL) {
    tokenLength = strlen(token);
    if ((tokenLength == 1) && (token[0] == gt)) {
      usingRedirection = 1;
      break;
    }
    //else if (token[0] == slash) {
    //char * thePath = NULL;
    //thePath = strdup(token);
    //int validPath = access(token, F_OK);
    //if (validPath == -1) {
    //	char error_message[30] = "An error has occurred\n";
    //	write(STDERR_FILENO, error_message, strlen(error_message));
    //	params[0] = NULL;
    //	return;
    //}
    //}
    else {
      params[i] = token;
      token = strtok(NULL, " ");
      i++;
      usingRedirection = 0;
    }
  }

  //printf("testMessage %s", params[0]);

  //if (Length > 128) {
  //params[0] = NULL;
    //char error_message[30] = "An error has occurred\n";
    //write(STDERR_FILENO, error_message, strlen(error_message));
    //return;
    //}

  if (usingRedirection == 1) {
    token = strtok(NULL, " ");
    destination = token;
    if (destination == NULL) {
      params[0] = NULL;
      char error_message[30] = "An error has occurred\n";
      write(STDERR_FILENO, error_message, strlen(error_message));
      return;
    }
    
    if (destination[0] == '/') {
    char * thePath = NULL;
    thePath = strdup(destination);
    int validPath = access(thePath, F_OK);
    if (validPath == -1) {
      char error_message[30] = "An error has occurred\n";
      write(STDERR_FILENO, error_message, strlen(error_message));
      params[0] = NULL;
      return;
    }
    }

    token = strtok(NULL, " ");
    if (token != NULL) {
      params[0] = NULL;
      char error_message[30] = "An error has occurred\n";
      write(STDERR_FILENO, error_message, strlen(error_message));
      return;
    }
  }

  params[i] = NULL;  
  numberOfPieces = i;
  }

// actually execute commands
// first try built-in commands as one process
// then fork and try other commands as child process
int executeCommand(char** params)
{
  char * command = params[0];

  //int pipefd[2];
  //pipe(pipefd);

  // see if the command is built in
  if ((strcmp(command, "cd") == 0) || (strcmp(command, "pwd") == 0) || (strcmp(command, "path") == 0) || (strcmp(command, "exit") == 0)) {
    builtInCommand(params);
    return 1;
  }

  // fork process
  int pid = fork();

  // error
  if (pid < 0) {
    //char* error = strerror(0);
    char error_message[30] = "An error has occurred\n";
    //char error_message[30] = "An error has occurred 2\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
    return 1;
  }

  // child process
  else if (pid == 0) {

    // let parent process handle built-in commands
    if ((strcmp(command, "cd") == 0) || (strcmp(command, "pwd") == 0) || (strcmp(command, "path") == 0) || (strcmp(command, "exit") == 0)) return 0;

    int i = 0;

    for (i = 0; i < numberOfPaths; i++) {
      //path = paths[i];
      char * finalCommand = malloc(129);
      finalCommand = strdup(paths[i]);
      strcat(finalCommand, command);

      char * myargv2[MAX_LENGTH];
      myargv2[0] = strdup(finalCommand);
      
      int j = 1;
      for (j = 1; j < numberOfPieces; j++) {
	myargv2[j] = strdup(params[j]);
      }

      int out = -1;
      int err = -1;
      char * stdoutDest = NULL;
      char * stderrDest = NULL;
      myargv2[j] = NULL;

      //printf("testing rdr %d, %s\n", usingRedirection, destination);
      if (usingRedirection == 1) {
	if (destination == NULL) {
	  char error_message[30] = "An error has occurred\n";
          //char error_message[30] = "An error has occurred 3\n";
          write(STDERR_FILENO, error_message, strlen(error_message));
          return 0;
        }
	else {
	  stdoutDest = strdup(destination);
	  strcat(stdoutDest, ".out");
	  stderrDest = strdup(destination);
          strcat(stderrDest, ".err");
	  //printf("testing rdr %d, %s\n", usingRedirection, stdoutDest);
	  close(STDOUT_FILENO);
	  //close(STDERR_FILENO);
	  out = open(stdoutDest, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
   	  close(STDERR_FILENO);
	  //open(destination, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
	  err = open(stderrDest, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
	  //printf("yyyyyyywawwzzwzwws %d, %d\n", out, err);
	  //int _fd_dup = dup2(fd, 0);
	  //close(fd);
	  //printf("testing rdr %d, %s\n", usingRedirection, destination);
	  //printf("testing fc %d\n", fd);
	}
      }

      //close(STDERR_FILENO);
      //open(destination, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
      //err = open("output.err", O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);

      //printf("wawwzzwzwws %d, %d\n", out, err);

      //close(pipefd[0]);    // close reading end in the child

      dup2(out, 1);  // send stdout to the pipe
      dup2(err, 2);  // send stderr to the pipe

      //write(pipefd[1], "testing sendmessage", 19);

      execv(myargv2[0], myargv2);

      //close(pipefd[1]);    // this descriptor is no longer needed

      char error_message[30] = "An error has occurred\n";
      write(STDERR_FILENO, error_message, strlen(error_message));
    }

    //close(STDOUT_FILENO);
    //open("/tmp/file.stdout", O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR)
    //char *myarg[4];
    //myarg[0] = strdup("/bin/ls");
    //myarg[1] = strdup("-l");
    //myarg[2] = strdup("-a");
    //myarg[3] = (char *) NULL;
    //execv(myarg[0], myarg);

    // error
    //char* error = strerror(0);
    //if(strcmp(error, "Success") != 0) {
    //char error_message[30] = "An error has occurred\n";
    //char error_message[30] = "An error has occurred 4\n";
    //write(STDERR_FILENO, error_message, strlen(error_message));
      //}
    return 0;
  }

  // parent process
  else {
    //char buffer[1024];

    //close(pipefd[1]);  // close the write end of the pipe in the parent

    //read(pipefd[0], buffer, sizeof(buffer));
    
    //write(1, buffer, strlen(buffer));
    //printf("Received string: %s\n", buffer);

      int childStatus;
      waitpid(pid, &childStatus, 0);
      return 1;
   }
}

int
builtInCommand(char ** params)
{
  int didItWork = -1;

  // built-in command for exit
  if ((numberOfPieces == 1) && (strcmp(params[0], "exit") == 0)) exit(0);
 
  // built-in command for cd
  else if (strcmp(params[0], "cd") == 0) {
    if ((numberOfPieces == 1) && (params[1] == '\0')) {
      char * homeDirectory = getenv("HOME");
      didItWork = chdir(homeDirectory);
      if (didItWork != 0) {
	char error_message[30] = "An error has occurred\n";
  	//char error_message[30] = "An error has occurred 5\n";
	write(STDERR_FILENO, error_message, strlen(error_message));
      }
    }

    else if ((numberOfPieces == 2) && (params[2] == '\0')) {
      char * newDirectory = params[1];
      didItWork = chdir(newDirectory);
      if (didItWork != 0) {
	char error_message[30] = "An error has occurred\n";
	//char error_message[30] = "An error has occurred 6\n";
	write(STDERR_FILENO, error_message, strlen(error_message));
      }
    }

    else {
      char error_message[30] = "An error has occurred\n";
      //char error_message[30] = "An error has occurred 7\n";
      write(STDERR_FILENO, error_message, strlen(error_message));
    }

    return 1;
  }

  // built-in command for 
  else if ((numberOfPieces == 1) && (strcmp(params[0], "pwd")) == 0) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
      //fprintf(stdout, "%s\n", cwd);
      write(1, cwd, strlen(cwd));
      write(1, "\n", 1);
    }
    else {
      char error_message[30] = "An error has occurred\n";
      //char error_message[30] = "An error has occurred 8\n";
      write(STDERR_FILENO, error_message, strlen(error_message));
    }
    return 1;
  }

  // built-in command for path
  else if (strcmp(params[0], "path") == 0) {
    updatePaths(params);
    return 1;
  }

  else return 0;
  return 1;
}

int
updatePaths(char** params)
{
  int i = 1;
  int pathLength = 0;
  int validPath = 0;

  // delete paths;
  int j = 0;
  for (j = 0; j < numberOfPaths; j++) {
    paths[j] = NULL;
    numberOfPaths = 0;
  }

  if (numberOfPieces == 1) {
    paths[0] = NULL;
    numberOfPaths = 0;
    return 0;
  }

  char * thePath = malloc(MAX_LENGTH);
  for (i = 1; i < numberOfPieces; i++) {
    strcpy(thePath, params[i]);

    //check if path is valid
    pathLength = strlen(thePath) - 1;
    
    validPath = access(thePath, F_OK);
    if (validPath == -1) {
      char error_message[30] = "An error has occurred\n";
      //char error_message[30] = "An error has occurred 9\n";
      write(STDERR_FILENO, error_message, strlen(error_message));
      continue;
    }
    validPath = access(thePath, R_OK);
    if (validPath == -1) {
      char error_message[30] = "An error has occurred\n";
      //char error_message[30] = "An error has occurred 10\n";
      write(STDERR_FILENO, error_message, strlen(error_message));
      continue;
    }
    
    char a = thePath[pathLength];
    char b = '/';
    if ( a != b ) {
      thePath[pathLength + 1] = '/';
      thePath[pathLength + 2] = '\0';
    }

    if (validPath == 0) {
      //strcpy(paths[i-1], thePath);
      paths[i-1] = thePath;
      numberOfPaths = i;
     }
  }

  return 0;
}
