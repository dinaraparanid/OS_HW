#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>

#define PROC_MAX 10

/** Holds the scheduling data of one process */

typedef struct {
    int index;
    int arrival_time;
    int initial_burst_time;
    int burst_time;
    int response_time;
    int waiting_time;
    int completion_time;
    int turnaround_time;
    int remaining_burst;
} process_data;

/**
 * The index of the running process.
 * -1 means no running processes
 */
int running_process = -1;

/**
 * The total time of the timer.
 * Should increment one second at a time by the scheduler
 */
unsigned total_time = 0;

/** Data of the processes */
process_data data[PROC_MAX];

/**
 * Array of all process pids.
 * Zero-valued pids - means the process is terminated or not created yet
 */
pid_t proc_pids[PROC_MAX];

/** Size of data array */
unsigned data_size = 0;

void read_file(FILE* const file) {
    while (!feof(file)) {
        process_data d;

        if (fscanf(file, "%d%d%d", &d.index, &d.arrival_time, &d.burst_time) != 3)
            break;

        d.initial_burst_time = d.burst_time;
        d.response_time = 0;
        d.waiting_time = 0;
        d.completion_time = 0;
        d.turnaround_time = 0;
        d.remaining_burst = 0;
        data[data_size++] = d;
    }

    memset(proc_pids, 0, data_size * sizeof(pid_t));
}

int is_pid_taken(const pid_t process) { return process > 0; }

/** Sends desired signal to the worker process */

void try_send(const pid_t process, const int signal) {
    if (is_pid_taken(process)) kill(process, signal);
}

/** Sends signal SIGCONT to the worker process */

void resume(const pid_t process) { try_send(process, SIGCONT); }

/** Sends signal SIGTSTP to the worker process */

void suspend(const pid_t process) { try_send(process, SIGTSTP); }

/** Sends signal SIGTERM to the worker process */

void terminate(const pid_t process) { try_send(process, SIGTERM); }

/** Creates a process using fork */

void create_process(const int new_process) {
    if (running_process != -1)
        suspend(proc_pids[running_process]);

    const pid_t child = fork();

    if (!child) {
        char args_buf[100];
        sprintf(args_buf, "%d", new_process);

        char* const args[] = { "worker", args_buf, NULL };

        if (execv("worker", args) == -1)
            perror("execv");

        return;
    }

    proc_pids[new_process] = child;
}

/** Searches for the next process for running */

process_data* find_next_process() {
    // location of the next process in the data array

    int location = 0;
    int min_at = INT32_MAX;
    int min_at_idx = INT32_MAX;

    for (int i = 0; i < data_size; ++i) {
        if (data[i].completion_time != 0)
            continue;

        if (data[i].arrival_time < min_at) {
            location = i;
            min_at = data[i].arrival_time;
            min_at_idx = data[i].index;
        } else if (data[i].arrival_time == min_at && data[i].index < min_at_idx) {
            location = i;
            min_at_idx = data[i].index;
        }
    }

    // if next_process did not arrive so far,
    // then we recursively call this function after incrementing total_time

    if (data[location].arrival_time > total_time) {
        printf("Scheduler: Runtime: %u seconds.\nProcess %d: has not arrived yet.\n", total_time, location);
        ++total_time;
        return find_next_process();
    }

    return data + location;
}

/** Reports the metrics and simulation results */

void report() {
    puts("Simulation results.....");

    int sum_wt = 0;
    int sum_tat = 0;

    for (int i = 0; i < data_size; ++i) {
        printf("process %d: \n", i);
        printf("	at=%d\n", data[i].arrival_time);
        printf("	bt=%d\n", data[i].initial_burst_time);
        printf("	ct=%d\n", data[i].completion_time);
        printf("	wt=%d\n", data[i].waiting_time);
        printf("	tat=%d\n", data[i].turnaround_time);
        printf("	rt=%d\n", data[i].response_time);
        sum_wt += data[i].waiting_time;
        sum_tat += data[i].turnaround_time;
    }

    printf("data size = %d\n", data_size);
    const float avg_wt = (float) sum_wt / data_size;
    const float avg_tat = (float) sum_tat / data_size;
    printf("Average results for this run:\n");
    printf("	avg_wt=%f\n", avg_wt);
    printf("	avg_tat=%f\n", avg_tat);
}

void check_burst() {
    for (int i = 0; i < data_size; ++i)
        if (data[i].burst_time > 0)
            return;

    // report simulation results
    report();

    // terminate the scheduler ?:{
    exit(EXIT_SUCCESS);
}

void run_new_process(const int process) {
    running_process = process;
    create_process(running_process);

    printf(
            "Scheduler: Starting Process %d (Remaining Time: %d)\n",
            process,
            data[running_process].burst_time
    );

    data[running_process].response_time = total_time - data[running_process].arrival_time;
}

void resume_process(const int process) {
    running_process = process;
    resume(proc_pids[running_process]);

    printf(
            "Scheduler: Resuming Process %d (Remaining Time: %d)\n",
            running_process,
            data[running_process].burst_time
    );
}

/** Called every second as handler for SIGALRM signal */

void schedule_handler(const int signum) {
    // increment the total time
    ++total_time;

    if (running_process != -1) {
        process_data* const running_data = data + running_process;
        --running_data->burst_time;

        printf("Scheduler: Runtime: %d seconds\n", total_time);
        printf("Scheduler: Process %d is running with %d seconds left\n", running_process, running_data->burst_time);

        // wait until done
        if (running_data->burst_time != 0)
            return;

        terminate(proc_pids[running_process]);

        printf(
                "Scheduler: Terminating Process %d (Remaining Time: %d)\n",
                running_process,
                running_data->burst_time
        );

        int res = 0;
        waitpid(proc_pids[running_process], &res, 0);

        running_data->completion_time = total_time;
        running_data->turnaround_time = total_time - running_data->arrival_time;
        running_data->waiting_time = running_data->turnaround_time - running_data->initial_burst_time;

        proc_pids[running_process] = 0;
        running_process = -1;
    }

    // this call will check the bursts of all processes
    check_burst();

    const process_data* next_proc = find_next_process();
    run_new_process(next_proc->index);
}

int main(int argc, char *argv[]) {
    // read the data file
    FILE* const in_file = fopen(argv[1], "r");

    if (in_file == NULL) {
        printf("File is not found or cannot open it!\n");
        exit(EXIT_FAILURE);
    } else {
        read_file(in_file);
    }

    // set a timer
    struct itimerval timer;

    // the timer goes off 1 second after reset
    timer.it_value.tv_sec = 1;
    timer.it_value.tv_usec = 0;

    // timer increments 1 second at a time
    timer.it_interval.tv_sec = 1;
    timer.it_interval.tv_usec = 0;

    // this counts down and sends SIGALRM to the scheduler process after expiration.
    setitimer(ITIMER_REAL, &timer, NULL);

    // register the handler for SIGALRM signal
    signal(SIGALRM, schedule_handler);

    // Wait till all processes finish
    while(1); // infinite loop
}