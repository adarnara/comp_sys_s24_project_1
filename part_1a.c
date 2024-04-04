/*
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

const int MAX_LINE_LENGTH = 256;
const int NUM_HIDDEN_KEYS = 60;
const int KEY_RANGE = 10000;
//const int KEY_SHIFT = 60;
const char *FILE_NAME_KEYS = "input_value_file_pt1_a.txt";
const char *FILE_NAME_OUTPUT = "output_pt1_a.txt";
const char *OUTPUT_MODE = "a+";
const char *FOUND_KEY_MSG = "Hi I am Process %d with return argument %d and I found the hidden key at position A[%d].\n";
const char *MEM_ALLOC_ERR_MSG = "allocation failed\n";
const char *ARGS_ERR_MSG = "error, need more arguments\n";
const char *TIME_TAKEN_MSG = "Elapsed time %Lf [s]\n";

const char *MAX_AVG_OUTPUT_STRING = "Max=%d, Avg =%Lf\n";


int L, H, PN;
long double elapsed_time = 0.0;
clock_t time_beg;

int
*setup_initializer(char *const *argv);

void
child_process_helper(FILE *output, const int *array_buffer, const int *fd, const int *bd, int i, int *start, int *end,
                     int *return_arg);

void
parent_process_helper(long double time_spent, clock_t time_beg, FILE *output, int *array_buffer, const int *fd, const int *bd,
                      int start, int end, int parent_root, int returnArg, int i);

void
parent_process_handler(long double time_spent, clock_t time_beg, FILE *output, int *array_buffer, const int *fd, const int *bd,
                       int i, int *max, long double *avg, int *count);

void
data_aggregation(const int *array_buffer, const int *fd, const int *bd, int start, int end, int i, int *max, long double *avg,
                 int *count);


void
curr_fd_writer(const int *fd, int i, int *max, long double *avg, int *count);


void
fd_reader(const int *fd, int i, int *max, long double *avg, int *count);

void
bd_fd_closer(const int *fd, const int *bd, int i);

void
prev_curr_fd_bd_closer(const int *fd, const int *bd, int i);

void
prev_close_reader(const int *fd, const int *bd, int i);


void *
mem_allocation(size_t num_elements, size_t size_per_element)
{
    void* ptr = calloc(num_elements, size_per_element);
    if (ptr == NULL) {
        fprintf(stderr, "%s", MEM_ALLOC_ERR_MSG);
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void
create_key_text_file()
{
    int* key_pos_set = mem_allocation((L + NUM_HIDDEN_KEYS), sizeof(int));
    for (int i = 0; i < L + NUM_HIDDEN_KEYS; i++) key_pos_set[i] = 0;

    int count = 0;
    srand(time(NULL));
    int rand_num = 0;

    while (count < NUM_HIDDEN_KEYS) {
        rand_num = (rand() % (L + NUM_HIDDEN_KEYS));
        if (key_pos_set[rand_num] == 0) {
            key_pos_set[rand_num] = -(rand() % NUM_HIDDEN_KEYS + 1);
            count++;
        }
    }

    FILE* keys;
    keys = fopen(FILE_NAME_KEYS, "w");

    for (int i = 0; i < (L + NUM_HIDDEN_KEYS); i++) {
        if (key_pos_set[i] < 0) {
            fprintf(keys, "%d\n", key_pos_set[i]);
        } else {
            rand_num = rand() % KEY_RANGE;
            fprintf(keys, "%d\n", rand_num);
        }
    }

    fclose(keys);
    free(key_pos_set);
}

void
parent_process_helper(long double time_spent, clock_t time_beg, FILE *output, int *array_buffer, const int *fd, const int *bd,
                      int start, int end, int parent_root, int returnArg, int i)
{
    long double avg = 0;
    long double avg_temp;
    int max = 0;
    int max_temp = 0;
    int count = 0;
    int counter_temp = 0;

    if (parent_root != getpid()) {
        data_aggregation(array_buffer, fd, bd, start, end, i, &max, &avg, &count);

        fd_reader(fd, i, &max_temp, &avg_temp, &counter_temp);
        close(fd[2 * i]);

        if (max_temp > max) {
            max = max_temp;
        }
        if (counter_temp + count > 0) {
            avg = (avg * count + avg_temp * counter_temp) / (long double)(counter_temp + count);
        }

        curr_fd_writer(fd, i-1, &max, &avg, &count);
        prev_close_reader(fd, bd, i);

        output = fopen(FILE_NAME_OUTPUT, OUTPUT_MODE);
        for (int i = start; i < end; i++) {
            if (H != 0 && array_buffer[i] <= -1 && array_buffer[i] >= -NUM_HIDDEN_KEYS) {
                printf(FOUND_KEY_MSG, getpid(), returnArg, i);
                fprintf(output, FOUND_KEY_MSG, getpid(), returnArg, i);
                H--;
            }
        }
        fclose(output);
        write(bd[2 * i + 1], &H, sizeof(int));
        close(bd[2 * i + 1]);
        wait(NULL);
        exit(0);
    }
    else {
        parent_process_handler(time_spent, time_beg, output, array_buffer, fd, bd, i, &max, &avg, &count);
    }
}



void
fd_reader(const int *fd, int i, int *max, long double *avg, int *count)
{
    read(fd[2 * i], max, sizeof(int));
    read(fd[2 * i], avg, sizeof(long double)); // use long double to avoid precision issue
    read(fd[2 * i], count, sizeof(int));
}


void
curr_fd_writer(const int *fd, int i, int *max, long double *avg, int *count)
{
    write(fd[2 * i + 1], max, sizeof(int));
    write(fd[2 * i + 1], avg, sizeof(long double)); // Change data type to long double
    write(fd[2 * i + 1], count, sizeof(int));
}
void
bd_fd_closer(const int *fd, const int *bd, int i)
{
    close(bd[2 * i]);
    close(fd[2 * i + 1]);
}

void
prev_curr_fd_bd_closer(const int *fd, const int *bd, int i)
{
    close(fd[2 * i] + 1);
    close(fd[2 * (i - 1)]);
    close(bd[2 * i]);
    close(bd[2 * (i - 1) + 1]);
}
void prev_close_reader(const int *fd, const int *bd, int i) {
    close(fd[2 * (i - 1) + 1]);

    read(bd[2 * (i - 1)], &H, sizeof(int));
    close(bd[2 * (i - 1)]);
}

void
data_aggregation(const int *array_buffer, const int *fd, const int *bd,
                 int start, int end, int i, int *max, long double *avg, int *count)
{
    prev_curr_fd_bd_closer(fd, bd, i);

    (*count) = 0;
    for (int j = start; j < end; j++) {
        if (array_buffer[j] >= 0) {
            if (array_buffer[j] > (*max)) (*max) = array_buffer[j];
            (*avg) += array_buffer[j];
            (*count)++;
        }
    }
    if ((*count) > 0) (*avg) /= (*count);
}



void
parent_process_handler(long double time_spent, clock_t time_beg, FILE *output,
                       int *array_buffer, const int *fd, const int *bd,
                       int i, int *max, long double *avg, int *count){
    bd_fd_closer(fd, bd, i);
    fd_reader(fd, i, max, avg, count);
    close(fd[2 * i]);
    output = fopen(FILE_NAME_OUTPUT, OUTPUT_MODE);
    fprintf(output, MAX_AVG_OUTPUT_STRING, (*max), (*avg)); // use long double to avoid precision issue
    fclose(output);
    printf(MAX_AVG_OUTPUT_STRING, (*max), (*avg)); // use long double to avoid precision issue
    write(bd[2 * i + 1], &H, sizeof(int));
    close(bd[2 * i + 1]);
    wait(NULL);
    clock_t end = clock();
    time_spent += (long double)(end - time_beg) / CLOCKS_PER_SEC; // use long double to avoid precision issue
    printf(TIME_TAKEN_MSG, time_spent);
    free(array_buffer);
    exit(0);
}


int
calculate_max(const int *data, int start, int end){
    int max = 0;
    for (int i = start; i < end; i++) {
        if (data[i] >= 0 && data[i] > max)
        {
            max = data[i];
        }
    }
    return max;
}

long double
calculate_average(const int *data, int start, int end, int *counter){
    if(data == NULL) return 0.0;
    long double sum = 0;
    for (int j = start; j < end; j++)
    {
        if (data[j] >= 0) {
            sum += data[j];
            (*counter)++;
        }
    }
    if (*counter > 0) {
        return sum / (*counter);
    } else {
        return 0;
    }
}


void
child_process_helper(FILE *output, const int *array_buffer, const int *fd, const int *bd,
                     int i, int *start, int *end, int *return_arg){
    output = fopen(FILE_NAME_OUTPUT, OUTPUT_MODE);
    printf(FOUND_KEY_MSG, getpid(), (*return_arg) + 1, getppid());
    fprintf(output, FOUND_KEY_MSG, getpid(), (*return_arg) + 1, getppid());
    (*return_arg)++;
    (*start) = (*end);
    (*end) = (*end) + (L + NUM_HIDDEN_KEYS) / PN;

    if ((*end) > (L + NUM_HIDDEN_KEYS))
    {
        (*end) = (L + NUM_HIDDEN_KEYS);
    }

    if ((PN - 1) == i) {
        int max = calculate_max(array_buffer, *start, *end);

        int counter = 0;
        long double avg = calculate_average(array_buffer, *start, *end, &counter);

        curr_fd_writer(fd, i, &max, &avg, &counter);
        close(fd[2 * i + 1]);

        read(bd[2 * i], &H, sizeof(int));
        close(bd[2 * i]);

        FILE *child_output = fopen(FILE_NAME_OUTPUT, OUTPUT_MODE);
        for (int j = *start; j < *end; j++) {
            if (H != 0 && array_buffer[j] >= -NUM_HIDDEN_KEYS && array_buffer[j] <= -1)
            {
                printf(FOUND_KEY_MSG, getpid(), (*return_arg), j);
                fprintf(child_output, FOUND_KEY_MSG, getpid(), (*return_arg), j);
                H--;
            }
        }
        fclose(child_output);

        exit(0);
    }
}


int *
setup_initializer(char *const *argv){
    char *end_ptr;
    long temp_long;

    temp_long = strtol(argv[1], &end_ptr, 10);
    if (*end_ptr != '\0') return NULL;
    L = (int)temp_long;

    temp_long = strtol(argv[2], &end_ptr, 10);
    if (*end_ptr != '\0') return NULL;
    H = (int)temp_long;

    temp_long = strtol(argv[3], &end_ptr, 10);
    if (*end_ptr != '\0') return NULL;
    PN = (int)temp_long;

    create_key_text_file();

    FILE *file = fopen(FILE_NAME_KEYS, "r");
    fclose(fopen(FILE_NAME_OUTPUT, "w"));
    int *array_buffer = mem_allocation((L + NUM_HIDDEN_KEYS), sizeof(int));

    char line[MAX_LINE_LENGTH];
    for (int i = 0; i < (L + NUM_HIDDEN_KEYS); i++)
    {
        if (fgets(line, sizeof(line), file) == NULL) break;
        temp_long = strtol(line, &end_ptr, 10);
        if (*end_ptr != '\n' && *end_ptr != '\0') return NULL;
        array_buffer[i] = (int)temp_long;
    }

    fclose(file);
    return array_buffer;
}

int
main(int argc, char* argv[]){
    time_beg = clock();
    if (argc != 4)
    {
        printf("%s", ARGS_ERR_MSG);
        return -1;
    }

    FILE *output = NULL;
    int *array_buffer = setup_initializer(argv);
    int fd[2 * (PN)];
    int bd[2 * (PN)];

    pid_t pid;
    int starting_point = 0;
    int ending_point = 0;

    int parentRoot = getpid();
    int returnArg = 1;

    for (int i = 0; i < PN; i++)
    {
        int index = 2*i;
        pipe(&fd[index]);
        pipe(&bd[index]);

        pid = fork();

        if (pid == 0)
        {
            child_process_helper(output, array_buffer, fd, bd, i, &starting_point, &ending_point, &returnArg);

        }
        else
        {
            parent_process_helper(elapsed_time, time_beg, output, array_buffer, fd, bd, starting_point, ending_point, parentRoot, returnArg, i);
        }
    }
}
*/

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>


const int ARRAY_BUFFER_SIZE = 160;
char *const INPUT_VALUE_FILE_NAME = "input_value_file_pt1_b.txt";
char *const OUTPUT_FILE_NAME = "output_pt1_b.txt";
char *const WRITE_MODE = "w";
char *const READ_MODE = "r";
const int MAX_ELEM_NUM = 256;
char *const FILE_MODE_APPEND = "a+";
const int MAX_RANDOM_RANGE = 10000;
const int MAX_RANDOM_OFFSET = 60;
const char *PROCESS_INFO_FORMAT = "Hi I'm process %d with return arg %d and my parent is %d.\n";
char *const RESULT_MSG_MAX_MIN = "Max: %d, Avg: %f\n\n";
char *const RESULT_MSG_FORMAT = "Hi I am Process %d with return argument %d and I found the ""hidden key at position A[%d].\n";
char *const PROMPT_MSG = "How Many Children Do You Want (Must Choose: 2 or 3)?\n";
static char *const TIME_TAKEN_MSG = "Elapsed time %f [s]\n";
const int INIT_VAL = 1;

const int MAX_CHILDREN_OPTIONS = 3;
const int MIN_CHILDREN_OPTIONS = 2;
int L, H, PN;
int *fd;
int data_array[ARRAY_BUFFER_SIZE];
double delta_time = 0.0;
clock_t time_beg;

void*
mem_allocation_using_calloc(size_t num_elements, size_t element_size);

void
generate_text_file();

int
get_input(char *const *argv, int *max_child, int *root_parent, int *h_start);

void
read_initialize_in_out_files(int **array_buffer, int *result, int *return_arg);

void
create_child_processes(int max_child, pid_t make_child, int count_child, const int *fd, int *track_child, int incre,
                       int *return_arg, pid_t *pid, int *start, int *end, int *child_return);

void
append_output_file(FILE *output, int return_arg);

void
parent_pipe_write(int **h, int *h_start, const int *fd, int parent_pipe, int *max, double *avg, int *count);

int
get_return_arg_from_child(int max_child, int return_arg);

int
iter_to_track_child_pid(int max_child, int *track_child_pid);

void
store_child_return_in_buffer(int *h, const int *array_buffer, int start, int end, int child_return,
                             int *h_start, int *max, double *avg);

void
track_start_and_end(int max_child, const int *track_child_pid, int initial_end, bool tracked, int *start,
                    int *end, bool *has_child, int *child_count);

void
local_avg_from_child_calc(int *h, const int *array_buffer, int start, int end, int child_return,
                          int *h_start, int *max, double *avg, int *count);

void
read_from_buffer_and_get_avg(const int *fd, const int *track_child_pid, int count, int *temp_max, double *temp_avg, int r,
                             int *max, double *avg, int *temp_count, int **temp_h, int *temp_h_start);

int
fill_in_keys_from_child(int *h, int h_start, const int *temp_h, int temp_h_start);

void
setup_read(const int *fd, const int *track_child_pid, int r, int **h, int *h_start, int *max, double *avg, int *count);

void
setup_write(int **h, int *h_start, const int *fd, int parent_pipe, int *max, double *avg, int *count);

void
print_output_to_file(const int *h, FILE *output, int max, double avg);

void
process_children_to_parent_for_max_avg_output(int max_child, int root_parent, int *h_start, const int *array_buffer, FILE *output,
                                              int result, int *return_arg, int count_child, pid_t *pid,
                                              pid_t make_child, int *start, int *end, int *track_child_pid,
                                              int parent_pipe,int initial_end, int *child_return);



void*
mem_allocation_using_calloc(size_t num_elements, size_t element_size)
{
    void* ptr = calloc(num_elements, element_size);
    if (ptr == NULL)
    {
        fprintf(stderr, "error, mem allocation failed\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void
generate_text_file()
{
    int* vars = mem_allocation_using_calloc(L + MAX_RANDOM_OFFSET, sizeof(int));
    for (int i = 0; i < L + MAX_RANDOM_OFFSET; i++) vars[i] = 0;

    int count = 0;
    srand(time(NULL));
    int random_number;

    while (count < MAX_RANDOM_OFFSET)
    {
        random_number = (rand() % (L + MAX_RANDOM_OFFSET));
        if (vars[random_number] == 0)
        {
            vars[random_number] = -(rand() % MAX_RANDOM_OFFSET + INIT_VAL);
            count++;
        }
    }

    FILE* input;
    input = fopen(INPUT_VALUE_FILE_NAME, WRITE_MODE);

    for (int i = 0; i < (L + MAX_RANDOM_OFFSET); i++)
    {

        if (vars[i] < 0)
        {
            fprintf(input, "%d\n", vars[i]);
        }
        else
        {
            random_number = rand() % MAX_RANDOM_RANGE;
            fprintf(input, "%d\n", random_number);
        }
    }

    fclose(input);
    free(vars);
}

int
get_input(char *const *argv, int *max_child, int *root_parent, int *h_start)
{
    long temp;
    char *char_endptr;

    (*root_parent) = getpid();
    (*h_start) = 0;

    temp = strtol(argv[INIT_VAL], &char_endptr, 10);
    if (*char_endptr != '\0' || char_endptr == argv[INIT_VAL] || temp > INT_MAX || temp < INT_MIN)
    {
        fprintf(stderr, "incorrect number for L: %s\n", argv[INIT_VAL]);
        return EXIT_FAILURE;
    }
    L = (int)temp;

    temp = strtol(argv[2], &char_endptr, 10);
    if (*char_endptr != '\0' || char_endptr == argv[2] || temp > INT_MAX || temp < INT_MIN)
    {
        fprintf(stderr, "incorrect number for H: %s\n", argv[2]);
        return EXIT_FAILURE;
    }
    H = (int)temp;

    temp = strtol(argv[3], &char_endptr, 10);
    if (*char_endptr != '\0' || char_endptr == argv[3] || temp > INT_MAX || temp < INT_MIN)
    {
        fprintf(stderr, "incorrect number for PN: %s\n", argv[3]);
        return EXIT_FAILURE;
    }
    PN = (int)temp;

    for (int i = 0; i < ARRAY_BUFFER_SIZE; i++) data_array[i] = -INIT_VAL;

    printf("%s", PROMPT_MSG);
    if (scanf("%d", max_child) != INIT_VAL)
    {
        fprintf(stderr, "couldn't read childs.\n");
        return EXIT_FAILURE;
    }

    if (!(*max_child >= MIN_CHILDREN_OPTIONS && *max_child <= MAX_CHILDREN_OPTIONS))
    {
        fprintf(stderr, "invalid child number -> 2 or 3.\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void
read_initialize_in_out_files(int **array_buffer, int *result, int *return_arg)
{
    (*array_buffer) = mem_allocation_using_calloc(L + MAX_RANDOM_OFFSET, sizeof(int));
    (*result) = floor(log2(PN));
    (*return_arg) = INIT_VAL;
    FILE* file = fopen(INPUT_VALUE_FILE_NAME, READ_MODE);
    char* line = mem_allocation_using_calloc(MAX_ELEM_NUM, sizeof(char));
    fclose(fopen(OUTPUT_FILE_NAME, WRITE_MODE));
    for (int i = 0; i < (L + MAX_RANDOM_OFFSET); i++) {
        fgets(line, sizeof(line), file);
        (*array_buffer)[i] = atoi(line);
    }
}

void
create_child_processes(int max_child, pid_t make_child, int count_child, const int *fd, int *track_child, int incre,
                       int *return_arg, int *pid, int *start, int *end, int *child_return)
{
    for (int i = 0; i < max_child; i++)
    {
        if (make_child == getpid() && (*return_arg) < PN)
        {
            count_child++;

            for (int l = 0; l < max_child; l++)
            {
                if (track_child[l] == -INIT_VAL)
                {
                    track_child[l] = (*return_arg) - INIT_VAL;
                    break;
                }
            }

            pipe(&fd[2 * ((*return_arg) - INIT_VAL)]);


            (*pid) = fork();
            if (i != 0) (*start) = (*start) + incre;


            (*end) = (*end) + incre;
            if ((L + MAX_RANDOM_OFFSET) - (*end) < 5) (*end) = L + MAX_RANDOM_OFFSET;


            (*return_arg) = (*return_arg) + INIT_VAL;
            (*child_return) = (*return_arg);
        }
    }
}

void
append_output_file(FILE *output, int return_arg)
{
    output = fopen(OUTPUT_FILE_NAME, FILE_MODE_APPEND);


    printf(PROCESS_INFO_FORMAT, getpid(), return_arg, getppid());


    fprintf(output, PROCESS_INFO_FORMAT, getpid(), return_arg, getppid());

    fclose(output);
}

void
parent_pipe_write(int **h, int *h_start, const int *fd, int parent_pipe, int *max, double *avg, int *count)
{
    close(fd[2 * parent_pipe]);


    write(fd[2 * parent_pipe + INIT_VAL], max, sizeof(int));
    write(fd[2 * parent_pipe + INIT_VAL], avg, sizeof(long));
    write(fd[2 * parent_pipe + INIT_VAL], count, sizeof(int));


    write(fd[2 * parent_pipe + INIT_VAL], h, sizeof(int) * ARRAY_BUFFER_SIZE);
    write(fd[2 * parent_pipe + INIT_VAL], h_start, sizeof(int));

    close(fd[2 * parent_pipe + INIT_VAL]);
}

int
get_return_arg_from_child(int max_child, int return_arg)
{
    if (max_child == 2)
    {
        return_arg = 2 * (return_arg) - INIT_VAL;
    }
    else if (max_child == 3)
    {
        return_arg = 3 * (return_arg) - 2;
    }
    else
    {
        return_arg = 4 * (return_arg) - 3;
    }
    return return_arg;
}

int
iter_to_track_child_pid(int max_child, int *track_child_pid)
{
    int parent_pipe;
    for (int l = 0; l < max_child; l++)
    {
        if (track_child_pid[l] != -INIT_VAL)
        {
            parent_pipe = track_child_pid[l];
            track_child_pid[l] = -INIT_VAL;
        }
    }
    return parent_pipe;
}

void
store_child_return_in_buffer(int *h, const int *array_buffer, int start, int end, int child_return, int *h_start,
                             int *max, double *avg)
{
    for (int j = start; j < end; j++)
    {
        if (array_buffer[j] > (*max)) (*max) = array_buffer[j];
        (*avg) += array_buffer[j];

        if (array_buffer[j] <= -INIT_VAL && array_buffer[j] >= -MAX_RANDOM_OFFSET)
        {
            h[(*h_start)] = getpid();
            h[(*h_start) + INIT_VAL] = j;
            h[(*h_start) + 2] = child_return;
            (*h_start) = (*h_start) + 3;
        }
    }
}

void
track_start_and_end(int max_child, const int *track_child_pid, int initial_end, bool tracked, int *start, int *end,
                    bool *has_child, int *child_count) {
    for (int r = 0; r < max_child; r++)
    {
        if (track_child_pid[r] != -INIT_VAL)
        {
            (*has_child) = true;
            (*child_count)++;
        }
        else if (track_child_pid[r] == -INIT_VAL && (*has_child) && !tracked)
        {
            (*start) = (*end);
            (*end) = initial_end;
            tracked = true;
        }
    }
}

void
local_avg_from_child_calc(int *h, const int *array_buffer, int start, int end, int child_return, int *h_start,
                          int *max, double *avg, int *count)
{
    for (int j2 = start; j2 < end; j2++) {
        if (array_buffer[j2] > (*max)) (*max) = array_buffer[j2];

        (*avg) += array_buffer[j2];
        if (array_buffer[j2] <= -INIT_VAL && array_buffer[j2] >= -MAX_RANDOM_OFFSET) {
            h[(*h_start)] = getpid();
            h[(*h_start) + INIT_VAL] = j2;
            h[(*h_start) + 2] = child_return;
            (*h_start) = (*h_start) + 3;
        }
    }
    // Cast the divisor to double to ensure floating-point division
    (*avg) = (*avg) / (double)(end - start);
    (*count) = end - start;
}

void
read_from_buffer_and_get_avg(const int *fd, const int *track_child_pid, int count, int *temp_max, double *temp_avg,
                             int r, int *max, double *avg, int *temp_count, int **temp_h, int *temp_h_start)
{
    read(fd[2 * track_child_pid[r]], temp_max, sizeof(int));
    read(fd[2 * track_child_pid[r]], temp_avg, sizeof(double)); // Change sizeof(long) to sizeof(double)
    read(fd[2 * track_child_pid[r]], temp_count, sizeof(int));
    read(fd[2 * track_child_pid[r]], temp_h, sizeof(int) * ARRAY_BUFFER_SIZE);
    read(fd[2 * track_child_pid[r]], temp_h_start, sizeof(int));

    if ((*temp_max) >= (*max)) {
        (*max) = (*temp_max);
    }
    (*avg) = (*temp_avg);
}

int
fill_in_keys_from_child(int *h, int h_start, const int *temp_h, int temp_h_start) {
    for (int i = 0; i < temp_h_start; i++)
    {
        h[h_start] = temp_h[i];
        h_start++;
    }
    return h_start;
}

void
setup_read(const int *fd, const int *track_child_pid, int r, int **h, int *h_start, int *max, double *avg,
           int *count) {
    read(fd[2 * track_child_pid[r]], max, sizeof(int));
    read(fd[2 * track_child_pid[r]], avg, sizeof(long));
    read(fd[2 * track_child_pid[r]], count, sizeof(int));
    read(fd[2 * track_child_pid[r]], h, sizeof(int) * ARRAY_BUFFER_SIZE);
    read(fd[2 * track_child_pid[r]], h_start, sizeof(int));
}

void
setup_write(int **h, int *h_start, const int *fd, int parent_pipe, int *max, double *avg, int *count) {
    write(fd[2 * parent_pipe + INIT_VAL], max, sizeof(int));
    write(fd[2 * parent_pipe + INIT_VAL], avg, sizeof(long));
    write(fd[2 * parent_pipe + INIT_VAL], count, sizeof(int));
    write(fd[2 * parent_pipe + INIT_VAL], h, sizeof(int) * ARRAY_BUFFER_SIZE);
    write(fd[2 * parent_pipe + INIT_VAL], h_start, sizeof(int));
}

void
print_output_to_file(const int *h, FILE *output, int max, double avg) {
    output = fopen(OUTPUT_FILE_NAME, FILE_MODE_APPEND);
    printf(RESULT_MSG_MAX_MIN, max, avg);
    fprintf(output, RESULT_MSG_MAX_MIN, max, avg);
    for (int i = 0; i < H * 3; i += 3)
    {
        printf(RESULT_MSG_FORMAT, h[i], h[i + 2], h[i + INIT_VAL]);
        fprintf(output, RESULT_MSG_FORMAT, h[i], h[i + 2], h[i + INIT_VAL]);
    }
    fclose(output);
}


void
process_children_to_parent_for_max_avg_output(int max_child, int root_parent, int *h_start, const int *array_buffer, FILE *output, int result, int *return_arg,
                                              int count_child, pid_t *pid, pid_t make_child, int *start, int *end, int *track_child_pid, int parent_pipe,
                                              int initial_end, int *child_return) {
    for (int j = 0; j < result; j++)
    {

        if (j != 0)
        {
            (*return_arg) = get_return_arg_from_child(max_child, (*return_arg));
        }

        make_child = getpid();
        int incr;
        incr = ceil(((*end) - (*start)) / max_child);
        initial_end = (*end);
        (*end) = (*start);
        create_child_processes(max_child, make_child, count_child, fd, track_child_pid, incr, return_arg, pid, start, end, child_return);


        if ((*pid) == 0)
        {
            parent_pipe = iter_to_track_child_pid(max_child, track_child_pid);
            append_output_file(output, (*return_arg));
            (*pid) = getpid();
            if (j == (result - INIT_VAL))
            {
                int max = 0;
                double avg = 0;
                int count = 0;

                store_child_return_in_buffer(data_array, array_buffer, (*start), (*end), (*child_return), h_start, &max, &avg);

                avg = avg / ((*end) - (*start));
                count = (*end) - (*start);
                parent_pipe_write((int **) &data_array, h_start, fd, parent_pipe, &max, &avg, &count);

                exit(0);
            }
        }
        else
        {
            bool has_child = false;
            int child_count = 0;
            int max = 0;
            bool tracked = false;
            double avg = 0;
            int count = 0;


            int temp_max, temp_count = -INIT_VAL;
            double temp_avg = 0.0;
            int temp_h[ARRAY_BUFFER_SIZE];
            int temp_h_start = 0;

            track_start_and_end(max_child, track_child_pid, initial_end, tracked, start, end, &has_child, &child_count);

            if (!has_child)
            {
                (*end) = initial_end;
            }

            if (child_count == 0)
            {
                local_avg_from_child_calc(data_array, array_buffer, (*start), (*end), (*child_return), h_start, &max, &avg, &count);

            }
            else if (child_count < max_child)
            {
                local_avg_from_child_calc(data_array, array_buffer, (*start), (*end), (*child_return), h_start, &max, &avg, &count);

                for (int r = 0; r < child_count; r++)
                {
                    read_from_buffer_and_get_avg(fd, track_child_pid, count, &temp_max, &temp_avg, r, &max, &avg, &temp_count,
                                                 (int **) &temp_h, &temp_h_start);

                    count += temp_count;

                    (*h_start) = fill_in_keys_from_child(data_array, (*h_start), temp_h, temp_h_start);
                }
            }
            else
            {
                for (int r = 0; r < child_count; r++)
                {
                    if (r != 0) {
                        read_from_buffer_and_get_avg(fd, track_child_pid, count, &temp_max, &temp_avg, r, &max, &avg, &temp_count,
                                                     (int **) &temp_h, &temp_h_start);

                        (*h_start) = fill_in_keys_from_child(data_array, (*h_start), temp_h, temp_h_start);
                    }
                    else
                    {
                        setup_read(fd, track_child_pid, r, (int **) &data_array, h_start, &max, &avg, &count);
                    }
                }
            }

            if (root_parent != getpid())
            {
                setup_write((int **) &data_array, h_start, fd, parent_pipe, &max, &avg, &count);
            }
            else
            {
                wait(NULL);
                print_output_to_file(data_array, output, max, avg);
                clock_t end = clock();
                delta_time += (double)(end - time_beg) / CLOCKS_PER_SEC;
                printf(TIME_TAKEN_MSG, (delta_time));
                exit(0);
            }
            wait(NULL);
            exit(0);
        }
    }
}

int
main(int argc, char* argv[]) {
    time_beg = clock();

    if (argc != 4)
    {
        printf("need to add more args. Should be in this order -> L H PN");
        return -INIT_VAL;
    }

    int max_child, root_parent, h_start;

    if (get_input(argv, &max_child, &root_parent, &h_start) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    printf("\n");

    generate_text_file();

    int *array_buffer;
    FILE *output;
    int result;
    int return_arg;
    read_initialize_in_out_files(&array_buffer, &result, &return_arg);


    int count_child = -INIT_VAL;
    fd = mem_allocation_using_calloc(2 * PN, sizeof(int));
    pid_t pid, make_child;
    int start = 0;
    int end = L + MAX_RANDOM_OFFSET;

    int track_child_pid[4] = {-INIT_VAL, -INIT_VAL, -INIT_VAL, -INIT_VAL};
    int parent_pipe = -INIT_VAL;
    int initial_end, child_return;
    process_children_to_parent_for_max_avg_output(max_child, root_parent, &h_start, array_buffer, output, result, &return_arg, count_child, &pid, make_child, &start, &end, track_child_pid, parent_pipe, initial_end, &child_return);
}