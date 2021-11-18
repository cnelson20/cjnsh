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
  char *cwd = getcwd(NULL,0);
  char user[256];
  char computer[256];

  getlogin_r(user,sizeof(user));
  gethostname(computer,sizeof(computer));

  while (1) {
    printf("\033[;32m%s \033[;33m%s\033[0m $ ",user,cwd);
    fgets(inputstring,65536,stdin);
    char *newline = strchr(inputstring,'\n');
    while(newline != NULL) {
      *newline = ';';
      newline = strchr(newline+1,'\n');
    }
    // printf("Input: \"%s\"\n",inputstring);
    newline = inputstring;
    while(newline != NULL) {
      char *semicolon;
      semicolon = strsep(&newline,";");
      listargs = parse_args(semicolon);
	  if (listargs[0] != NULL && listargs[1] != NULL && !strcmp(listargs[0],"cd")) {
		chdir(listargs[1]);
		free(cwd);
		cwd = getcwd(NULL,0);
	  } else if (listargs[0] != NULL && !strcmp(listargs[0],"exit")) {
		exit(0);
	  } else if (fork()) {
		int childstatus;
		wait(&childstatus);
      } else {
        execvp(listargs[0],listargs);
      }
	  free(listargs);
    }
  }
  free(cwd);
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
