struct token_struct **parse_args(char *line);
int execline(char *line);
int do_pipes(struct token_struct **listargs);
int main();

struct token_struct {
	char *s;
	int q;
};