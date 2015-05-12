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

// Function declarations
void RunShellLoop();
void RemoveNewLineAndAddNullTerm(char *stringValue);

// Program entry point
int main()
{
	RunShellLoop();
	return 0;
}

void RunShellLoop()
{
	int exitShell = 0;

      char *argv[10];
      argv[0] = "ls";
      argv[1] = 0;
      execvp(argv[0], argv);

	while (exitShell == 0)
	{
      	// Output the colon
		printf(": ");

		char userInput[80];
		fgets(userInput, 79, stdin);
		RemoveNewLineAndAddNullTerm(userInput);

		if (strcmp(userInput, "exit") == 0)
		{
			exitShell = 1;
		}
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
