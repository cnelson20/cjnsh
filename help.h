char *join(struct token_struct **liststrings);
void *min(void *a, void *b);
int inquotes(char *string, char *ptinstring);
char *replace_string(char *haystack, char *needle, char *toreplace);
int escapeable(char c);
void escapecharacters(char **stringpointer, char quotes);
void *max(void *a, void *b);
char **getstringargs(struct token_struct **listargs);