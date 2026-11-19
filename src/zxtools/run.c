/*
 * run.c - execute a sibling tool, falling back to PATH.
 */
#include "zxtools.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static int wait_for(pid_t pid)
{
    int status;

    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) {
            perror("zxtools: waitpid");
            return 1;
        }
    }
    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    if (WIFSIGNALED(status))
        return 128 + WTERMSIG(status);
    return 1;
}

int zxtools_run(const char *self_dir, const char *tool, int argc,
                char *const *argv)
{
    char *path;
    char **child_argv;
    pid_t pid;
    size_t path_len;
    int i;

    path_len = strlen(self_dir) + 1 + strlen(tool) + 1;
    path = malloc(path_len);
    child_argv = calloc((size_t)argc + 2, sizeof(*child_argv));
    if (path == NULL || child_argv == NULL) {
        fprintf(stderr, "zxtools: out of memory\n");
        free(path);
        free(child_argv);
        return 1;
    }

    snprintf(path, path_len, "%s/%s", self_dir, tool);
    child_argv[0] = (char *)tool;
    for (i = 0; i < argc; i++)
        child_argv[i + 1] = (char *)argv[i];
    child_argv[argc + 1] = NULL;

    pid = fork();
    if (pid < 0) {
        perror("zxtools: fork");
        free(path);
        free(child_argv);
        return 1;
    }
    if (pid == 0) {
        execv(path, child_argv);
        execvp(tool, child_argv);
        fprintf(stderr, "zxtools: %s not found (looked in %s and PATH)\n",
                tool, self_dir);
        _exit(127);
    }

    free(path);
    free(child_argv);
    return wait_for(pid);
}
