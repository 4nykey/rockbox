#include <time.h>
#include <stdbool.h>

/* struct tm defined */
struct tm *get_time(void);
int set_time(struct tm *tm);
bool valid_time(struct tm *tm);
