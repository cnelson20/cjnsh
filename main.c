#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

char **parse_args(char *line);
int main(int argc, char *argv[]);

int main(int argc, char *argv[]) {
  char **listargs;
  char inputstring[65536];

  while (1) {
    fgets(inputstring,65536,stdin);
    char *newline = strchr(inputstring,'\n');
    while(newline != NULL) {
      *newline = ';';
      newline = strchr(newline+1,'\n');
    }
    printf("Input: \"%s\"\n",inputstring);
    newline = inputstring;
    while(newline != NULL) {
      char *semicolon;
      semicolon = strsep(&newline,";");
      listargs = parse_args(semicolon);
      if (fork()) {
	int childstatus;
	wait(&childstatus);
      } else {
        execvp(listargs[0],listargs);
      }
    }
 }
  return 0;
}

char **parse_args(char *line) {
  char *strend = strchr(line,'\0');
  char *strtemp = line;

  char **liststr;
  int length = 0;
  while (strtemp < strend) {
    char *strstart = strtemp;
    if (strtemp != line) {
      *(strtemp - 1) = ' ';
    }
    if (strchr(strtemp,' ') != NULL) {
      strsep(&strtemp," ");
    } else {
      strtemp = strend;
    }
    length++;
  }
  liststr = malloc((length + 1) * sizeof(char *));
  
  int i = 0;
  strtemp = line;
  while (strtemp < strend) {
    char *strstart = strtemp;
    if (strtemp != line) {
      *(strtemp - 1) = ' ';
    }
    if (strchr(strtemp,' ') != NULL) {
      strsep(&strtemp," ");
    } else {
      strtemp = strend;
    }
    liststr[i] = malloc(strlen(strstart) + 1);
    strcpy(liststr[i],strstart);
    i++;
  }
  liststr[i] = NULL;
  return liststr;
}
