/*************************************************************************************************
 * File that includes the definition of the different structs used in the proyect                *
*************************************************************************************************/

#include <sys/types.h>
#include <string>
#include <limits.h>

#define MAX_BUF_SIZE 1000
#define MAX_WORD_SIZE 100

struct st_payment_req
{
    pid_t pid;
    int new_balance;
};

struct st_browser_req
{
    pid_t pid;
    char objective[MAX_WORD_SIZE];
    int balance;
    int type;
    int results_pipe;
};

struct st_result
{
    char msg[MAX_BUF_SIZE];
    char thread_id[MAX_WORD_SIZE];
    char filename[NAME_MAX];
};