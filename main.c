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
	  if (listargs[1] == NULL) {
		exit(0);
	  }
	  int i;
	  sscanf(listargs[1],"%d",&i);
	  exit(i);
    } else {
		int i = 0;
		while (listargs[i]) {i++;}
		i--;
		if (!strcmp(listargs[i],"&")) {
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
	errno = 0;
	rval = execvp(listargs[0],listargs);
	if (errno) {
		printf("-cjnsh: %s: %s\n",listargs[0],strerror(errno));
	}
	return rval;
}

/*
  Joins list of strings with spaces 
  Each string is enclosed with quotes
  Returned value should be passed into free()
*/
char *join(char **liststrings) {
	int i, totalstringlength;
	totalstringlength = 0;
	// Calculate total length of string by mallocing it.
	for (i = 0; liststrings[i] != NULL; i++) {
		char *j = liststrings[i];
		if (i != 0) {totalstringlength++;} // space inbetween strings
		totalstringlength += 2; // begin & end "
		while (*j) {
			totalstringlength += ((*j == '\'' || *j == '"') ? 2 : 1);
			j++;
		}
	}
	char *joined = malloc(totalstringlength+1);
	// Combine strings in joined
	char *tcp = joined;
	for (i = 0; liststrings[i] != NULL; i++) {
		char *j = liststrings[i];
		if (i != 0) {*tcp = ' '; tcp++;} // space inbetween strings
		*tcp = '"'; tcp++;
		while (*j) {
			if (*j == '\\' || *j == '\'' || *j == '"') {
				*tcp = '\\';
				tcp++;
			}
			// Copy char, increment pointer 
			*tcp = *j;
			tcp++;
			j++;
		}
		*tcp = '"'; tcp++;
	}
	joined[totalstringlength] = '\0';
	return joined;
}

/* 
  Parses a line of text into a 2d array of arguments (seperates by spaces)
  Line is a null-terminated string
  returns a pointer to an array of strings, with the last element followed by NULL
  Each element of the returned array should be passed into free(), and the array should be as well
*/
// Note: You can escape quotes / other backslashes but the backslashes still show up, fix this
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
		while (*(temp+1) != qtype || *temp == '\\') {
			// If escaped quote
			if (*temp == '\\') {
				//printf("Test: temp: '%c' temp+1: '%c' temp+2: '%c' \n",temp[0],temp[1],temp[2]);
				char *temp2;
				for (temp2 = temp; temp2 > strstart; temp2--) {
					*temp2 = *(temp2 - 1);
				}
				strstart++;
			}
			temp++;
		}
		strtemp = temp+2;
        *(temp+1) = '\0';
		strstart++;
	  } else {
		// If not, just proceed to copy line 
	    strsep(&strtemp," ");
	  }
	  // If last token of line, find type of quote and strrchr it, remove them and keep spaces  
    } else {
	  // If quotes are in string 
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

/* 
  Determines whether a character within a string is in quotes (single or double) 
  String points to a null-terminated string of characters, ptinstring is a pointer to a char within that string
  Returns 1 / 0 (True / False)
*/
int inquotes(char *string, char *ptinstring) {
  int qlvl = 0;
  char qtype = 0;
  char *p = string;
  char *temp = strchr(string,'\'');
  // If neither ' or " in string return 0
  if (temp == NULL || temp > ptinstring) {
    temp = strchr(string,'"');
    if (temp == NULL || temp > ptinstring) {
      return 0;
    }
  }	
  /*printf("string: '%s'\n",string); */
  while (p < ptinstring && p != NULL) {
    char *q = min(strchr(p,'\''),strchr(p,'"'));
    if (q == NULL || q >= ptinstring) {break;}
    if (qlvl == 0) {
	  // If is first byte of string or last byte is not backslash
      if (q == string || *(q-1) != '\\') {
	    qtype = *q;
	    qlvl = 1;
      }
    } else {
	  // If match ' vs " and is first byte or does not proceed a backslash
      if (*q == qtype && (q == string || *(q-1) != '\\')) {
	    qlvl = 0;
      }
    }
    p = q + 1;
    //printf("qlvl: %d\n",qlvl);
	
  }
  return qlvl;
}

/* 
  Finds needle in haystack and returns a new string where it has been replaced by toreplace
  Returns a pointer to a new string, NULL if needle was not found.
  Caller should free() the old string
*/
char *replace_string(char *haystack, char *needle, char *toreplace) {
  //printf("'%s' '%s' '%s'\n",haystack,needle,toreplace);
  char *tmpchr, *tmpndl, *lpchr;
  tmpchr = haystack;
  while (*tmpchr) {
    lpchr = tmpchr;
    tmpndl = needle;
	
	// while letters of haystack and needle match
    while (*lpchr == *tmpndl) {
      lpchr++;
      tmpndl++;
	  // if end of needle (success), break
      if (!(*tmpndl)) {
	    break;
      }
	  // if end of string reached, return null
      if (!(*lpchr)) {
	    return NULL;
      }
    }
	// if end of while loop was because end of needle, break
    if (!(*tmpndl)) {
      break;
    }
    tmpchr++;
  }
  // if end of string was reached return NULL
  if (tmpchr - haystack >= strlen(haystack)) {
    return NULL;
  }
  char *half2, *new;
  int sizehalf1, sizehalf2, sizenew;
  // Calculate sizes of string before and after matching part
  sizehalf1 = tmpchr - haystack;
  sizehalf2 = strlen(haystack) - (tmpchr - haystack) - strlen(needle);
  // calc size of new string 
  sizenew = sizehalf1 + sizehalf2 + strlen(toreplace);
  //printf("sizehalf1: %d sizehalf2: %d sizenew: %d\n",sizehalf1,sizehalf2,sizenew);
  new = malloc(sizenew+1);
  // Copy half before, what to replace, and half after 
  memcpy(new,haystack,sizehalf1);
  memcpy(new+sizehalf1,toreplace,strlen(toreplace));
  memcpy(new+sizehalf1+strlen(toreplace),tmpchr+strlen(needle),sizehalf2);
  // Null-terminate string 
  //printf("sizehalf1: %d strlen(toreplace): %d sizehalf2: %d\n",sizehalf1,strlen(toreplace),sizehalf2);
  *(new+sizehalf1+strlen(toreplace)+sizehalf2) = '\0';
  //printf("Haystack: '%s'\nNew: '%s'\n",haystack,new);
  return new;
}

/* 
  Returns the minimum of two pointers unless one is null, then return the other 
  (if both are null, returns null)
  a and b are (possibly null) pointers
*/
void *min(void *a, void *b) {
	//return (a == NULL) ? b : ((b == NULL) ? a : (a < b ? a : b));
	if (a == NULL) {
	  return b;	
	} else if (b == NULL) {
	  return a;	
	} else {
	  return a < b ? a : b;	
	}
}