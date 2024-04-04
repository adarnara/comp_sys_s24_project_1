#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stddef.h>
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
char *const RESULT_MSG_MAX_MIN = "Max: %d, Avg: %f\n";
char *const RESULT_MSG_FORMAT = "Hi I am Process %d with return argument %d and I found the ""hidden key at position A[%d].\n";
char *const PROMPT_MSG = "How Many Children Do You Want (Must Choose: 2 or 3)?\n";
static char *const TIME_TAKEN_MSG = "Elapsed time %f [s]\n";
const int INIT_VAL = 1;

const int MAX_CHILDREN_OPTIONS = 3;
const int MIN_CHILDREN_OPTIONS = 2;
int L, H, PN;
int *fd;
int data_array[160];
double spent_time = 0.0;


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
                                              int parent_pipe,int initial_end, int *child_return, clock_t begin);



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
    if (*char_endptr != '\0' || char_endptr == argv[INIT_VAL] || temp > INT_MAX || temp < 10000)
    {
        fprintf(stderr, "incorrect number for L: %s\n", argv[INIT_VAL]);
        return EXIT_FAILURE;
    }
    L = (int)temp;

    temp = strtol(argv[2], &char_endptr, 10);
    if (*char_endptr != '\0' || char_endptr == argv[2] || temp < 30 || temp > 60)
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
        fprintf(stderr, "invalid child number -> must be 2 or 3.\n");
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

            for (int j = 0; j < max_child; j++)
            {
                if (track_child[j] == -INIT_VAL)
                {
                    track_child[j] = (*return_arg) - INIT_VAL;
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
    else
    {
        return_arg = 3 * (return_arg) - 2;
    }
    return return_arg;
}

int
iter_to_track_child_pid(int max_child, int *track_child_pid)
{
    int parent_pipe;
    for (int i = 0; i < max_child; i++)
    {
        if (track_child_pid[i] != -INIT_VAL)
        {
            parent_pipe = track_child_pid[i];
            track_child_pid[i] = -INIT_VAL;
        }
    }
    return parent_pipe;
}

void
store_child_return_in_buffer(int *h, const int *array_buffer, int start, int end, int child_return, int *h_start,
                             int *max, double *avg)
{
    for (int i = start; i < end; i++)
    {
        if (array_buffer[i] > (*max)) (*max) = array_buffer[i];
        (*avg) += array_buffer[i];

        if (array_buffer[i] <= -INIT_VAL && array_buffer[i] >= -MAX_RANDOM_OFFSET)
        {
            h[(*h_start)] = getpid();
            h[(*h_start) + INIT_VAL] = i;
            h[(*h_start) + 2] = child_return;
            (*h_start) = (*h_start) + 3;
        }
    }
}

void
track_start_and_end(int max_child, const int *track_child_pid, int initial_end, bool tracked, int *start, int *end,
                    bool *has_child, int *child_count) {
    for (int i = 0; i < max_child; i++)
    {
        if (track_child_pid[i] != -INIT_VAL)
        {
            (*has_child) = true;
            (*child_count)++;
        }
        else if (track_child_pid[i] == -INIT_VAL && (*has_child) && !tracked)
        {
            (*start) = (*end);
            (*end) = initial_end;
            tracked = true;
        }
    }
}

void
local_avg_from_child_calc(int *h, const int *array_buffer, int start, int end, int child_return, int *h_start, int *max,
                          double *avg, int *count) {
    for (int i = start; i < end; i++) {
        if (array_buffer[i] > (*max)) (*max) = array_buffer[i];


        (*avg) += array_buffer[i];
        if (array_buffer[i] <= -INIT_VAL && array_buffer[i] >= -MAX_RANDOM_OFFSET) {
            h[(*h_start)] = getpid();
            h[(*h_start) + INIT_VAL] = i;
            h[(*h_start) + 2] = child_return;
            (*h_start) = (*h_start) + 3;
        }
    }
    (*avg) = (*avg) / (double)(end - start);
    (*count) = end - start;
}

void
read_from_buffer_and_get_avg(const int *fd, const int *track_child_pid, int count, int *temp_max, double *temp_avg, int r,
                             int *max, double *avg, int *temp_count, int **temp_h, int *temp_h_start) {
    read(fd[2 * track_child_pid[r]], temp_max, sizeof(int));
    read(fd[2 * track_child_pid[r]], temp_avg, sizeof(long));
    read(fd[2 * track_child_pid[r]], temp_count, sizeof(int));
    read(fd[2 * track_child_pid[r]], temp_h, sizeof(int) * ARRAY_BUFFER_SIZE);
    read(fd[2 * track_child_pid[r]], temp_h_start, sizeof(int));

    if ((*temp_max) >= (*max))
    {
        (*max) = (*temp_max);
    }
    (*avg) = ((*avg) * count + (*temp_avg) * (*temp_count)) / (double)((*temp_count) + count);
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
           int *count)
{
    read(fd[2 * track_child_pid[r]], max, sizeof(int));
    read(fd[2 * track_child_pid[r]], avg, sizeof(long));
    read(fd[2 * track_child_pid[r]], count, sizeof(int));
    read(fd[2 * track_child_pid[r]], h, sizeof(int) * ARRAY_BUFFER_SIZE);
    read(fd[2 * track_child_pid[r]], h_start, sizeof(int));
}

void
setup_write(int **h, int *h_start, const int *fd, int parent_pipe, int *max, double *avg, int *count)
{
    write(fd[2 * parent_pipe + INIT_VAL], max, sizeof(int));
    write(fd[2 * parent_pipe + INIT_VAL], avg, sizeof(long));
    write(fd[2 * parent_pipe + INIT_VAL], count, sizeof(int));
    write(fd[2 * parent_pipe + INIT_VAL], h, sizeof(int) * ARRAY_BUFFER_SIZE);
    write(fd[2 * parent_pipe + INIT_VAL], h_start, sizeof(int));
}

void
print_output_to_file(const int *h, FILE *output, int max, double avg)
{
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
                                              int initial_end, int *child_return, clock_t begin)
{
    for (int i = 0; i < result; i++)
    {

        if (i != 0)
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
            if (i == (result - INIT_VAL))
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

                for (int a = 0; a < child_count; a++)
                {
                    read_from_buffer_and_get_avg(fd, track_child_pid, count, &temp_max, &temp_avg, a, &max, &avg, &temp_count,
                                                 (int **) &temp_h, &temp_h_start);

                    count += temp_count;

                    (*h_start) = fill_in_keys_from_child(data_array, (*h_start), temp_h, temp_h_start);
                }
            }
            else
            {
                for (int j = 0; j < child_count; j++)
                {
                    if (j != 0) {
                        read_from_buffer_and_get_avg(fd, track_child_pid, count, &temp_max, &temp_avg, j, &max, &avg, &temp_count,
                                                     (int **) &temp_h, &temp_h_start);

                        (*h_start) = fill_in_keys_from_child(data_array, (*h_start), temp_h, temp_h_start);
                    }
                    else
                    {
                        setup_read(fd, track_child_pid, j, (int **) &data_array, h_start, &max, &avg, &count);
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
                if (end > begin) {
                    spent_time += (double)(end - begin) / CLOCKS_PER_SEC;
                } else {
                    spent_time = 0.0;
                }
                printf(TIME_TAKEN_MSG, spent_time);
                exit(0);
            }
            wait(NULL);
            exit(0);
        }
    }
}

int
main(int argc, char* argv[]) {
    clock_t begin = clock();

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
    process_children_to_parent_for_max_avg_output(max_child, root_parent, &h_start, array_buffer, output, result, &return_arg, count_child, &pid, make_child, &start, &end, track_child_pid, parent_pipe, initial_end, &child_return, begin);
}