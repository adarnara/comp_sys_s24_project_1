
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>


const int MAX_LINE_LENGTH = 256;
const int NUM_HIDDEN_KEYS = 60;
const int KEY_RANGE = 10000;
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
