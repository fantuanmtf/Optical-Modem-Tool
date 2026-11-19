#ifndef ZXTOOLS_H
#define ZXTOOLS_H

#include <stdio.h>

void zxtools_usage(FILE *fp);
int zxtools_run(const char *self_dir, const char *tool, int argc,
                char *const *argv);
void zxtools_menu(const char *self_dir);

#endif /* ZXTOOLS_H */
