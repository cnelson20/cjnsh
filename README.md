# cjnsh
My shell for the class Systems Level Programming

## Features
- Semicolon parsing
- Quote handling 
- Special characters (~, |, >, >>, <) are not parsed when in quotes
- Ability to escape characters
- Replacement of ~ with home directory 
- Pipe
- Redirection to, from files

- user, directory printout at prompt

## Bugs / Issues

- Pipes & redirects must be their own token
- User entering token as `"\\"` or `'\\'` (any even number of backslashes > 0) does not process correctly (quotes stay)

- Control-C does terminate the program (Not a bug just want to note this)

## Function List

```c
/* in main.h: */
struct token_struct **parse_args(char *line);
int execline(char *line);
int do_pipes(struct token_struct **listargs);
int main();

/* in help.h: */
char *join(struct token_struct **liststrings);
void *min(void *a, void *b);
int inquotes(char *string, char *ptinstring);
char *replace_string(char *haystack, char *needle, char *toreplace);
int escapeable(char c);
void escapecharacters(char **stringpointer, char quotes);
void *max(void *a, void *b);
char **getstringargs(struct token_struct **listargs);

/* custom struct (in main.h): */
struct token_struct {
        char *s;
        int q;
};
```
