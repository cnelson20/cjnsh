#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#define MIN(a,b) (a != NULL ? (a < b ? a : b) : b) 

/* function definitions */
char **parse_args(char *line);
int inquotes(char *string, char *ptinstring);
int main(int argc, char *argv[]);

/* global variables */
char user[256];
char computer[256];
;
int main(int argc, char *argv[]) {
	
  char **listargs;
  char inputstring[65536];
  char *cwd = getcwd(NULL,0);

  getlogin_r(user,sizeof(user));
  gethostname(computer,sizeof(computer));

  while (1) {
    printf("\033[;32m%s \033[;33m%s\033[0m $ ",user,cwd);
    fgets(inputstring,65536,stdin);
	
	char *line = inputstring;
    while(1) {
	  char *newline = strchr(line,'\n');
	  if (newline == NULL) {break;}
	  
	  while (newline != NULL && inquotes(line,newline)) {
		newline = strchr(newline+1,'\n');  
	  }
	  if (newline == NULL) {
		fgets(inputstring + strlen(inputstring),65536 - strlen(inputstring),stdin);
		newline = line;
	    continue;
	  }
	  //printf("'%s'\n",inputstring);
	  //char *test = inputstring;
	  //while (*test) {printf("%hhx-%d ",*test,inquotes(line,test));test++;}
	  *newline = ';';
	  
	  line = newline + 1; 
    }
    
	printf("'%s'\n",inputstring);
    line = inputstring;
    while(line != NULL) {
      char *semicolon;
      semicolon = strsep(&line,";");
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
	// Add code to parse quotes 
	if (strlen(strstart)) {
      liststr[i] = malloc(strlen(strstart) + 1);
      strcpy(liststr[i],strstart);
      i++;
	}
  }
  liststr[i] = NULL;
  return liststr;
}
int inquotes(char *string, char *ptinstring) {
	int qlvl = 0;
	char qtype = 0;
	char *p = string;
	char *temp = strchr(string,'\'');
	if (temp == NULL || temp > ptinstring) {
		temp = strchr(string,'"');
		if (temp == NULL || temp > ptinstring) {
			return 0;
		}
	}	
	//printf("string: '%s'\n",string);
	while (p < ptinstring && p != NULL) {
		char *q = MIN(strchr(p,'\''),strchr(p,'"'));
		if (q == NULL || q >= ptinstring) {break;}
		//printf("q: '%s'\n",q);
		if (qlvl == 0) {
			if (q == string || *(q-1) != '\\') {
				qtype = *q;
				qlvl = 1;
			}
		} else {
			if (*q == qtype && (q == string || *(q-1) != '\\')) {
				qlvl = 0;
			}
		}
		p = q + 1;
		//printf("qlvl: %d\n",qlvl);
	}
	return qlvl;
}