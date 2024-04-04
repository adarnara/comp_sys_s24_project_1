
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <signal.h>
#include <math.h>
#include <limits.h>
#include <errno.h>




const int MAX_ELEM_NUM = 256;
const int NUM_ARGS = 3;
const int MAX_CHILDREN_OPTIONS = 3;
const int MAX_RANDOM_RANGE = 10000;
const int MAX_RANDOM_OFFSET = 60;
const int CHILD_INCREMENT = 5;
const int SLEEP_WITH_100 = 100;
const int DEFAULT_CHILD_SLEEP_SHORT = 20;
const int PIPE_DATA_SIZE = 160;
const int PIPE_DATA_ELEMENTS = 3;
const int STATUS_SUCC = 0;

const char *KEY_VALUE_FILE_NAME = "key_value_file_pt2.txt";
const char *OUTPUT_FILE_NAME = "output_pt2.txt";
const char *READ_MODE = "r";
const char *WRITE_MODE = "w";
const char *FILE_MODE_APPEND = "a+";

const int INIT_VAL= -1;
const int SLEEP_TIME = 3;

int L, H, PN;
bool is_signal_rec = false;

const char *PROCESS_INFO_FORMAT = "Hi I'm process %d with return arg %d and my parent is %d.\n";
const char *PROMPT_MSG = "Please enter the number (it must be 2 or 3)?\n";
const char *RESULT_MSG_FORMAT = "Hi I am Process %d with return arg %d and I found the hidden key at position A[%d].\n";
const char *RESULT_MSG_MAX_MIN = "Max=%d, Avg =%lld.\n";


void
setup_signal_handling();

void
command_line_reader(char *const *argv);

int
ask_user_prompt(int *max);

void *
mem_allocation(size_t num_elements, size_t element_size);

void
input_reader(FILE *file, int *data_values, char *line);


void
writer(int **proc_info, int *str_pos, const int *fd, int *curr_max, long long int *curr_avg, int *counter, pid_t *c_pid, int fd_size);

void
calc_avg_cnt(int *proc_info, const int *data_values, int starting_point, int ending_point, int status_code,
             int *str_pos, int *max, int *cnt, long long int *avg);


void
rule_1_2_3(int child_arg, int max_a, int max_b, pid_t t_pid, int str);


void
custom_signal_handler(int sig) { is_signal_rec = true; }


void
kill_c_proc(pid_t pid, int signal)
{
    int status;
    printf("Terminating process ID %d\n", pid);
    kill(pid, signal);


    pid_t terminated_pid = waitpid(pid, &status, 0);


    if (terminated_pid > 0) {
        if (WIFSIGNALED(status)) {
            printf("Process %d was terminated by signal %d\n", terminated_pid, WTERMSIG(status));
        }
        else if (WIFEXITED(status)) {
            printf("Process %d exited with status %d\n", terminated_pid, WEXITSTATUS(status));
        }
        else {
            printf("Process %d terminated with unknown status\n", terminated_pid);
        }
    }
    else {
        printf("Error waiting for process %d\n", pid);
    }
}

void
create_output_text_file()
{
    int *rand_nums = (int *)mem_allocation((L + MAX_RANDOM_OFFSET), sizeof(int));
    for (int i = 0; i < L + MAX_RANDOM_OFFSET; i++) {
        rand_nums[i] = 0;
    }
    int count = 0;
    srand(time(NULL));
    int randomNumber;
    while (count < MAX_RANDOM_OFFSET) {
        randomNumber = (rand() % (L + MAX_RANDOM_OFFSET));
        if (rand_nums[randomNumber] == 0) {
            rand_nums[randomNumber] = -(rand() % MAX_RANDOM_OFFSET + 1);
            count++;
        }
    }
    FILE* keys = fopen(KEY_VALUE_FILE_NAME, WRITE_MODE);
    for (int i = 0; i < (L + MAX_RANDOM_OFFSET); i++) {
        if (rand_nums[i] < 0 && rand_nums[i] > -MAX_RANDOM_OFFSET) {
            fprintf(keys, "%d\n", rand_nums[i]);
        }
        else {
            randomNumber = rand() % MAX_RANDOM_RANGE;
            fprintf(keys, "%d\n", randomNumber);
        }
    }
    fclose(keys);
    free(rand_nums);
}

void
reader(int fd[], const int child_processes[], int index, int *temp_max, int *temp_counter,
       long long *temp_avg, pid_t *tempPID, int temp_arr[], int *temp_h_beg)
{
    read(fd[2 * child_processes[index] + 1], temp_max, sizeof(int));
    read(fd[2 * child_processes[index] + 1], temp_avg, sizeof(int64_t));
    read(fd[2 * child_processes[index] + 1], temp_counter, sizeof(int));
    read(fd[2 * child_processes[index] + 1], temp_arr, sizeof(int) * PIPE_DATA_SIZE);
    read(fd[2 * child_processes[index] + 1], temp_h_beg, sizeof(int));
    read(fd[2 * child_processes[index] + 1], tempPID, sizeof(pid_t));

}


int
main(int argc, char* argv[])
{
    double delta_time = 0.0;
    clock_t time_beg = clock();

    if (argc != 4)
    {
        printf("need to add more args. Should be in this order -> L H PN");
        return -INIT_VAL;
    }

    command_line_reader(argv);
    int user_input_num_of_max_children;
    int parentRoot = getpid();

    int proc_info[PIPE_DATA_SIZE];
    for (int i = 0; i < PIPE_DATA_SIZE; i++) {
        proc_info[i] = INIT_VAL;
    }
    int str_pos = 0;
    user_input_num_of_max_children = ask_user_prompt(&user_input_num_of_max_children);

    if (user_input_num_of_max_children != 2 && user_input_num_of_max_children != 3) {
        printf("error, the entered number must be 2 or 3\n");
        return -1;
    }

    create_output_text_file();

    FILE *opened_file = fopen(KEY_VALUE_FILE_NAME, READ_MODE);
    FILE* output_txt_file;
    fclose(fopen(OUTPUT_FILE_NAME, WRITE_MODE));

    int* data_values = (int*)mem_allocation((L + MAX_RANDOM_OFFSET), sizeof(int));
    char* line = (char*)mem_allocation(MAX_ELEM_NUM, sizeof(char));

    input_reader(opened_file, data_values, line);

    const int FD_SIZE = 2 * PN;
    pid_t pid;
    pid_t child_gen_process_pid;
    int depth_with_log2 = floor(log2(PN));
    int child_proc_arg = 1;
    int starting_point = 0;
    int ending_point = L + MAX_RANDOM_OFFSET;
    int child_cnt = -1;
    int fd[FD_SIZE];

    int child_processes[MAX_CHILDREN_OPTIONS];
    for (int i = 0; i < MAX_CHILDREN_OPTIONS; i++) {
        child_processes[i] = INIT_VAL;
    }

    int p_comm_pipe = INIT_VAL;
    int prev_ending_point = 0;
    int status_code;

    for (int pos = 0; pos < depth_with_log2; pos++) {
        if (pos != 0) {
            if (user_input_num_of_max_children == 2){
                child_proc_arg = 2 * (child_proc_arg) - 1;
            }
            else{
                child_proc_arg = 3 * (child_proc_arg) - 2;
            }
        }
        child_gen_process_pid = getpid();
        int offset = ceil((ending_point - starting_point) / user_input_num_of_max_children);
        prev_ending_point = ending_point;
        ending_point = starting_point;
        bool is_detected = false;
        for (int i = 0; i < user_input_num_of_max_children; i++) {
            if (child_gen_process_pid == getpid() && child_proc_arg < PN) {
                child_cnt++;
                for (int l = 0; l < user_input_num_of_max_children; l++) {
                    if (child_processes[l] < 0 && child_processes[l] > -MAX_RANDOM_OFFSET) {
                        child_processes[l] = child_proc_arg - 1;
                        break;
                    }
                }
                pipe(&fd[2 * (child_proc_arg - 1)]);
                pid = fork();
                if (i != 0) {
                    starting_point = starting_point + offset;
                }
                ending_point = ending_point + offset;
                if ((L + MAX_RANDOM_OFFSET) - ending_point < CHILD_INCREMENT) {
                    ending_point = L + MAX_RANDOM_OFFSET;
                }
                child_proc_arg = child_proc_arg + 1;
                status_code = child_proc_arg;
            }
        }
        if (pid == 0) {
            for (int l = 0; l < user_input_num_of_max_children; l++) {
                if (!(child_processes[l] < 0 && child_processes[l] > -MAX_RANDOM_OFFSET)) {
                    p_comm_pipe = child_processes[l];
                    child_processes[l] = -(rand() % MAX_RANDOM_OFFSET + 1);
                }
            }

            output_txt_file = fopen(OUTPUT_FILE_NAME, FILE_MODE_APPEND);
            printf(PROCESS_INFO_FORMAT, getpid(), child_proc_arg, getppid());
            fprintf(output_txt_file, PROCESS_INFO_FORMAT, getpid(), child_proc_arg, getppid());
            fclose(output_txt_file);
            pid = getpid();
            if (pos == (depth_with_log2 - 1)) {
                int curr_max = 0;
                long long curr_avg = 0;
                int counter = 0;
                for (int i = starting_point; i < ending_point; i++) {
                    if (data_values[i] > curr_max) curr_max = data_values[i];
                    curr_avg += data_values[i];

                    if (data_values[i] < 0 && data_values[i] > -MAX_RANDOM_OFFSET) {
                        proc_info[str_pos] = getpid();
                        proc_info[str_pos + 1] = i;
                        proc_info[str_pos + 2] = status_code;
                        str_pos = str_pos + PIPE_DATA_ELEMENTS;
                    }
                }

                curr_avg = curr_avg / (ending_point - starting_point);
                counter = ending_point - starting_point;
                pid_t childPID = getpid();
                const int fd_size = 2 * p_comm_pipe;
                close(fd[fd_size]);
                writer(&proc_info, &str_pos, fd, &curr_max, &curr_avg, &counter, &childPID, fd_size);

                setup_signal_handling();
                raise(SIGCONT);
                sleep(SLEEP_TIME);
            }
        }
        else {
            int temp_counter = INIT_VAL;
            bool is_tracked = false;
            long long avg = 0;
            long long temp_avg = INIT_VAL;
            int buffer[PIPE_DATA_SIZE];
            int temp_buff = 0;
            bool has_child = false;
            int num_of_children = 0;
            int max = 0;
            int tempMax = INIT_VAL;
            int cnt = 0;

            for (int i = 0; i < user_input_num_of_max_children; i++) {
                if (!(child_processes[i] < 0 && child_processes[i] > -MAX_RANDOM_OFFSET)) {
                    has_child = true;
                    num_of_children++;
                }

                else if ((child_processes[i] < 0 && child_processes[i] > -MAX_RANDOM_OFFSET) && has_child && !is_tracked) {
                    starting_point = ending_point;
                    ending_point = prev_ending_point;
                    is_tracked = true;
                }
            }

            if (!has_child) {
                ending_point = prev_ending_point;
            }

            if (num_of_children == 0) {
                calc_avg_cnt(proc_info, data_values, starting_point, ending_point, status_code, &str_pos, &max, &cnt, &avg);
            }
            else if (num_of_children < user_input_num_of_max_children) {
                calc_avg_cnt(proc_info, data_values, starting_point, ending_point, status_code, &str_pos, &max, &cnt, &avg);

                for (int index = 0; index < num_of_children; index++) {
                    pid_t tempPID;
                    int tempH[PIPE_DATA_SIZE];
                    int tempHstart = 0;
                    reader(fd, child_processes, index, &tempMax, &temp_counter, &temp_avg, &tempPID, tempH, &tempHstart);

                    int nodesToAdd = tempHstart / PIPE_DATA_ELEMENTS < 2 ? tempHstart / PIPE_DATA_ELEMENTS : 2;
                    for (int i = 0; i < nodesToAdd; ++i) {
                        if (str_pos < PIPE_DATA_SIZE - PIPE_DATA_ELEMENTS) {
                            proc_info[str_pos++] = tempH[i * PIPE_DATA_ELEMENTS];
                            proc_info[str_pos++] = tempH[i * PIPE_DATA_ELEMENTS + 1];
                            proc_info[str_pos++] = tempH[i * PIPE_DATA_ELEMENTS + 2];
                        }
                    }
                    rule_1_2_3(status_code, max, tempMax, tempPID, tempHstart);

                    avg = (avg * cnt + temp_avg * temp_counter) / (temp_counter + cnt);

                    if (tempHstart >= 2) {
                        for (int i = 0; i < 3; i++) {
                            proc_info[str_pos] = tempH[i];
                            str_pos++;
                        }
                    }
                }
            }
            else {
                pid_t pid1, pid2;
                int tracker;
                for (int r = 0; r < num_of_children; r++) {
                    if (r != 0) {
                        reader(fd, child_processes, r, &tempMax, &temp_counter, &temp_avg,
                               &pid2, buffer, &temp_buff);

                        rule_1_2_3(status_code, max, tempMax, temp_buff, pid1);

                        avg = (avg * cnt + temp_avg * temp_counter) / (temp_counter + cnt);

                        if (temp_buff >= 2) {
                            for (int i = 0; i < 3; i++) {
                                proc_info[str_pos] = buffer[i];
                                str_pos++;
                            }
                        }
                    }
                    else {
                        reader(fd, child_processes, r, &max, &cnt, &avg,
                               (pid_t *) &buffer, &temp_buff, &pid1);
                        if (temp_buff >= 2) {
                            for (int i = 0; i < 3; i++) {
                                proc_info[str_pos] = buffer[i];
                                str_pos++;
                            }
                        }
                        tracker = str_pos;
                    }
                }
                kill(pid1, SIGCONT);
            }

            if (parentRoot != getpid()) {
                pid_t childPID = getpid();
                writer((int **) proc_info, &str_pos, fd, &max, &avg, &cnt, &childPID, 2 * p_comm_pipe);

            }
            else {
                wait(NULL);
                output_txt_file = fopen(OUTPUT_FILE_NAME, FILE_MODE_APPEND);
                printf(RESULT_MSG_MAX_MIN, max, avg);
                fprintf(output_txt_file, RESULT_MSG_MAX_MIN, max, avg);
                for (int i = 0; i < user_input_num_of_max_children * 3; i += 3) {
                    printf(RESULT_MSG_FORMAT, proc_info[i], proc_info[i + 2], proc_info[i + 1]);
                    fprintf(output_txt_file, RESULT_MSG_FORMAT, proc_info[i], proc_info[i + 2], proc_info[i + 1]);
                }
                fclose(output_txt_file);
                clock_t time_ended = clock();
                delta_time += (double)(time_ended - time_beg) / CLOCKS_PER_SEC;
                printf("Elapsed Time: %f [s]\n", delta_time);
                exit(STATUS_SUCC);
            }

            setup_signal_handling();
            raise(SIGCONT);
            sleep(SLEEP_TIME);
            if (is_signal_rec) {
                sleep(SLEEP_WITH_100);
            }
            else {
                sleep(DEFAULT_CHILD_SLEEP_SHORT);
            }

            exit(status_code);
        }
    }
}




void
rule_1_2_3(int child_arg, int max_a, int max_b, pid_t t_pid, int str)
{
    // rule 1
    if (max_b > max_a) {
        kill(t_pid, SIGCONT);
        sleep(SLEEP_WITH_100);
        exit(child_arg);
    }
        // rule 2
    else if (H <= (str+1) / PIPE_DATA_ELEMENTS) {
        kill_c_proc(t_pid, SIGKILL);
    }
        // rule 3
    else {
        kill(t_pid, SIGCONT);
        sleep(SLEEP_TIME);
        kill(t_pid, SIGINT);
        sleep(SLEEP_TIME);
        kill(t_pid, SIGQUIT);
    }
}


void
calc_avg_cnt(int *proc_info, const int *data_values, int starting_point,
             int ending_point, int status_code,
             int *str_pos, int *max, int *cnt, long long int *avg)
{
    for (int i = starting_point; i < ending_point; i++) {
        if (data_values[i] > (*max)) (*max) = data_values[i];
        (*avg) += data_values[i];
        if (data_values[i] < 0 && data_values[i] > -MAX_RANDOM_OFFSET) {
            proc_info[(*str_pos)] = getpid();
            proc_info[(*str_pos) + 1] = i;
            proc_info[(*str_pos) + 2] = status_code;
            (*str_pos) = (*str_pos) + PIPE_DATA_ELEMENTS;
        }
    }
    (*avg) = (*avg) / (ending_point - starting_point);
    (*cnt) = ending_point - starting_point;
}

void
writer(int **proc_info, int *str_pos, const int *fd,
       int *curr_max, long long int *curr_avg, int *counter,
       pid_t *c_pid, const int fd_size)
{
    write(fd[fd_size], curr_max, sizeof(int));
    write(fd[fd_size], curr_avg, sizeof(int64_t));
    write(fd[fd_size], counter, sizeof(int));
    write(fd[fd_size], proc_info, sizeof(int) * PIPE_DATA_SIZE);
    write(fd[fd_size], str_pos, sizeof(int));
    write(fd[fd_size], c_pid, sizeof(pid_t));
    close(fd[fd_size]);

}


void
setup_signal_handling()
{
    struct sigaction sa;
    sa.sa_handler = &custom_signal_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGCONT, &sa, NULL);
}


void
input_reader(FILE *file, int *data_values, char *line)
{
    for (int i = 0; i < (L + MAX_RANDOM_OFFSET); i++) {
        if (fgets(line, MAX_ELEM_NUM, file) != NULL) {
            char *end_ptr;
            long int data = strtol(line, &end_ptr, 10);
            if (end_ptr == line || *end_ptr != '\n') {
                printf("error, check inputs\n");
                exit(EXIT_FAILURE);
            }
            data_values[i] = (int)data;
        }
        else {
            printf("error, reading from file\n");
            exit(EXIT_FAILURE);
        }
    }
}

int
ask_user_prompt(int *max)
{
    printf("%s", PROMPT_MSG);
    scanf("%d", max);
    return (*max);
}



void
command_line_reader(char *const *argv)
{
    char *end_ptr;
    long val;

    for (int i = 1; i <= NUM_ARGS; ++i) {
        errno = 0;
        val = strtol(argv[i], &end_ptr, 10);

        if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN))
            || (errno != 0 && val == 0)) {
            perror("strtol");
            exit(EXIT_FAILURE);
        }

        if (end_ptr == argv[i] || *end_ptr != '\0') {
            fprintf(stderr, "error, no digits were found\n");
            exit(EXIT_FAILURE);
        }

        if (val < INT_MIN || val > INT_MAX) {
            fprintf(stderr, "error, integer overflow or underflow\n");
            exit(EXIT_FAILURE);
        }

        if (i == 1 && val <= 10000) {
            fprintf(stderr, "error, L must be greater than 10000\n");
            exit(EXIT_FAILURE);
        }
        else if (i == 2 && (val < 30 || val > 60)) {
            fprintf(stderr, "error, H must be between 30 and 60\n");
            exit(EXIT_FAILURE);
        }

        switch(i) {
            case 1: L = (int)val; break;
            case 2: H = (int)val; break;
            case 3: PN = (int)val; break;
            default: break;
        }
    }
}


void *
mem_allocation(size_t num_elements, size_t element_size)
{
    void* ptr = calloc(num_elements, element_size);
    if (ptr == NULL) {
        printf("error, mem allocation failed\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}