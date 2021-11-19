#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <pwd.h>

/* function definitions */
char **parse_args(char *line);
int inquotes(char *string, char *ptinstring);
char *replace_string(char *haystack, char *needle, char *toreplace);
void *min(void *a, void *b);
int main();

/* global variables */
char user[256];
char computer[256];
char *homedir;

/* 
   main
   runs a loop of waiting for input, then executing necessary functions 
   returns 0;
*/
int main() {
	
  char **listargs;
  char inputstring[65536];
  char *cwd = getcwd(NULL,0);
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
    
    line = inputstring;
    while(line != NULL) {
      char *semicolon;
      semicolon = strsep(&line,";");
      listargs = parse_args(semicolon);
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
		int childstatus;
		wait(&childstatus);
      } else {
		execvp(listargs[0],listargs);
		printf("-cjnsh: %s: %s\n",listargs[0],strerror(errno));	
		exit(0);
      }
	  int i = 0;
	  while (listargs[i]) {
		free(listargs[i]);
		i++;
	  }
	  free(listargs);
	  
    }
  }
  
  /* free extraneous variables, cleanup */
  free(cwd);
  return 0;
}

/* 
  Parses a line of text into a 2d array of arguments (seperates by spaces)
  returns a pointer to an array of strings, with the last element followed by NULL
  Each element should be passed into free()
*/
// For reference: You can escape quotes but the backslashes still show up, fix this
char **parse_args(char *line) {
  char *strtemp;
  char *ogline = line;

  strtemp = strchr(line,'~');
  while (strtemp != NULL) {
  if (!inquotes(line,strtemp) && (strtemp == line || *(strtemp-1) == ' ') &&(*(strtemp+1) == ' ' || *(strtemp+1) == '\0')) {
	  char *temp = replace_string(line,"~",homedir);
	  if (temp != NULL) {
	    if (line != ogline) {
		  free(line);
		}
		line = temp;
	  }
	  strtemp = strchr(line,'~');
	} else {
		strtemp = strchr(strtemp+1,'~');
	}
  }
  /*if (strlen(ogline)) {
	printf("Line: '%s'\n",ogline);
	printf("Line: '%s'\n",line);
  }*/

  strtemp = line;
  char *strend = strchr(line,'\0');
  char **liststr;
  liststr = malloc(0);
    
  int i = 0;
  strtemp = line;
  while (strtemp < strend && strtemp != NULL) {
    char *strstart = strtemp;
    if (strtemp != line) {
      *(strtemp - 1) = ' ';
    }
	char *quote = min(strchr(strstart,'\''),strchr(strstart,'"'));
    if (strchr(strtemp,' ') != NULL) {	  
	  //If line has space, check whether the next quote is before the space
	  if (quote != NULL && quote < strchr(strstart,' ')) {
		char qtype = (quote == strchr(strstart,'\'')) ? '\'' : '"';
		char *temp = quote;
		while (*(temp+1) != qtype && *temp != '\\') {
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
    /*printf("q: '%s'\n",q);*/
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
*/
void *min(void *a, void *b) {
	if (a == NULL) {
	  return b;	
	} else if (b == NULL) {
	  return a;	
	} else {
	  return a < b ? a : b;	
	}
}