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
char prompt[1024];
char *cwd;
char *homedir;

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

  // Done with setup 	

  while (1) {
	// Prints user in green, directory in yellow
	printf("\033[;32m%s \033[;33m%s\033[0m $ ",user,cwd);
    fgets(inputstring,65536, stdin);
	
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
  free(homedir);
  return 0;
}

/* 
  Exec a line by running parse_args and doing the right stuff
  line is a string to be parsed and executed
  returns exit value of executed line
*/
int execline(char *line) {
	int childstatus;
	struct token_struct **listargs = parse_args(line);

    if (listargs[0] != NULL && listargs[1] != NULL && !strcmp(listargs[0]->s,"cd")) {
	  if (chdir(listargs[1]->s) == 0) {
		free(cwd);
	    cwd = getcwd(NULL,0);
		char *tmp = replace_string(cwd,homedir,"~");
		if (tmp != NULL) {
		  free(cwd);
		  cwd = tmp;
		}
	  } else {
		printf("-cjnsh: %s: %s: %s\n",listargs[0]->s,listargs[1]->s,strerror(errno));	
	  }
    } else if (listargs[0] != NULL && !strcmp(listargs[0]->s,"exit")) {
	  if (listargs[1]->s == NULL) {
		exit(0);
	  }
	  int i;
	  sscanf(listargs[1]->s,"%d",&i);
	  exit(i);
    } else if (listargs[0] != NULL) {
		int i = 0;
		while (listargs[i]) {i++;}
		i--;
		if (!strcmp(listargs[i]->s,"&")) {
			free(listargs[i]->s);
			free(listargs[i]);
			listargs[i] = NULL;
			i = 0;
		} else { i = 1; }
		if (fork()) {
			if (i) { wait(&childstatus); }
		} else {
			do_pipes(listargs);
			exit(0); // Do pipes runs exec but just in case 
		}
    }
	int i = 0;
	while (listargs[i]) {
	  if (listargs[i]->s) {free(listargs[i]->s);}
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
int do_pipes(struct token_struct **listargs) {	
	int rval;
	int length; 
	for (length = 0; listargs[length] != NULL; length++);
	int i;
	for (i = length - 2; i >= 1; i--) {
		if (!listargs[i]->q) {
			if (!strcmp(listargs[i]->s,">") || !strcmp(listargs[i]->s,">>")) {
				int redirectto = open(listargs[i+1]->s,O_WRONLY | O_CREAT | (strlen(listargs[i]->s) >= 2 ? O_APPEND : O_TRUNC),0755);
				if (errno) {
					printf("-cjnsh: %s: %s\n",listargs[i+1]->s,strerror(errno));
				} else {
					int fd_copystdout = dup(STDOUT_FILENO);	
					// Append if strlen >= 2 (>>)
					dup2(redirectto,STDOUT_FILENO);
					struct token_struct *temppointer = listargs[i];
					listargs[i] = NULL;
					
					do_pipes(listargs);
					
					listargs[i] = temppointer;
					dup2(fd_copystdout,STDOUT_FILENO);
					close(fd_copystdout);
					close(redirectto);
				}
			} else if (!strcmp(listargs[i]->s,"<")) {
				int redirectfrom = open(listargs[i+1]->s,O_RDONLY);
				if (errno) {
					printf("-cjnsh: %s: %s\n",listargs[i+1]->s,strerror(errno));
				} else {
					int fd_copystdin = dup(STDIN_FILENO);	
					dup2(redirectfrom,STDIN_FILENO);
					struct token_struct *temppointer = listargs[i];
					listargs[i] = NULL;
					
					do_pipes(listargs);
					
					listargs[i] = temppointer;
					dup2(fd_copystdin,STDIN_FILENO);
					close(fd_copystdin);
					close(redirectfrom);
				}
			} else if (!strcmp(listargs[i]->s,"|")) {
				char *joined1, *joined2;
				joined1 = join(&(listargs[i+1]));
				
				struct token_struct *temp = listargs[i];
				listargs[i] = NULL;
				joined2 = join(listargs);

				listargs[i] = temp;
				FILE *pipefrom = popen(joined2,"r");
				FILE *pipeto = popen(joined1,"w");
				while (1) {
					if (feof(pipefrom)) {break;}
					fputc(fgetc(pipefrom),pipeto);
				}
				//if (pclose(pipefrom) == -1) {
				//	printf("-cjnsh: %s: %s (Did you type something wrong?)\n",listargs[0],strerror(errno));
				//}
				int rval = pclose(pipeto);
				if (rval == -1) {
					printf("-cjnsh: %s: %s (Did you type something wrong?)\n",listargs[i+1],strerror(errno));
				}
				
				free(joined1);
				free(joined2);
				return(rval);
			}
		}
	}
	errno = 0;		
	char **strargs = getstringargs(listargs);
	rval = execvp(strargs[0],strargs);
	free(strargs);
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
struct token_struct **parse_args(char *line) {
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
	struct token_struct **liststr;
	char qtype = '\0';
	liststr = malloc(0);
		
	int i = 0;
	strtemp = line;
	while (strtemp < strend && strtemp != NULL) {
		char *strstart = strtemp;
		if (strtemp != line /*&& *(strtemp - 1) == '\0'*/) {
		  *(strtemp - 1) = ' ';
		}
		char *quote = min(strchr(strstart,'\''),strchr(strstart,'"')); // First occurance of ' or ".
		if (strchr(strtemp,' ') != NULL) {	  
			//If line has space, check whether the next quote is before the space
				if (quote != NULL && quote < strchr(strstart,' ')) {
					qtype = (quote == strchr(strstart,'\'')) ? '\'' : '"';
					char *temp = quote;
					char lastquotes = 1; // boolean for backslash check  
					while (*(temp+1) != qtype || *temp == '\\') {
						// If escaped quote
						if (*temp == '\\' && escapeable(*(temp+1)) && lastquotes) {
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
				//printf("'%s'\n",strstart);
				// If not, just proceed to copy line 
				// but check for escaped characters 
				char *temp = strsep(&strtemp," ");
				escapecharacters(&strstart,0);
			}
		// If last token of line, find type of quote and strrchr it, remove them and keep spaces  
		} else {
		  char *qpos = max(strrchr(strstart,'\''),strrchr(strstart,'"'));
		  if (qpos && (qpos == strstart || *(qpos-1) != '\\')) {
			qtype = *qpos;
			strstart = strchr(strstart,qtype)+1;
			*strrchr(strstart,qtype) = '\0';
			escapecharacters(&strstart,1);
		  } else {
			escapecharacters(&strstart,0);
		  }
		  strtemp = strend;
		} 
		if (strlen(strstart)) {
		  // Reallocate memory, then copy string 
		  liststr = realloc(liststr,(i+1)*8);
		  liststr[i] = malloc(sizeof (struct token_struct));
		  liststr[i]->s = malloc(strlen(strstart) + 1);
		  liststr[i]->q = qtype;
		  strcpy(liststr[i]->s,strstart); 
		    
		  i++;	  
		}
	}
	liststr = realloc(liststr,(i+1)*8);
	liststr[i] = NULL;
	return liststr;
}
