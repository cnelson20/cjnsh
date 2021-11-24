char **parse_args(char *line);
int inquotes(char *string, char *ptinstring);
char *replace_string(char *haystack, char *needle, char *toreplace);
int execline(char *line);
int do_pipes(char **listargs);
char *join(char **liststrings);
void *min(void *a, void *b);
int main();