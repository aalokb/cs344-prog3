/**************************************************************
 * *  Filename: smallsh.c
 * *  Coded by: Kevin To
 * *  Purpose -
 * *
 * ***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

// stdin fix 
#include <termios.h>

#define MAX_ARGS 513 
#define MAX_COMMAND_LENGTH 513 
#define MAX_ERR_MSG_LENGTH 80 

// Function declarations
void RunShellLoop();
void RemoveNewLineAndAddNullTerm(char *stringValue);
int RunForeGroundCommand(char *userCommand, char *errMsg);
void ParseUserInputToArgs(char *userCommand, char **returnArr);
void InitializeArgsArray(char **argv);
int ContainsString(char *stringToSearch, char *stringToSearchFor);
void GetFileName(char *userCommand, char *returnValue);
int RunBackGroundCommand(char *userCommand);
static void sigchld_handler (int sig);

/**************************************************************
 * * Entry:
 * *  N/a
 * *
 * * Exit:
 * *  N/a
 * *
 * * Purpose:
 * *	This is the entry point into the program.
 * *
 * ***************************************************************/
int main()
{
	struct sigaction act;
	act.sa_handler = sigchld_handler;
	sigaction(SIGCHLD, &act, NULL);

	RunShellLoop();

	return 0;
}

/**************************************************************
 * * Entry:
 * *  N/a
 * *
 * * Exit:
 * *  N/a
 * *
 * * Purpose:
 * *	Runs the shell loop
 * *
 * ***************************************************************/
void RunShellLoop()
{
	int exitShell = 0;
	int statusNumber = 0;
	char userInput[80];

	char errMsg[MAX_ERR_MSG_LENGTH] = "";

	while (exitShell == 0)
	{
		fflush(stdout);
    	
    	// Output the colon
		printf(": ");
		fgets(userInput, 79, stdin);
		fflush(stdout);
		RemoveNewLineAndAddNullTerm(userInput);

		// Restart loop if user entered nothing
		if (strcmp(userInput, "") == 0)
		{
			continue;
		}

		// Restart the loop if the user entered a comment
		if (userInput[0] == '#')
		{
			continue;
		}

		// Exit the shell if user wants us to
		if (strcmp(userInput, "exit") == 0)
		{
			exitShell = 1;
			exit(0);
		}

		// Change the current shell's directory to HOME if the 
		// user types "cd"
		if (strcmp(userInput, "cd") == 0)
		{
			char* homePath;
  			homePath = getenv("HOME");
			chdir(homePath);
			continue;
		}

		// Change current directory to user specified directory
		if ((userInput[0] == 'c') && (userInput[1] == 'd') && (userInput[2] == ' '))
		{
			char *argArr[MAX_ARGS];
			InitializeArgsArray(argArr);

			ParseUserInputToArgs(userInput, argArr);
			chdir(argArr[1]);
			continue;
		}

		// Get the status of the previous command
		if (strcmp(userInput, "status") == 0)
		{
			if (strncmp(errMsg, "", MAX_ERR_MSG_LENGTH) == 0)
			{
				printf("exit value %d\n", statusNumber);
			}
			else
			{
				printf("%s\n", errMsg);
			}

			strncpy(errMsg, "", MAX_ERR_MSG_LENGTH);
			statusNumber = 0;
			continue;
		}
		else
		{
			// Clean up the status if we didn't want to display it
			strncpy(errMsg, "", MAX_ERR_MSG_LENGTH);
			statusNumber = 0;
		}

		// Check if we are doing a background process
		if (ContainsString(userInput, "&"))
		{
			// printf("background processes are not supported yet.\n");
			RunBackGroundCommand(userInput);
			// strncpy(userInput, "", MAX_COMMAND_LENGTH);
			continue;
		}
			
		statusNumber = RunForeGroundCommand(userInput, errMsg);

		// If you at the end of an input file, then exit.
		if (feof(stdin))
		{
			exit(0);
		}
	}
}

/**************************************************************
 * * Entry:
 * *  userCommand - the user entered command string
 * *
 * * Exit:
 * *  Returns 0, if command executed without errors.
 * *  Returns any other number, if command executed with errors.
 * *
 * * Purpose:
 * *	Runs the specified foreground command.
 * *
 * ***************************************************************/
int RunBackGroundCommand(char *userCommand)
{	
	// int status = 0;
	int returnStatus = 0;
	pid_t spawnPid = -5;
	char pidNumberStr[10];

	// struct sigaction act;

	//pid_t pid;
	int fd = -1;
	int hasOutputRedirect = ContainsString(userCommand, ">");
	int hasInputRedirect = ContainsString(userCommand, "<");
	char fileName[MAX_COMMAND_LENGTH] = "";

	char *argv[MAX_ARGS];
	InitializeArgsArray(argv);

	// Get the file descriptor if we have to redirect output
	if (hasOutputRedirect == 1)
	{
		GetFileName(userCommand, fileName);
		fd = open(fileName, O_WRONLY|O_TRUNC|O_CREAT, 0644);
	}

	// Get the file descriptor if we have to redirect input 
	if (hasInputRedirect == 1)
	{
		GetFileName(userCommand, fileName);
		fd = open(fileName, O_RDONLY, 0644);
	}
	else
	{
		fd = open("/dev/null", O_RDONLY);
	}

	ParseUserInputToArgs(userCommand, argv);

	// Start the child process for command execution	
	spawnPid = fork();

	

	switch (spawnPid)
	{
		case -1:
			exit(1);
			break;
		case 0:
	
			// Establish the std output redirect, exit if error is found.
			if ((hasOutputRedirect) && (dup2(fd, 1) < 0))
			{ 
				exit(1);
			}
			else if ((dup2(fd, 0) < 0))
			{ 
				// Establish the std input redirect, exit if error is found.
				printf("smallsh: cannot open %s for input\n", fileName);
				exit(1);
			}

   	 		close(fd);

	  		execvp(argv[0], argv);
	  		printf("%s: no such file or directory\n", argv[0]);
	  		exit(1);
			break;
		default:
			close(fd);
			// Output the process ID message for background processes
			snprintf(pidNumberStr, sizeof(pidNumberStr), "%d", spawnPid);
			printf("background pid is %s\n", pidNumberStr);

			break;
	}

	return returnStatus;
}

static void sigchld_handler (int sig)
{
	int status;
	pid_t childPid;

	while ((childPid = waitpid(-1, &status, WNOHANG)) > 0)
	{
		// Convert the child pid to a string
		char pidNumberStr[10];
		snprintf(pidNumberStr, sizeof(pidNumberStr), "%d", childPid);

		// Declare the message variable
		char childTerminateMsg[MAX_ERR_MSG_LENGTH]; 
		char statusNumberStr[5];

		// Construct the child terminate message
		//  Example: "background pid 4923 is done: exit value 0"
		strncpy(childTerminateMsg, "\nbackground pid ", MAX_ERR_MSG_LENGTH);
		strcat(childTerminateMsg, pidNumberStr);
		strcat(childTerminateMsg, " is done: ");
	
		if(WIFSIGNALED(status)) {
				int signalNumber = WTERMSIG(status);
				char signalNumberStr[10];
   				snprintf(signalNumberStr, sizeof(signalNumberStr), "%d", signalNumber);

   				// Output the correct error message and save it for the status command
				strcat(childTerminateMsg, "terminated by signal ");
				strcat(childTerminateMsg, signalNumberStr);
				strcat(childTerminateMsg, "\n");
				write(1, childTerminateMsg, sizeof(childTerminateMsg));
		}
		else
		{
			strcat(childTerminateMsg, "exit value ");
			snprintf(statusNumberStr, sizeof(statusNumberStr), "%d", WEXITSTATUS(status));
			strcat(childTerminateMsg, statusNumberStr);
			strcat(childTerminateMsg, "\n");

			// Write out the message
			write(1, childTerminateMsg, sizeof(childTerminateMsg));
		}

		continue;
	}
}

/**************************************************************
 * * Entry:
 * *  userCommand - the user entered command string
 * *  errMsg - the return variable to hold the error message
 * *
 * * Exit:
 * *  Returns 0, if command executed without errors.
 * *  Returns any other number, if command executed with errors.
 * *
 * * Purpose:
 * *	Runs the specified foreground command.
 * *
 * ***************************************************************/
int RunForeGroundCommand(char *userCommand, char *errMsg)
{	
	int status = 0;
	int returnStatus = 0;
	pid_t spawnPid = -5;

	struct sigaction act;

	//pid_t pid;
	int fd = -1;
	int hasOutputRedirect = ContainsString(userCommand, ">");
	int hasInputRedirect = ContainsString(userCommand, "<");
	char fileName[MAX_COMMAND_LENGTH] = "";

	char *argv[MAX_ARGS];
	InitializeArgsArray(argv);

	// Get the file descriptor if we have to redirect output
	if (hasOutputRedirect == 1)
	{
		GetFileName(userCommand, fileName);
		fd = open(fileName, O_WRONLY|O_TRUNC|O_CREAT, 0644);
	}

	// Get the file descriptor if we have to redirect input 
	if (hasInputRedirect == 1)
	{
		GetFileName(userCommand, fileName);
		fd = open(fileName, O_RDONLY, 0644);
	}

	ParseUserInputToArgs(userCommand, argv);

	// Start the child process for command execution	
	spawnPid = fork();
	switch (spawnPid)
	{
		case -1:
			exit(1);
			break;
		case 0:
			// Establish the std output redirect, exit if error is found.
			if ((hasOutputRedirect) && (dup2(fd, 1) < 0))
			{ 
				exit(1);
			}

			// Establish the std input redirect, exit if error is found.
			if ((hasInputRedirect) && (dup2(fd, 0) < 0))
			{ 
				printf("smallsh: cannot open %s for input\n", fileName);
				exit(1);
			}

			// Do not ignore
			act.sa_handler = SIG_DFL;
			sigaction(SIGINT, &act, NULL);

			// Try to execute the user command
   	 		close(fd);
	  		execvp(argv[0], argv);
	  		printf("%s: no such file or directory\n", argv[0]);
	  		exit(1);
			break;
		default:
			close(fd);
			
			// Ignore termination signal
			act.sa_handler = SIG_IGN;
			sigaction(SIGINT, &act, NULL);

			// Wait for child process to finish	
			// wait(&status);
			waitpid(spawnPid, &status, 0);

			returnStatus = WEXITSTATUS(status);

			// Save the appropriate signal error message
			if(WIFSIGNALED(status)) {
				int signalNumber = WTERMSIG(status);
				char terminateMsg[MAX_ERR_MSG_LENGTH]; 
				char signalNumberStr[10];
   				snprintf(signalNumberStr, sizeof(signalNumberStr), "%d", signalNumber);

   				// Output the correct error message and save it for the status command
				strncpy(terminateMsg, "terminated by signal ", MAX_ERR_MSG_LENGTH);
				strcat(terminateMsg, signalNumberStr);
				printf("%s\n", terminateMsg);
				strncpy(errMsg, terminateMsg, MAX_ERR_MSG_LENGTH);
			}

			break;
	}

	return returnStatus;
}

/**************************************************************
 * * Entry:
 * *  userCommand - the user entered command string
 * *  returnValue - the array containing the user entered commands.
 * *								used as a return container.
 * *
 * * Exit:
 * *  n/a
 * *
 * * Purpose:
 * *  Takes a user entered command string and breaks it up per word
 * *  and puts it into an array. We will not put the redirection 
 * *	symbols into the return array. We will also not put anything
 * *	after the redirection symbols into the return array.
 * *
 * ***************************************************************/
void ParseUserInputToArgs(char *userCommand, char **returnArr)
{
  char *currentToken;
  int currentTokenNumber = 0;

	int HasOutputRedirect = ContainsString(userCommand, ">");
	int HasInputRedirect = ContainsString(userCommand, "<");

  // Increment through the user command and break each word based on the 
  //  whitespace delimiter. Put each word into the return array.
  currentToken = strtok(userCommand, " ");
  while (currentToken != NULL)
  {
  	// Break if we find an output redirect symbol
  	if ((HasOutputRedirect == 1) && (strcmp(currentToken, ">") == 0))
    {
    	break;
    }

  	// Break if we find an input redirect symbol
    if ((HasInputRedirect == 1) && (strcmp(currentToken, "<") == 0))
    {
    	break;
    }

    // Break if we find the background process symbol ("&")
	if (strcmp(currentToken, "&") == 0)
    {
    	break;
    }

    // Add the command arg to the array
  	returnArr[currentTokenNumber] = currentToken;
    currentToken = strtok (NULL, " ");
    currentTokenNumber++;

    // Break out if we are reaching the array limit.
    if (currentTokenNumber == (MAX_ARGS - 1))
    {
  		returnArr[MAX_ARGS - 1] = 0;
    	break;
    }
  }

  // Add the null terminator to the end of the array
  returnArr[currentTokenNumber] = 0;
}

/**************************************************************
 * * Entry:
 * *  userCommand - the user entered command string
 * *  returnValue - the return value to hold the file name
 * *
 * * Exit:
 * *  n/a
 * *
 * * Purpose:
 * *  Gets the file name for the user entered command string. We
 * *  will only get a file name if there is a redirection symbol.
 * *
 * ***************************************************************/
void GetFileName(char *userCommand, char *returnValue)
{
	// There are no redirects, so do not get the file name
	if ((ContainsString(userCommand, "<") == 0) && (ContainsString(userCommand, ">") == 0)) 
	{
		return;
	}

	// Break all the commands into an array and find the first element
	//  after the redirection signs
	char *tempArray[MAX_ARGS];
	char *currentToken;
  int currentTokenNumber = 0;
  int redirSymPosition = 0;

  // Increment through the user command and break each word based on the 
  //  whitespace delimiter. Put each word into the return array.
  InitializeArgsArray(tempArray);
  currentToken = strtok(userCommand, " ");
  while (currentToken != NULL)
  {
  	// Save the position of redirect symbol
  	if ((strcmp(currentToken, "<") == 0) || (strcmp(currentToken, ">") == 0))
  	{
  		redirSymPosition = currentTokenNumber;
  	}

    // Add the command arg to the array
  	tempArray[currentTokenNumber] = currentToken;
    currentToken = strtok (NULL, " ");
    currentTokenNumber++;

    // Break out if we are reaching the array limit.
    if (currentTokenNumber == (MAX_ARGS - 1))
    {
  		tempArray[MAX_ARGS - 1] = 0;
    	break;
    }
  }

  // Get the file name and save it in the return variable
  if (tempArray[redirSymPosition + 1] != 0)
  {
  	strncpy(returnValue, tempArray[redirSymPosition + 1], MAX_COMMAND_LENGTH);
  }
}

/**************************************************************
 * * Entry:
 * *  argv - the array of strings
 * *
 * * Exit:
 * *  n/a
 * *
 * * Purpose:
 * *  Will set all the elements in the specified array to 0.
 * *
 * ***************************************************************/
void InitializeArgsArray(char **argv)
{
	int i; 
	for(i = 0; i < MAX_ARGS; i++)
	{
		// Initialize all values in the array to NULL
		argv[i] = 0;
	}
}

/**************************************************************
 * * Entry:
 * *  stringValue - the string you want to transform
 * *
 * * Exit:
 * *  n/a
 * *
 * * Purpose:
 * *  Removes the new line character from the string and adds a null
 * *  terminator in its place.
 * *
 * ***************************************************************/
void RemoveNewLineAndAddNullTerm(char *stringValue)
{
   size_t ln = strlen(stringValue) - 1;
   if (stringValue[ln] == '\n')
   {
      stringValue[ln] = '\0';
   }
}

/**************************************************************
 * * Entry:
 * *  stringToSearch - The string to perform a search on.
 * *  stringToSearchFor - The string to search for.
 * *
 * * Exit:
 * *  Returns 1, if we found the string.
 * *  Returns 0, if we didn't find the string.
 * *
 * * Purpose:
 * *  To seach a string to see if contains another string.
 * *
 * ***************************************************************/
int ContainsString(char *stringToSearch, char *stringToSearchFor)
{
  char *foundStringPointer;

  // Search for the specified string
  foundStringPointer = strstr(stringToSearch, stringToSearchFor); 

  if (foundStringPointer == 0)
  {
  	return 0; // False, did not find the string.
  }
  else
  {
  	return 1; // True, found the string.
  }
}