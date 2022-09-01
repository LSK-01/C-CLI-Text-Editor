#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

void readInput(char **content);
int doesExist(char *firstArg);
int getCommandInput(char *fullcmd, char *cmd, char *flag, char *firstArg, char *secondArg);
void writeToLog(char *op, char *firstArg, int numLines);
int getNumLines(FILE *fileP);
char *getFilenameArg(char *firstArg, char *secondArg, int *numLines);

#define bufferSizeInit 20
#define filenameLim 255
int main()
{
  char info[] = "Create a file == cr -f filename\nCopy a file == cp -f sourcefile destinationfile\nAppend n lines (n can be left out and will default to 1) == ap -l n filename\nDelete a file/n lines (n can be left out and will default to 1) == dl -f/l n filename\nShow a file/line == sh -f/l filename\nInsert n lines (n can be left out and will default to 1) == cr -l n filename\nRename a file == rn -f originalfile newfile\nType 'quit' to quit the editor\n";

  char cmd[3];
  char flag[3];
  char firstArg[filenameLim + 1];
  char secondArg[filenameLim + 1];

  char dirName[filenameLim + 1];

  int res;

  printf("_____________________________________________________________\n\n");

  printf("Welcome to the best editor you will ever use. To:\n\n%s", info);
  printf("_____________________________________________________________\n\n");

  //check if user has permissions to read and write in this directory - if not exit the program because almost
  //all commands require this
  getcwd(dirName, sizeof(dirName));
  if (access(dirName, R_OK) + access(dirName, W_OK) != 0)
  {
    printf("You are missing read and/or write permissions in this directory that the program needs to work. Exiting program.\n");
    return 1;
  }

  while (1)
  {
    //reset pointers to null for next command
    char *content = NULL;
    char *lineNumS = NULL;
    char *fullcmd = NULL;

    FILE *firstFP = NULL;
    FILE *secondFP = NULL;

    //nullify argument strings
    secondArg[0] = '\0';
    firstArg[0] = '\0';
    cmd[0] = '\0';
    flag[0] = '\0';

    res = getCommandInput(fullcmd, cmd, flag, firstArg, secondArg);

    while (res == 0)
    {
      printf("Your command or first argument was formatted badly. Try again you ape. (You can type 'man' to see which commands are allowed).\n");
      res = getCommandInput(fullcmd, cmd, flag, firstArg, secondArg);
    }
    if (res == 2)
    {
      //show instructions
      printf("\n%s", info);
    }
    if (res == -1)
    {
      printf("Quit the editor. Thank you for using me honey.\n");
      free(content);
      free(lineNumS);
      break;
    }

    switch (cmd[1])
    {
    //rename
    case 'n':
      rename(firstArg, secondArg);
      break;
    //create (cr) -f or -l n
    case 'r':
      switch (flag[1])
      {
      case 'f':
        //open a file for writing, created if does not exist.
        firstFP = fopen(firstArg, "w");
        writeToLog("was created", firstArg, getNumLines(firstFP));
        break;
      case 'l':
      {

        int numLines = 1;
        int *p = &numLines;
        char *filename;

        filename = getFilenameArg(firstArg, secondArg, p);
        if (strcmp(filename, "") == 0)
        {
          break;
        }

        printf("Line number to insert at: ");
        readInput(&lineNumS);

        //if lineNumS is not a valid integer, strtol() will return 0, and no line will be written, as intended.
        int lineNum = strtol(lineNumS, NULL, 10);

        char *tempName = "tempFile.txt";
        firstFP = fopen(filename, "r");
        secondFP = fopen(tempName, "w+");
        char currC = fgetc(firstFP);
        char firstC = currC;

        int i = 1;
        while (currC != EOF)
        {
          if (currC == '\n')
          {
            i++;
          }
          if (i == lineNum)
          {
            if (i != 1)
            {
              fputc('\n', secondFP);
              currC = fgetc(firstFP);
            }

            printf("Type content to insert: \n");
            for (int i = 1; i <= numLines; i++)
            {
              char *content = NULL;
              readInput(&content);

              fputs(content, secondFP);
              free(content);
            }

            //move on to the next lines, content has been inserted
            i++;
          }
          fputc(currC, secondFP);
          currC = fgetc(firstFP);
        }

        if (firstC == EOF && i == 1)
        {
          printf("This file is empty - use the append function to add content.\n");
          break;
        }
        else if (lineNum > i)
        {
          printf("This file ends at line %d\n", getNumLines(secondFP));
          break;
        }
        printf("Done inserting.\n");
        //rename tempfile to actual file
        rename(tempName, filename);
        writeToLog("had insertions", filename, getNumLines(secondFP));

        break;
      }
      }
      break;
    //delete -f or -l
    case 'l':
      switch (flag[1])
      {
      case 'f':
      {
        if (remove(firstArg) != 0)
        {
          printf("Could not delete the file. Try again.\n");
          break;
        }

        writeToLog("was deleted", firstArg, 0);
        break;
      }

      case 'l':
      {
        printf("Line number to start deletion from: ");
        readInput(&lineNumS);
        //if lineNumS is not a valid integer, strtol() will return 0, and no line will be written, as intended.
        int lineNum = strtol(lineNumS, NULL, 10);
        int numLines = 1;
        int *p = &numLines;
        char *filename;

        filename = getFilenameArg(firstArg, secondArg, p);
        if (strcmp(filename, "") == 0)
        {
          break;
        }

        //read through file, adding characters to temp file, iterating every time newline is found. When required line is reached, iterate over but dont add
        //any characters to temp file until next newline. Then continue adding till EOF. then rename temp file.
        char *tempName = "tempFile.txt";
        secondFP = fopen(tempName, "w+");
        firstFP = fopen(filename, "r");
        char currC = fgetc(firstFP);

        int i = 1;
        while (currC != EOF)
        {

          if (currC == '\n')
          {
            //we dont want to include the newline char if deleting first line
            if (lineNum == 1 && i == 1)
            {
              currC = fgetc(firstFP);
            }
            i++;
          }

          //only write these characters
          if (i < lineNum || i > lineNum + numLines - 1)
          {
            fputc(currC, secondFP);
          }
          currC = fgetc(firstFP);
        }

        rename(tempName, filename);
        //if no lines in the file
        if (i == 1)
        {
          printf("The file is empty\n");
          break;
        }

        writeToLog("had deletions", filename, getNumLines(secondFP));
        break;
      }
      }
      break;
    //show -f or -l or -n
    case 'h':
      switch (flag[1])
      {
      case 'f':
      {
        printf("_____________________________________________________________\n\n");

        //using fgetc() instead of fgets() so there is no limit to line size
        firstFP = fopen(firstArg, "r");
        char currC = fgetc(firstFP);
        while (currC != EOF)
        {
          putchar(currC);
          currC = fgetc(firstFP);
        }
        printf("\n_____________________________________________________________\n\n");

        break;
      }
      case 'l':
      {
        printf("Enter a line number to show: ");
        readInput(&lineNumS);
        int lineNum = strtol(lineNumS, NULL, 10);

        firstFP = fopen(firstArg, "r");
        char currC = fgetc(firstFP);

        int i = 1;
        while (currC != EOF)
        {
          while (currC == '\n')
          {
            //we dont want to include the newline char at the beggining
            currC = fgetc(firstFP);

            i++;
          }
          if (i == lineNum)
          {
            do
            {
              putchar(currC);
              if (currC == '\n')
              {
                break;
              }
              currC = fgetc(firstFP);

            } while (currC != '\n' && currC != EOF);
            break;
          }

          currC = fgetc(firstFP);
        }
        putchar('\n');
        break;
      }
      case 'n':
        firstFP = fopen(firstArg, "r");
        printf("%d\n", getNumLines(firstFP));
      }
      break;
    //copy -f or append -l
    case 'p':
      switch (flag[1])
      {
      case 'f':
      {
        firstFP = fopen(firstArg, "r");
        // Open another file for writing
        secondFP = fopen(secondArg, "w");
        if (secondFP == NULL)
        {
          printf("Cannot open file %s \n", firstArg);
          return 1;
        }

        // Read contents from file
        char currC = fgetc(firstFP);
        while (currC != EOF)
        {
          fputc(currC, secondFP);
          currC = fgetc(firstFP);
        }
        writeToLog("was copied", firstArg, getNumLines(firstFP));
        break;
      }

      case 'l':
      {

        int numLines = 1;
        int *p = &numLines;
        char *filename;
        //if they have specified two arguments with ap -l
        filename = getFilenameArg(firstArg, secondArg, p);
        if (strcmp(filename, "") == 0)
        {
          break;
        }

        firstFP = fopen(filename, "r+");

        //move pointer to beggining to see if newline char is needed if file is not empty
        fseek(firstFP, 0, SEEK_SET);
        char firstC = fgetc(firstFP);

        //move back pointer to end of file to append
        fseek(firstFP, 0, SEEK_END);
        if (firstC != EOF)
        {
          fputc('\n', firstFP);
        }

        printf("Type content to append:\n");
        for (int i = 1; i <= numLines; i++)
        {
          char *content = NULL;
          readInput(&content);

          if (i == numLines)
          {
            //remove final newline char
            content[strlen(content) - 1] = '\0';
          }

          fputs(content, firstFP);
          free(content);
        }
        printf("Done appending.\n");
        writeToLog("was appended to", filename, getNumLines(firstFP));
        break;
      }
      }
      break;
    }

    //close any open file streams, otherwise can lead to undefined behaviour
    if (firstFP != NULL)
    {
      fclose(firstFP);
    }

    if (secondFP != NULL)
    {
      fclose(secondFP);
    }

    //free memory - don't know how long the user will wait between commands or if
    //these buffers will even be used again in the next commands, so free them at
    //the end of every command
    free(content);
    free(lineNumS);
  }
  return 0;
}

void readInput(char **contentPtr)
{

  size_t size = 0;
  getline(contentPtr, &size, stdin);
}

//get the filename argument and the num lines argument if present
char *getFilenameArg(char *firstArg, char *secondArg, int *numLines)
{
  //if they have specified two arguments with ap -l
  if (secondArg[0] != '\0')
  {
    //0 if not number input
    *numLines = strtol(firstArg, NULL, 10);
    if (!*numLines)
    {
      printf("Specify a valid positive number first when using 2 arguments with dl -l\n");
      return "\0";
    }
    return secondArg;
  }
  else
  {
    return firstArg;
  }
}

int getNumLines(FILE *fileP)
{
  fseek(fileP, 0, SEEK_SET);
  char currC = fgetc(fileP);

  int i = 1;
  while (currC != EOF)
  {

    if (currC == '\n')
    {
      i++;
    }
    currC = fgetc(fileP);
  }
  return i;
}

void writeToLog(char *op, char *filename, int numLines)
{
  FILE *log = fopen("ChangeLog.txt", "a");

  fprintf(log, "%s %s at %lu. New number of lines: %d\n", filename, op, time(NULL), numLines);
  fclose(log);
}

int getCommandInput(char *fullcmd, char *cmd, char *flag, char *firstArg, char *secondArg)
{

  size_t buff = bufferSizeInit;
  getline(&fullcmd, &buff, stdin);
  //parse string
  const char s[2] = " ";
  char *token;

  token = strtok(fullcmd, s);

  if (strcmp(token, "quit\n") == 0)
  {
    free(fullcmd);
    return -1;
  }
  if (strcmp(token, "man\n") == 0)
  {
    free(fullcmd);
    return 2;
  }

  int i = 0;
  while (token != NULL)
  {
    i++;
    if (i < 3 && (token == NULL || strlen(token) != 2))
    {
      //if either the cmd or flag is not the right size, or we only have 2 arguments
      free(fullcmd);
      return 0;
    }
    //if either argument is longer than 255 - filenames can only be this lengths or shorter
    else if (i > 2 && token != NULL && strlen(token) > filenameLim)
    {
      free(fullcmd);
      return 0;
    }
    //if any parts of the command have a '/', this is not allowed in my program, and filenames are not allowed to have '/' in the name in unix systems
    else if (strchr(token, '/') != NULL)
    {
      free(fullcmd);
      return 0;
    }

    switch (i)
    {
    case 1:
      strcpy(cmd, token);
      break;
    case 2:
      strcpy(flag, token);
      break;
    case 3:
      strcpy(firstArg, token);
      break;
    case 4:
      strcpy(secondArg, token);
      break;
    }
    token = strtok(NULL, s);

    //remove newline character read in by getline()
    switch (i)
    {
      FILE *fpTemp;
    case 3:
    {
      //if firstArg is the only argument (need to check if whitespace fucks this up)
      if (firstArg[strlen(firstArg) - 1] == '\n')
      {
        firstArg[strlen(firstArg) - 1] = '\0';

        //if we are not creating a file and file does not exist
        if ((strcmp(cmd, "cr") + strcmp(flag, "-f") != 0) && access(firstArg, F_OK) != 0)
        {
          printf("The file %s does not exist - try creating it first with 'cr -f'\n", firstArg);
          free(fullcmd);
          return 0;
        }
      }

      break;
    }
    case 4:
      //if secondArg is '\n' we just have trailing whitespace on firstArg so break
      if (strcmp(secondArg, "\n") == 0)
      {
        break;
      }
      secondArg[strlen(secondArg) - 1] = '\0';

      if ((strcmp(cmd, "cp") + strcmp(flag, "-f") == 0))
      {
        //only need to check if firstArg exists if using copy command, else check if secondArg exists

        if (access(firstArg, F_OK) != 0)
        {
          printf("The file %s does not exist - try creating it first with 'cr -f'\n", firstArg);
          free(fullcmd);
          return 0;
        }
      }
      //not using copy command, so firstArg should be a number and secondArg a file
      //unless renaming a file
      else if (access(secondArg, F_OK) != 0 && (strcmp(cmd, "rn") + strcmp(flag, "-f") != 0))
      {
        printf("The file %s does not exist - try creating it first with 'cr -f'\n", secondArg);
        free(fullcmd);
        return 0;
      }
      break;
    }
  }
  free(fullcmd);
  return 1;
}