# cjnsh
My shell for the class Systems Level Programming

## Features
- Semicolon parsing
- Quote handling 
- Ability to escape quotes / other backslashes 
- Replacement of ~ with home directory 
- Pipe
- Redirection to, from files

- user, directory printout at prompt

### Bugs 

- Will seg fault if any token is \\' or \\"
- User entering token as \\\\ does not process into one backslash

## Function List

```c
/* in main.h: */
char **parse_args(char *line);
int execline(char *line);
int do_pipes(char **listargs);
int main();

/* in help.h: */
char *join(char **liststrings);
void *min(void *a, void *b);
int inquotes(char *string, char *ptinstring);
char *replace_string(char *haystack, char *needle, char *toreplace);
int escapeable(char c);
void escapecharacters(char **stringpointer, char quotes);
void *max(void *a, void *b);
```
