/* regex.c -- Demonstrate regular expressions
   by Zhaolong Zhu

-------------------------------------------------------------------------------

.       Matches any single character (including newline)
[list]  Matches a single character contained in the brackets.
        Ranges using "-" are supported; "-" is a literal if first or last.
        Negate by beginning with ^ inside [ ].
^       Matches the starting position.  MUST USE if want to match whole string.
$       Matches the ending position.  MUST USE if want to match whole string.
\       Escape character; a following metacharacter loses its meaning.
        Use \\ for backslash itself.
(...)   Subexpression; treat the entire expression as one expression.
|       Alternative.  "cat|dog" matches either "cat" or "dog".

*       Matches the preceding element zero or more times.
+       Matches the preceding element one or more times.
?       Matches the preceding element zero or one times ("optional")
{m,n}   Matches the preceding element at least m and not more than n times.
{m}     Matches the preceding element exactly m times.
 */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

#define clean_errno() (errno == 0 ? "None" : strerror(errno))
#define log_err(M, ...) fprintf(stderr, "[ERROR] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)
#define check(A, M, ...) if(!(A)) { log_err(M, ##__VA_ARGS__); errno=0; goto error; }
#define check_mem(A) check((A), "Out of memory.")

#define LINE_BUF_SIZE 1024

struct TestCase {
    int regex_idx;
    char *str;
    int isMatch;
};

void free_lines(char **lines, int size) {
    for (int i = 0; i < size; i++)
        free(lines[i]);
    free(lines);
}

char **read_lines(char *filepath, int *size) {
    char buf[LINE_BUF_SIZE];
    FILE *f;
    f = fopen(filepath, "r");
    check(f != NULL, "file open error: %s", filepath);
    int cap = 10;
    char **lines = (char **)malloc(sizeof(char *) * (cap));
    check_mem(lines);

    int i = 0;
    int len;
    while (fgets(buf, LINE_BUF_SIZE, f) != NULL) {
        if (i == cap) {
            int new_cap = cap * 2;
            char **tmp= (char **)realloc(lines, sizeof(char *) * new_cap);
            check_mem(tmp);
            lines = tmp;
            cap = new_cap;
        }
        len = strlen(buf);
        lines[i] = (char *)malloc(sizeof(char) * (len + 1));
        check_mem(lines[i]);
        strncpy(lines[i], buf, len-1);
        lines[i][len-1] = '\0';
        i++;
    }
    if (f) fclose(f);
    *size = i;
    return lines;

error:
    free_lines(lines, i);
    if (f) fclose(f);
    *size = 0;
    return NULL;
}

void free_test_cases(struct TestCase **tests, int size) {
    for (int i = 0; i < size; i++) {
        if (tests[i]->str) free(tests[i]->str);
        free(tests[i]);
    }
    free(tests);
}

struct TestCase **parse_test_cases(char **lines, int size) {
    struct TestCase **rs = (struct TestCase **)malloc(sizeof(struct TestCase*) * size);
    check_mem(rs);
    char *line;
    char *token;
    char *sep = " ";
    int i = 0;
    for (; i < size; i++) {
        struct TestCase *t = (struct TestCase *)malloc(sizeof(struct TestCase));
        check_mem(t);
        rs[i] = t;
        line = lines[i];
        token = strtok(line, sep);
        t->regex_idx = atoi(token);
        token = strtok(NULL, sep);
        t->str = strdup(token);
        token = strtok(NULL, sep);
        t->isMatch = atoi(token);
    }
    return rs;
error:
    free_test_cases(rs, i);
    return NULL;
}

int main() {
    int reti;
    int retcode = 1;
    char msgbuf[100];

    /* Read patterns */
    int p_size;
    char **patterns = read_lines("pattern.txt", &p_size);

    /* Compile regular expression */
    regex_t *regexs = (regex_t *)malloc(sizeof(regex_t) * p_size);
    check_mem(regexs);
    int regcomp_cnt;
    for (int i=0; i < p_size; i++) {
        reti = regcomp(&regexs[i], patterns[i], REG_EXTENDED | REG_NOSUB);
        if (reti) {
            fprintf(stderr, "Could not compile regex: %s\n", patterns[i]);
            regcomp_cnt = i;
            goto error;
        }
    }

    /* Read test cases */
    int t_size;
    char **test_lines = read_lines("test.txt", &t_size);
    struct TestCase **tests = parse_test_cases(test_lines, t_size);

    /* Execute regular expression */
    for (int i=0; i < t_size; i++) {
        struct TestCase *t = tests[i];
        reti = regexec(&regexs[t->regex_idx], t->str, 0, NULL, 0);
        if ((t->isMatch && reti == 0) || (!t->isMatch && reti)) {
            regerror(reti, &regexs[t->regex_idx], msgbuf, sizeof(msgbuf));
            fprintf(stderr, "[Success] regex %d %s: %s\n", t->regex_idx, t->str, msgbuf);
        } else {
            regerror(reti, &regexs[t->regex_idx], msgbuf, sizeof(msgbuf));
            fprintf(stderr, "[Failed] regex %d %s: %s\n", t->regex_idx, t->str, msgbuf);
        }
    }
    retcode = 0;
error:
    if (regexs) {
        for(int i = 0; i < regcomp_cnt; i++)
            regfree(&regexs[i]);
        free(regexs);
    }
    if (patterns) free_lines(patterns, p_size);
    if (test_lines) free_lines(test_lines, t_size);
    if (tests) free_test_cases(tests, t_size);
    return retcode;
}
