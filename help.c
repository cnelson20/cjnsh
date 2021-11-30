#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "main.h"
#include "help.h"

/*
  Joins list of strings with spaces 
  Each string is enclosed with quotes
  Returned value should be passed into free()
*/
char *join(struct token_struct **liststrings) {
	int i, totalstringlength;
	totalstringlength = 0;
	// Calculate total length of string by mallocing it.
	for (i = 0; liststrings[i] != NULL; i++) {
		char *j = liststrings[i]->s;
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
		char *j = liststrings[i]->s;
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
      if (q == string || *(q-1) != '\\' || (q >= string+2 && *(q-2) == '\\')) {
	    qtype = *q;
	    qlvl = 1;
      }
    } else {
	  // If match ' vs " and is first byte or does not proceed a backslash
      if (*q == qtype && (q == string || *(q-1) != '\\' || (q >= string+2 && *(q-2) == '\\'))) {
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
   Returns 1 if c is \, ', or " .
   Returns 0 otherwise
*/
int escapeable(char c) {
	return (c == '"' || c == '\'' || c == '\\');
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

/* 
  Returns the greater of two pointers
  a and b are pointers 
*/
void *max(void *a, void *b) {
	return a > b ? a : b;
}


/* 
  Escapes characters following backslashes
  stringpointer is a pointer to a string 
  quotes is a boolean on whether the characters escapeable should be limited 
*/
void escapecharacters(char **stringpointer, char quotes) {
	/*printf("escapecharacters():\n");
	printf("input: '%s'\n",*stringpointer);*/
	char lastquotes = 1;
	char *temp = *stringpointer;
	while (*temp) {
		if (*temp == '\\' && lastquotes && (!quotes || escapeable(temp[1]))) {
			char *temp2;
			for (temp2 = temp; temp2 > *stringpointer; temp2--) {
				*temp2 = *(temp2 - 1);
			}
			(*stringpointer)++;
			lastquotes = 0;
		} else {
			lastquotes = 1;
		}
		//printf("test\n");
		temp++;
	}
	/*printf("output: '%s'\n",*stringpointer);*/
}	

/* 
	Takes an array of struct token_struct pointers and returns an array of each token_struct's string.
	listargs is a array of pointers to token_struct structs. 
*/
char **getstringargs(struct token_struct **listargs) {
	int i;
	for (i = 1; listargs[i]; i++);
	char **liststrings = malloc(sizeof(char *) * i);
	for (i = 0; listargs[i]; i++) {
		liststrings[i] = listargs[i]->s;
	}
	liststrings[i] = NULL;
	return liststrings;
}