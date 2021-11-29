#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <pwd.h>

/* function definitions */
#include "main.h"
#include "help.h"

/* global variables */
char user[256];
char computer[256];
char *homedir;
char *cwd;


/* 
   main
   runs a loop of waiting for input, then executing necessary functions 
   returns 0;
*/
int main() {
  // Setup 
  char inputstring[65536];
  cwd = getcwd(NULL,0);
  if ((homedir = getenv("HOME")) == NULL) {
	homedir = getpwuid(getuid())->pw_dir;
  }
  char *tmp = replace_string(cwd,homedir,"~");
  if (tmp != NULL) {
	free(cwd);
	cwd = tmp;
  }
  
  getlogin_r(user,sizeof(user));
  gethostname(computer,sizeof(computer));

  // Done with setup 	

  while (1) {
	// Prints user in green, directory in yellow
	inputstring[0] = '\0';
    printf("\033[;32m%s \033[;33m%s\033[0m $ ",user,cwd);
    fgets(inputstring,65536,stdin);
	
    char *line = inputstring;
	// Replace newlines (not in quotes) with semicolons
    while(1) {
      char *newline = strchr(line,'\n');
      if (newline == NULL) {break;}
	  
      while (newline != NULL && inquotes(line,newline)) {
		newline = strchr(newline+1,'\n');  
      }
	  // If no closing quote prompt for rest of line 
      if (newline == NULL) {
		printf("> ");
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
    
    line = inputstring;
    while(line && *line) { // Line is not NULL and doesn't point to the null terminator
      char *semicolon;
      semicolon = strsep(&line,";");
      execline(semicolon);
    }
  }
  
  /* free extraneous variables, cleanup */
  free(cwd);
  return 0;
}

/* 
  Exec a line by running parse_args and doing the right stuff
  line is a string to be parsed and executed
  returns exit value of executed line
*/
int execline(char *line) {
	int childstatus;
	char **listargs = parse_args(line);
    if (listargs[0] != NULL && listargs[1] != NULL && !strcmp(listargs[0],"cd")) {
	  if (chdir(listargs[1]) == 0) {
		free(cwd);
	    cwd = getcwd(NULL,0);
		char *tmp = replace_string(cwd,homedir,"~");
		if (tmp != NULL) {
		  free(cwd);
		  cwd = tmp;
		}
	  } else {
		printf("-cjnsh: %s: %s: %s\n",listargs[0],listargs[1],strerror(errno));	
	  }
    } else if (listargs[0] != NULL && !strcmp(listargs[0],"exit")) {
	  exit(0);
    } else if (fork()) {
	  wait(&childstatus);
    } else {
	  do_pipes(listargs);
	  exit(0);
    }
	int i = 0;
	while (listargs[i]) {
	  free(listargs[i]);
	  i++;
	}
  free(listargs);
  return WEXITSTATUS(childstatus);
  
}

/*
  Handles pipes, and redirection 
  Listargs is a array of pointers to strings with NULL at the end
  Returns the return value of the last program run
*/
// Note: only handles one pipe per command, though guidelines say that's fine
int do_pipes(char **listargs) {	
	int rval;
	int length; 
	for (length = 0; listargs[length] != NULL; length++);
	int i;
	for (i = length - 2; i >= 1; i--) {
		if (!strcmp(listargs[i],">") || !strcmp(listargs[i],">>")) {
			int redirectto = open(listargs[i+1],O_WRONLY | O_CREAT | (strlen(listargs[i]) >= 2 ? O_APPEND : O_TRUNC),0755);
			if (errno) {
				printf("-cjnsh: %s: %s\n",listargs[i+1],strerror(errno));
			} else {
				int fd_copystdout = dup(STDOUT_FILENO);	
				// Append if strlen >= 2 (>>)
				dup2(redirectto,STDOUT_FILENO);
				char *temppointer = listargs[i];
				listargs[i] = NULL;
				
				do_pipes(listargs);
				
				listargs[i] = temppointer;
				dup2(fd_copystdout,STDOUT_FILENO);
				close(fd_copystdout);
				close(redirectto);
			}
		} else if (!strcmp(listargs[i],"<")) {
			int redirectfrom = open(listargs[i+1],O_RDONLY);
			if (errno) {
				printf("-cjnsh: %s: %s\n",listargs[i+1],strerror(errno));
			} else {
				int fd_copystdin = dup(STDIN_FILENO);	
				dup2(redirectfrom,STDIN_FILENO);
				char *temppointer = listargs[i];
				listargs[i] = NULL;
				
				do_pipes(listargs);
				
				listargs[i] = temppointer;
				dup2(fd_copystdin,STDIN_FILENO);
				close(fd_copystdin);
				close(redirectfrom);
			}
		} else if (!strcmp(listargs[i],"|")) {
			char *joined1, *joined2;
			joined1 = join(&(listargs[i+1]));
			
			char *temp = listargs[i];
			listargs[i] = NULL;
			joined2 = join(listargs);

			listargs[i] = temp;
			FILE *pipefrom = popen(joined2,"r");
			FILE *pipeto = popen(joined1,"w");
			while (1) {
				if (feof(pipefrom)) {break;}
				fputc(fgetc(pipefrom),pipeto);
			}
			if (pclose(pipefrom) == -1) {
				printf("-cjnsh: %s: %s (Did you type something wrong?)\n",listargs[0],strerror(errno));
			}
			int rval = pclose(pipeto);
			if (rval == -1) {
				printf("-cjnsh: %s: %s (Did you type something wrong?)\n",listargs[i+1],strerror(errno));
			}
			
			free(joined1);
			free(joined2);
			return(rval);
		}
	}
	errno = 0;
	rval = execvp(listargs[0],listargs);
	if (errno) {
		printf("-cjnsh: %s: %s\n",listargs[0],strerror(errno));
	}
	return rval;
}



/* 
  Parses a line of text into a 2d array of arguments (seperates by spaces)
  Line is a null-terminated string
  returns a pointer to an array of strings, with the last element followed by NULL
  Each element of the returned array should be passed into free(), and the array should be as well
*/
// Note: Escaped characters in the last arg of a command arent parsed ,except for quotes 
char **parse_args(char *line) {
	char *strtemp;
	char *ogline = line;
 
	// Replace ~ with user's home directory 
	strtemp = strchr(line,'~');
	while (strtemp != NULL) {
	if (!inquotes(line,strtemp) && (strtemp == line || *(strtemp-1) == ' ') && (*(strtemp+1) == ' ' || *(strtemp+1) == '/' || *(strtemp+1) == '\0')) {
		char *temp = replace_string(line,"~",homedir);
		if (temp != NULL) {
			if (line != ogline) {free(line);}
			line = temp;
		}
		strtemp = strchr(line,'~');
	} else {
		strtemp = strchr(strtemp+1,'~');
	}
  }

  // Split line into args 
  strtemp = line;
  char *strend = strchr(line,'\0');
  char **liststr;
  liststr = malloc(0);
    
  int i = 0;
  strtemp = line;
  while (strtemp < strend && strtemp != NULL) {
    char *strstart = strtemp;
    if (strtemp != line /*&& *(strtemp - 1) == '\0'*/) {
      *(strtemp - 1) = ' ';
    }
	char *quote = min(strchr(strstart,'\''),strchr(strstart,'"'));
    if (strchr(strtemp,' ') != NULL) {	  
	  //If line has space, check whether the next quote is before the space
	  if (quote != NULL && quote < strchr(strstart,' ')) {
		char qtype = (quote == strchr(strstart,'\'')) ? '\'' : '"';
		char *temp = quote;
		char lastquotes = 1; // boolean for backslash check  
		while (*(temp+1) != qtype || *temp == '\\') {
			// If escaped quote
			if (*temp == '\\' && escapeable(*(temp+1)) && lastquotes) {
				//printf("Test: temp: '%c' temp+1: '%c' temp+2: '%c' \n",temp[0],temp[1],temp[2]);
				char *temp2;
				for (temp2 = temp; temp2 > strstart; temp2--) {
					*temp2 = *(temp2 - 1);
				}
				strstart++;
				lastquotes = 0;
			} else {
				lastquotes = 1;
			}
			temp++;
		}
		strtemp = temp+2;
        *(temp+1) = '\0';
		strstart++;
	  } else {
		// If not, just proceed to copy line 
		// but check for escaped characters 
	    char *temp = strsep(&strtemp," ");
		if (temp != NULL) {
			char lastquotes = 1;
			while (*temp) {
				if (*temp == '\\' && lastquotes) {
					printf("Test: temp: '%c' temp+1: '%c' temp+2: '%c' \n",temp[0],temp[1],temp[2]);
					char *temp2;
					for (temp2 = temp; temp2 > strstart; temp2--) {
						*temp2 = *(temp2 - 1);
					}
					strstart++;
					lastquotes = 0;
				} else {
					lastquotes = 1;
				}
				temp++;
			}
		}
	  }
	  // If last token of line, find type of quote and strrchr it, remove them and keep spaces  
    } else {
	  // If quotes are in string 
	  // PARSE ESCAPE CHARACTERS IN THIS 
	  if (quote != NULL) {
		char qtype = (quote == strchr(strstart,'\'')) ? '\'' : '"';
		char *temp = quote;
		*strrchr(strstart,qtype) = '\0';
		strstart++;
	  }
      strtemp = strend;
    } 
    if (strlen(strstart)) {
	  // Reallocate memory, then copy string 
	  liststr = realloc(liststr,(i+1)*8);
      liststr[i] = malloc(strlen(strstart) + 1);
      strcpy(liststr[i],strstart); 
      i++;	  
    }
  }
  liststr = realloc(liststr,(i+1)*8);
  liststr[i] = NULL;
  return liststr;
}
