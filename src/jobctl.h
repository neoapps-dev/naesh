#include "naesh.h"
typedef struct {
    pid_t pids[NAESH_MAX_JOBS];
    int pid_count;
    char *cmd_str;
    job_status state;
} naesh_job;
static naesh_job job_table[NAESH_MAX_JOBS];
static int job_count = 0;
static int find_job(pid_t pid) {
    int i;
    int j;
    for (i = 0; i < job_count; i++) {
        for (j = 0; j < job_table[i].pid_count; j++) {
            if (job_table[i].pids[j] == pid) return i;
        }
    }
    return -1;
}
static void handle_sigchld(void) {
    int status;
    pid_t pid;
    int idx;
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        idx = find_job(pid);
        if (idx >= 0) {
            if (WIFSTOPPED(status)) {
                job_table[idx].state = JOB_STOPPED;
            } else {
                job_table[idx].state = JOB_DONE;
            }
        }
    }
}
static void cleanup_jobs(void) {
    int i;
    int j;
    int write_idx;
    write_idx = 0;
    for (i = 0; i < job_count; i++) {
        int still_active;
        still_active = 0;
        for (j = 0; j < job_table[i].pid_count; j++) {
            if (kill(job_table[i].pids[j], 0) == 0) {
                still_active = 1;
                break;
            }
        }
        if (still_active || job_table[i].state == JOB_STOPPED) {
            if (write_idx != i) {
                job_table[write_idx] = job_table[i];
            }
            write_idx++;
        } else {
            free(job_table[i].cmd_str);
        }
    }
    job_count = write_idx;
}
static int add_job(pid_t *pids, int pid_count, const char *cmd, job_status state) {
    int idx;
    int i;
    if (job_count >= NAESH_MAX_JOBS) {
        fprintf(stderr, "naesh: too many jobs\n");
        return -1;
    }
    idx = job_count++;
    job_table[idx].pid_count = pid_count;
    for (i = 0; i < pid_count; i++) {
        job_table[idx].pids[i] = pids[i];
    }
    job_table[idx].cmd_str = strdup(cmd);
    job_table[idx].state = state;
    return idx;
}
