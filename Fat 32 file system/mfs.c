/*
  Santosh Bhandari  
  1001387116
  Assignment #4
  12/06/2020
*/
//the file system open file on open and close file on close

//exit command to close the mfs> shell.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>

#define MAX_NUM_ARGUMENTS 4

#define WHITESPACE " \t\n" // We want to split our command line up into tokens \
                           // so we need to define what delimits our tokens.   \
                           // In this case  white space                        \
                           // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255 // The maximum command-line size

FILE *fp; //Pointer to point the file

int16_t BPB_BytsPerSec; //Count bytes per sector
int8_t BPB_SecPerClus;  //Count no of sector per unit
int16_t BPB_RsvdSecCnt; //Number of reserved sectors
int8_t BPB_NumFATs;     //Count FAT data structure
int32_t BPB_FATSz32;    //32-bit count occupied by one FAT

int32_t address;

struct __attribute__((__packed__)) DirectoryEntry
{
  char DIR_Name[11];
  uint8_t DIR_Attr;
  uint8_t Unused[8];
  uint16_t DIR_FirstClusterHigh;
  uint8_t Unused2[4];
  uint16_t DIR_FirstClusterLow;
  uint32_t DIR_FileSize;
};
//Structure array of the Directory Entry
struct DirectoryEntry dir[16];

//function that take the sector number that point block of data
//And return the value of the address for the block of data
//by finding the starting address of block of data given the sector number
//corresponding to that data

int LBAToOffset(int32_t sector)
{
  return ((sector - 2) * BPB_BytsPerSec) +
         (BPB_BytsPerSec * BPB_RsvdSecCnt) + (BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec);
}

//Function to look for a logical address, look up into the first FAT
//and return the logical address of the block in the file. If
//there is no further blocks then return -1
int16_t NextLB(uint32_t sector)
{
  uint32_t FATAddress = (BPB_BytsPerSec * BPB_RsvdSecCnt) + (sector * 4);
  int16_t val;
  fseek(fp, FATAddress, SEEK_SET);
  fread(&val, 2, 1, fp);
  return val;
}

//Function to handle cd
int cd(char *directoryName)
{
  int i;
  //Set the found to 0
  //if Dir_Name match then set
  //found to 1
  int found = 0;
  //save the cluster number is directory is found
  int cluster;
  for (i = 0; i < 16; i++)
  {
    char *tempname;
    tempname = strdup(dir[i].DIR_Name);
    char *token;
    //tokenzing the name just to get
    //name of the file
    token = strtok(tempname, " ");
    if (strcmp(token, directoryName) == 0)
    {
      cluster = dir[i].DIR_FirstClusterLow;
      if (cluster == 0)
      {
        cluster = 2;
      }
      //to get the offset from the beginning
      //This offset helps to point us where the
      //directory begin so that we can seek and read
      // the file
      int offset = LBAToOffset(cluster);
      //printf("%d\n", offset);

      //Seeking at address offset from the
      //beginning of the file system.
      fseek(fp, offset, SEEK_SET);

      //Reading the data from the directory to dir
      //16 elements of size of struct from file pointer
      fread(dir, sizeof(struct DirectoryEntry), 16, fp);
      found = 1;
      break;
    }
  }
  //if file is not found then
  //print error messsage to user.
  if (!found)
  {
    printf("error:Directory not found\n");
    return -1;
  }
  return 0;
}

//Function to handle read command entered by the users.
int readData(char *name, int requestOffset, int requestedBytes)
{
  int i;         //to handle for loop
  int found = 0; //status to inform file is found or not

  //Running for loop to find the file name
  for (i = 0; i < 16; i++)
  {
    char *tempname;
    tempname = strdup(dir[i].DIR_Name);
    char *token;
    //tokenzing the name just to get
    //name of the file
    token = strtok(tempname, " ");
    if (strcmp(token, tempname) == 0)
    {
      found = 1;
      //This gives the logical address that point the block
      //at which the file is present
      int cluster = dir[i].DIR_FirstClusterLow;
      int searchSize = requestOffset;

      while (searchSize >= BPB_BytsPerSec)
      {
        //The address of the next logical block for the file
        //to go to the position at which user want us to start
        //to read the file i.e. position of the file
        cluster = NextLB(cluster);
        searchSize = searchSize - BPB_BytsPerSec;
      }

      //Gives the starting addresss of the block
      int offset = LBAToOffset(cluster);

      //This gives the number of block that
      //we need to upset before reading the file
      int byteoffset = (requestOffset % BPB_BytsPerSec);
      fseek(fp, offset * byteoffset, SEEK_SET);
      //As we already subtracted the search size greter than bytes per sector which skips blocks
      //Now the file where we want to read can be achieved by
      //Subtracting the remaining size from the bytes per secoter i.e block

      int firstBlockBytes = BPB_BytsPerSec - searchSize;
      unsigned char buffer[BPB_BytsPerSec];
      fread(buffer, 1, firstBlockBytes, fp);

      for (i = 0; i < firstBlockBytes; i++)
      {
        printf("%x ", buffer[i]);
      }

      //Here we printed the bytes that were in firstblock
      //So the bytes that we need to read decrease
      requestedBytes = requestedBytes - firstBlockBytes;
      //Here we will  be reading full block
      //This is the middle part of the bytes that we read
      while (requestedBytes >= BPB_BytsPerSec)
      {
        cluster = NextLB(cluster);
        offset = LBAToOffset(cluster);
        fseek(fp, offset, SEEK_SET);
        fread(buffer, 1, BPB_BytsPerSec, fp);
        for (i = 0; i < BPB_BytsPerSec; i++)
        {
          printf("%x ", buffer[i]);
        }
        requestedBytes = requestedBytes - BPB_BytsPerSec;
      }
      //At last if we don't have to read full block
      //then we just need to read remaining part of the
      //block
      if (requestedBytes)
      {
        cluster = NextLB(cluster);
        offset = LBAToOffset(cluster);
        fseek(fp, offset, SEEK_SET);
        fread(buffer, 1, requestedBytes, fp);
        for (i = 0; i < requestedBytes; i++)
        {
          printf("%x ", buffer[i]);
        }
      }
      printf("\n");
      break;
    }
  }
}

int main()
{

  char *cmd_str = (char *)malloc(MAX_COMMAND_SIZE);

  int status = 0; // Status help to check if any file system is open or not

  while (1)
  {
    // Print out the mfs prompt
    printf("mfs> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while (!fgets(cmd_str, MAX_COMMAND_SIZE, stdin));
    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    int token_count = 0;
    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;

    char *working_str = strdup(cmd_str);

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;

    // Tokenize the input stringswith whitespace used as the delimiter
    while (((arg_ptr = strsep(&working_str, WHITESPACE)) != NULL) &&
           (token_count < MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup(arg_ptr, MAX_COMMAND_SIZE);
      if (strlen(token[token_count]) == 0)
      {
        token[token_count] = NULL;
      }
      token_count++;
    }
    //For the first time check if user want to open a
    //file system image if not print the message that file
    //system is not open
    if (token[0] == NULL)
    {
      continue;
    }
    if (strcmp(token[0], "exit") == 0)
    {
      exit(0);
    }
    
    if (status == 0) //status 0 means file system is not open
                     //user need to open file system before
                     //using and functionality.
    {
      if (strcmp(token[0], "close") == 0)
      {
        printf("Error: FIle system not opened\n");
      }
      else if (strcmp(token[0], "open") != 0)
      {
        printf("Error: FIle system image must be opened first\n");
      }
      else
      {
        //If user entered open then check if the file system is
        //present or not
        fp = fopen(token[1], "r");
        if (fp == NULL)
        {
          perror("Error");
        }
        else
        {
          status = 1;
        }
      }
    }
    else
    {
      if (strcmp(token[0], "open") == 0) //check if user want to open another file
      {
        printf("Error: FIle system is already open\n"); //display message
      }
      else if (strcmp(token[0], "close") == 0) //closing the file system
      {
        fclose(fp);
        status = 0; //changing status to 0 helps to know if the file system is
                    //open or not
      }
      else
      {

        //reading bytes per sector
        fseek(fp, 11, SEEK_SET);
        fread(&BPB_BytsPerSec, 2, 1, fp);

        //Reading no so sector per unit
        fseek(fp, 13, SEEK_SET);
        fread(&BPB_SecPerClus, 1, 1, fp);

        //Reading no of reserved sector
        fseek(fp, 14, SEEK_SET);
        fread(&BPB_RsvdSecCnt, 2, 1, fp);

        //Reading no of FATs
        fseek(fp, 16, SEEK_SET);
        fread(&BPB_NumFATs, 1, 1, fp);

        //Reading space occuppied by fat
        fseek(fp, 36, SEEK_SET);
        fread(&BPB_FATSz32, 4, 1, fp);

        //Root directory address which is at the first cluster
        //if address value is empty then we are reading the file
        //for the first time
        if (!address)
        {
          address = ((BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec) + (BPB_RsvdSecCnt * BPB_BytsPerSec));
          //Readinf files from root direrctory
          fseek(fp, address, SEEK_SET);
          fread(dir, 16, sizeof(struct DirectoryEntry), fp);
        }
        //printing all the bpb info if user
        //press bpb after they open the file system.
        if (strcmp(token[0], "bpb") == 0)
        {
          printf("BPB_BytsPerSec: %d\n", BPB_BytsPerSec);
          printf("BPB_SecPerClus: %d\n", BPB_SecPerClus);
          printf("BPB_RsvdSecCnt: %d\n", BPB_RsvdSecCnt);
          printf("BPB_NumFATs: %d\n", BPB_NumFATs);
          printf("BPB_FATz32: %d\n", BPB_FATSz32);
        }

        else if (strcmp(token[0], "stat") == 0)
        {
          int i;
          for (i = 0; i < 16; i++)
          {
            //statement to check if there is any unnecessary file that should not
            //be displayed in the directory
            char *tempname;
            tempname = strdup(dir[i].DIR_Name);
            char *name;
            name = strtok(tempname, " ");
            if (strcmp(name, token[1]) == 0)
            {
              printf("File Attribute\tSize\tStarting Cluster Number\n");
              printf("%d\t\t%d\t%d\n", dir[i].DIR_Attr, dir[i].DIR_FileSize, dir[i].DIR_FirstClusterLow);
            }
          }
        }
        else if (strcmp(token[0], "cd") == 0)
        {
          cd(token[1]);
        }
        else if (strcmp(token[0], "ls") == 0)
        {
          int i;
          for (i = 0; i < 16; i++)
          {
            //statement to check if there is any unnecessary file that should not
            //be displayed in the directory
            if (dir[i].DIR_Attr == 0x01 || dir[i].DIR_Attr == 0x10 || dir[i].DIR_Attr == 0x20)
            {
              //To remove extra information after type of file
              //Using strncpy to copy charcter till index 11 and closing
              //the file with null character
              char filename[12];
              strncpy(filename, dir[i].DIR_Name, 11);
              filename[12] = '\0';

              printf("Filename: %s\n", filename);
            }
          }
        }
        else if (strcmp(token[0], "read") == 0)
        {
          readData(token[1], atoi(token[2]), atoi(token[3]));
        }
        else if (strcmp(token[0], "get") == 0)
        {
          int cluster;
          int size;
          int i;
          int found = 0;

          FILE *nfp;
          //See if user want new name or
          //just want old name for the file to
          //get in directory.
          if (token[2] == 0)
          {
            nfp = fopen(token[1], "w");
            if (nfp == NULL)
            {
              printf("Could not open file %s\n", token[1]);
              perror("Error: ");
            }
          }
          else
          {
            nfp = fopen(token[2], "w");
            if (nfp == NULL)
            {
              printf("couldnot open the file %s\n", token[3]);
              perror("Error: ");
            }
          }
          for (i = 0; i < 16; i++)
          {
            char *tempname;
            tempname = strdup(dir[i].DIR_Name);
            char *fname;
            //tokenzing the name just to get
            //name of the file
            fname = strtok(tempname, " ");
            if (strcmp(fname, tempname) == 0)
            {
              cluster = dir[i].DIR_FirstClusterLow;
              found = 1;
              size = dir[i].DIR_FileSize;
              int offset = 0;
              unsigned char buffer[BPB_BytsPerSec];

              while (size >= BPB_BytsPerSec)
              {
                offset = LBAToOffset(cluster);
                fseek(fp, offset, SEEK_SET);

                fread(buffer, 1, BPB_BytsPerSec, fp);
                fwrite(buffer, 1, BPB_BytsPerSec, nfp);

                cluster = NextLB(cluster);
                size = size - BPB_BytsPerSec;
              }

              //Writing remaining part
              //to the file
              if (size)
              {
                offset = LBAToOffset(cluster);
                fseek(fp, offset, SEEK_SET);

                fread(buffer, 1, size, fp); //reading each bytes to buffer
                fwrite(buffer, 1, size, nfp); //writing each bytes from buffer
              }
              fclose(nfp);
              break;
            }
          }
        }
      }
    }
    
  }
  return 0;
}
