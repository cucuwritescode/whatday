//datebump: write today to a file then commit it
//author: Facundo Franchino

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

//target file sits at the repo root
//launchd sets the cwd before exec
#define TARGET "TODAY"

//halt with errno context; never returns
static void die(const char *what) {
    perror(what);
    exit(1);
}

int main(void) {
    //wall clock in seconds since the epoch
    time_t now = time(NULL);
    if (now == (time_t)-1) die("time");

    //resolve to the user's local timezone
    struct tm *lt = localtime(&now);
    if (!lt) die("localtime");

    //ISO 8601 date: ten characters plus the NUL
    char today[11];
    if (strftime(today, sizeof today, "%Y-%m-%d", lt) == 0) die("strftime");

    //overwrite the file; same content yields the same bytes
    FILE *f = fopen(TARGET, "w");
    if (!f) die("fopen");
    if (fprintf(f, "%s\n", today) < 0) die("fprintf");
    if (fclose(f) != 0) die("fclose");

    //stage the single tracked path
    if (system("git add " TARGET) != 0) die("git add");

    //precondition for commit: a nonempty staged diff
    //exit 0 from --quiet means no diff, so we bail cleanly
    //this makes the programme idempotent within a day
    if (system("git diff --cached --quiet") == 0) return 0;

    //compose the message in a bounded buffer
    //"update to YYYY-MM-DD" plus the wrapping bytes fits well under 64
    char cmd[128];
    int n = snprintf(cmd, sizeof cmd,
        "git commit -m \"update to %s\"", today);
    if (n < 0 || (size_t)n >= sizeof cmd) die("snprintf");

    if (system(cmd) != 0) die("git commit");

    //post upstream so the commit survives the laptop
    if (system("git push") != 0) die("git push");

    return 0;
}
