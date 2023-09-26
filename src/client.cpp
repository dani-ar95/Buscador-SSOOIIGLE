/***********************************************************************************************
 * The clients job is to create a file where the results of the search will be stored and to   *
 * send a word search request to the server                                                    *
 ***********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <chrono>
#include "structs.cpp"
#include "config.h"

#define MAX_WORD_SIZE 100
#define MAXSIZE 1000

#define SIGNOBALANCE SIGUSR1
#define SIGNEWBALANCE SIGUSR2

// Global variables : needed for signal handlers
pid_t g_pid;
int g_pipe_to_server;

void signal_handler_no_balance(int signal_num);
void signal_handler_new_balance(int signal_num);
int get_type();

int main(int agc, char *argv[])
{
    // argv 1 server pipe
    // argv 2 objective word

    std::cout << "[CLIENT] A client is initialiced with PID: " << getpid() << std::endl;
    int fd;
    g_pipe_to_server = atoi(argv[1]);
    g_pid = getpid();

    // Set initial balance
    srand(rand() ^ g_pid);
    int balance = 10 + (rand() % 21);

    // Openning named pipe
    std::ostringstream path;
    path << PATH_PIPES << "/" << g_pid;

    if (mkfifo(path.str().c_str(), 0666) != 0)
    {
        std::cerr << "[CLIENT] Error creating named pipe" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Allocate space for results
    st_result *results_buffer = (st_result *)malloc(sizeof(st_result));

    // Getting word from dictionary
    char *objective = argv[2];

    // Create log file
    std::ostringstream oss;
    oss << PATH_RESULTS << "/"
        << "Results_" << g_pid;
    std::ofstream log_file(oss.str());

    // Creating search petition
    struct st_browser_req req;
    req.pid = g_pid;
    sprintf(req.objective, "%s", objective);
    req.balance = balance;
    req.type = get_type();
    req.results_pipe = g_pid; // Same name for the named pipe

    // Asigning signal handler for notifications of type no balance
    signal(SIGNOBALANCE, signal_handler_no_balance);

    // Asigning signal handler for notifications of new balance
    signal(SIGNEWBALANCE, signal_handler_new_balance);

    // Sending our petition to the server
    if (write(g_pipe_to_server, &req, sizeof(req)) != sizeof(st_browser_req))
    {
        std::cerr << "[CLIENT " << g_pid << "] Error sending search request. Exiting..." << std::endl;
        exit(EXIT_FAILURE);
    }

    // Measuring time
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    // Waiting for results
    if ((fd = open(path.str().c_str(), O_RDONLY)) < 0)
        std::cerr << "[CLIENT " << g_pid << "] Error opening named pipe." << std::endl;

    while (read(fd, results_buffer, sizeof(struct st_result)) > 0) // 0 == EOF
    {
        log_file << "[CLIENT] "
                 << "Thread id: " << results_buffer->thread_id
                 << "| Path: " << results_buffer->filename << "| "
                 << results_buffer->msg << std::endl;
    }

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    log_file << std::endl;
    log_file << "Total search time (sec): " << (std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count()) / 1000000.0 << std::endl;
    log_file.close();

    close(g_pipe_to_server);
    std::cout << "[CLIENT " << g_pid << "] Finished receiving results. Results file closed." << std::endl;

    return EXIT_SUCCESS;
}

// Type of client
int get_type()
{
    srand(g_pid);
    return (rand() % 3);
}

void signal_handler_no_balance(int signal_num)
{
    signal(SIGNOBALANCE, signal_handler_no_balance);
    std::cout << "[CLIENT " << g_pid << "] No balance left. Asking for new balance..." << std::endl;

    // Create a payment petition
    struct st_browser_req new_pay;
    new_pay.pid = g_pid;
    new_pay.type = PAYMENT;
    new_pay.balance = 10 + (rand() % 21);

    // For not printing trash
    new_pay.results_pipe = 0;
    sprintf(new_pay.objective, "%s", "");


    // Sending payment petition
    if (write(g_pipe_to_server, &new_pay, sizeof(new_pay)) != sizeof(st_browser_req))
    {
        std::cerr << "[CLIENT " << g_pid << "] Error sending payment request. Exiting..." << std::endl;
        exit(EXIT_FAILURE);
    }
}

void signal_handler_new_balance(int signal_num)
{
    signal(SIGNEWBALANCE, signal_handler_new_balance);
    std::cout << "[CLIENT " << g_pid << "] Updated balanced." << std::endl;
}
