#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define WHITESPACE " \t\n" // We want to split our command line up into tokens \
                           // so we need to define what delimits our tokens.   \
                           // In this case  white space                        \
                           // will separate the tokens on our command line
#define MAX_COMMAND_SIZE 255 // The maximum command-line size
#define MAX_NUM_ARGUMENTS 10 // Mav shell only supports 10 arguments

int main()
{
  char *cmd_str = (char *)malloc(MAX_COMMAND_SIZE);
  char *recent_cmd[15]; //This will store last 15 command that is required to show on history
  pid_t pid_history[15];//This will store the pid of the last 15 processor
  int hcount = 0;  //To keep track of command history
  int count = 0;   //To keep track of pid history
  int i;   //To run for loop 
  while (1)
  {
    // Print out the msh prompt
    printf("msh> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while (!fgets(cmd_str, MAX_COMMAND_SIZE, stdin));
	
	//If user just press return key then loop will start by 
	//printing the msh command again
    if (cmd_str[0] == '\n')
    {
	  continue;
    }

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    int token_count = 0;

    // Pointer to point to the token
    // parsed by strsep
    char *argument_ptr;
    //Copy of command that is stored in cmd_str is created dynamically.
    //The address is returned to working_str
    char *working_str = strdup(cmd_str);
    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;
	
	//We are checking if user wants to run command 
	//from the history 
    if (working_root[0] == '!')
	{
	  int line;
	  //Convert string value into int so that we can get
	  //Command from the history
	  
	  line = atoi(&working_root[1]);	  
	  working_str = strdup(recent_cmd[line-1]); //Subracting 1 as value is stored from 0
	}
    //Copy of command is stored in recent command
	//So that we can track last 15 commands.
    if (hcount < 15)
    {
      recent_cmd[hcount] = strndup(cmd_str,MAX_COMMAND_SIZE);
      hcount++;
    }
    else
    {
	  //After first 15 command stored we need to 
	  //replace the first command with the 
	  //latest command. 
      for (i = 0; i < 14; i++)
      {
        recent_cmd[i] = strdup(recent_cmd[i+1]);
      }
        recent_cmd[14] = strdup(cmd_str);
    }

    // Tokenize the input stringswith whitespace used as the delimiter
    while (((argument_ptr = strsep(&working_str, WHITESPACE)) != NULL) &&
           (token_count < MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup(argument_ptr, MAX_COMMAND_SIZE);
      if (strlen(token[token_count]) == 0)
      {
        token[token_count] = NULL;
      }
      token_count++;
    }
	//Checking if user want to exit from the mav shell
    if (strcmp(token[0], "exit") == 0 || strcmp(token[0], "quit") == 0)
    {
	  
      exit(0);
    }

    else if (strcmp(token[0], "cd") == 0)
    {
      int child;
      if (token[1] == NULL)
      {
        child = chdir(getenv("HOME"));
      }
      else
      {
        child = chdir(token[1]);
      }
      if (child == -1)
      {
        printf("No such directory found\n");
      }
      continue;
    }
    else if (strcmp(token[0], "showpids") == 0)
    {
      for (i = 0; i < count; i++)
      {
        printf("%d: %d\n", i + 1, pid_history[i]);
      }
      continue;
    }
    else if (strcmp(token[0], "history") == 0)
    {
      for (i = 0; i < hcount; i++)
      {
        printf("%d: %s", i + 1, recent_cmd[i]);
      }
      continue;
    }
	int status;
    pid_t pid = fork();
    if (pid == 0)
    {
      int ret = execvp(token[0], &token[0]);
      //printf("%d\n",ret);
      if (ret == -1)
      {
        printf("%s: Command is not found\n", token[0]);
      }
    }
    else
    {
      if (count < 15)
      {
        pid_history[count] = getpid();
        count++;
      }
      else
      {
        for (i = 0; i < 14; i++)
        {
          pid_history[i] = pid_history[i + 1];
        }
        pid_history[14] = getpid();
      }
      waitpid(pid,&status,0);
    }

    // Now print the tokenized input as a debug check
    // \TODO Remove this code and replace with your shell functionality
    /*
    int token_index  = 0;
    for( token_index = 0; token_index < token_count; token_index ++ ) 
    {
      printf("token[%d] = %s\n", token_index, token[token_index] );  
    }
    */
    free(working_root);
  }
  return 0;
}


