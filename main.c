#include <stdio.h>
#include <unistd.h>
#include <string.h>
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
		chdir(listargs[1]);
		free(cwd);
		cwd = getcwd(NULL,0);
		char *tmp = replace_string(cwd,homedir,"~");
		if (tmp != NULL) {
	      free(cwd);
	      cwd = tmp;
		}
      } else if (listargs[0] != NULL && !strcmp(listargs[0],"exit")) {
		exit(0);
      } else if (fork()) {
		int childstatus;
		wait(&childstatus);
      } else {
		execvp(listargs[0],listargs);
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
char **parse_args(char *line) {
  char *strend = strchr(line,'\0');
  char *strtemp = line;

  char **liststr;
  liststr = malloc(0);
    
  int i = 0;
  strtemp = line;
  while (strtemp < strend && strtemp != NULL) {
    char *strstart = strtemp;
    if (strtemp != line) {
      *(strtemp - 1) = ' ';
    }
    if (strchr(strtemp,' ') != NULL) {	  
	  char *quote = min(strchr(strstart,'\''),strchr(strstart,'"'));	
	  //printf("__%s__ '%s'\n",quote,strstart);
	  if (quote != NULL && (quote < strchr(strstart,' ') || strchr(strstart,' ') == NULL)) {
		char qtype = (quote == strchr(strstart,'\'')) ? '\'' : '"';
		//printf("qtype: %c\n",qtype);
		char *temp = quote;
		while (*(temp+1) != qtype && *temp != '\\') {
			temp++;
		}
		strtemp = temp+2;
        *(temp+1) = '\0';
		strstart++;
	  } else {
	    strsep(&strtemp," ");
	  }
	  // code  
    } else {
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
/* */
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
  memcpy(new,toreplace,strlen(toreplace));
  memcpy(new+sizehalf1+strlen(toreplace),tmpchr+strlen(needle),sizehalf2);
  // Null-terminate string 
  *(new+sizehalf1+strlen(toreplace)+sizehalf2) = '\0';
  //printf("'%s'\n",new);
  return new;
}

void *min(void *a, void *b) {
	if (a == NULL) {
	  return b;	
	} else if (b == NULL) {
	  return a;	
	} else {
	  return a < b ? a : b;	
	}
}