#ifndef _LOG_H_
#define _LOG_H_

extern FILE *gfp_log;

void log_init(char *log_path);
void log_uninit();
#endif
