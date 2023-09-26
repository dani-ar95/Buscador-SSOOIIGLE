/***************************************************************************************************
 * Manager process is supposed to create the server process, keep creating client processes and to *
 * create the dictionary from which the words to be used by clients will be extracted              *
 ***************************************************************************************************/

#include <iostream>
#include <unistd.h>
#include <exception>
#include <vector>
#include <fstream>
#include <filesystem>
#include "Dictionary.cpp"
#include "config.h"
#include <sys/wait.h>
#include "Browser.cpp"

// Global variables
int g_pipe_client_server[2];
pid_t g_server_pid;
std::vector<pid_t> g_generated_clients;

// Function to split a string by a delimiter
void tokenize(std::string const &str, const char delim, std::vector<std::string> &out)
{
    // construct a stream from the string
    std::stringstream ss(str);

    std::string s;
    while (std::getline(ss, s, delim))
    {
        out.push_back(s);
        // std::cout << "La linea es: " << str << std::endl;
    }
}

Dictionary create_dictionary()
{
    std::vector<std::string> all_words; // Vector with all the words of a file
    std::vector<std::string> final_dictionary;
    std::string line, random_word;
    int index, random;
    srand(getpid());

    Browser *br; // To access get_words

    for (const auto &entry : std::filesystem::directory_iterator(PATH_BOOKS)) // Open books directory
    {
        // Open first book
        std::ifstream p_file;
        p_file.open(entry.path());

        while (getline(p_file, line))
        {
            br->get_words(line, all_words);
        }
        p_file.close();

        // Build dictionary
        while (final_dictionary.size() < DICTIONARY_SIZE)
        {
            index = rand() % all_words.size();
            random = rand() % 100 + 1;
            random_word = all_words.at(index);
            if (random > CHANCE_OF_WORD && random_word.size() > 2) // The word is added to the dictionary
            {
                final_dictionary.push_back(random_word);
            }
        }
        break; // Only read one file
    }

    Dictionary wordlist(final_dictionary);

    return wordlist;
}

// Function to create the server process
pid_t create_server(const char *path)
{
    pid_t pid;

    if ((pid = fork()) == -1)
    {
        std::cerr << "[MANAGER] Error forking for creating the server. Exiting..." << std::endl;
        exit(EXIT_FAILURE);
    }

    if (pid == 0)
    {
        execl(path, (char *const)std::to_string(g_pipe_client_server[READ]).c_str(), NULL);
        std::cerr << "[MANAGER] Error executing execl (launching server). Exiting..." << std::endl;
        exit(EXIT_FAILURE);
    }

    return pid;
}

// Function to create the client process
pid_t create_client(const char *path, Dictionary wordlist)
{
    pid_t pid;

    if ((pid = fork()) == -1)
    {
        std::cerr << "[MANAGER] Error forking clients. Exiting..." << std::endl;
        exit(EXIT_FAILURE);
    }

    if (pid == 0)
    {
        execl(path, "client", (char *const)std::to_string(g_pipe_client_server[WRITE]).c_str(), (char *const)wordlist.get_random_word().c_str(), NULL);
        std::cerr << "[MANAGER] Error executing execl (launching new client). Exiting..." << std::endl;
        exit(EXIT_FAILURE);
    }

    return pid;
}

// Function to keep creating clients
void keep_creating_clients(int pipe, Dictionary wordlist)
{
    pid_t client_pid;

    for (int i = 1; i <= TOTAL_CLIENTS; i++)
    {
        client_pid = create_client("exec/client", wordlist);
        g_generated_clients.push_back(client_pid);
        std::cout << "[MANAGER] Client with pid " << client_pid << " created" << std::endl;
        if (i % CLIENTS_SUBSET == 0) // pause creating clients depending on the CLIENTS_SUBSET variable
            sleep(WAIT_BETWEEN_CLIENTS);
    }
}

// Function to remove active clients
void remove_clients()
{
    std::cout << "[MANAGER] Removing active clients..." << std::endl;
    for (unsigned int i = 0; i < g_generated_clients.size(); i++)
    {
        if (kill(g_generated_clients[i], SIGTERM) != 0)
            fprintf(stderr, "[MANAGER] The client couln't be removed %d\n", g_generated_clients[i]);
    }
}

// Function to remove named pipes for communication
void remove_pipes()
{
    std::cout << "[MANAGER] Removing named pipes for communication..." << std::endl;
    try
    {
        for (const auto &entry : std::filesystem::directory_iterator(PATH_PIPES))
            std::filesystem::remove_all(entry.path());
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        std::cerr << "[MANAGER] Error removing named pipes" << std::endl;
    }
}

// Function to end the server process
void close_server()
{
    std::cout << "[MANAGER] Closing server..." << std::endl;
    if (kill(g_server_pid, SIGTERM) != 0)
        fprintf(stderr, "[MANAGER] The server could not be closed\n");
}

// Function to be executed if Ctrl+C is pressed
void signal_handler(int signum)
{
    std::cout << "[MANAGER] Closing application..." << std::endl;

    remove_clients();
    remove_pipes();
    close_server();
    exit(EXIT_SUCCESS);
}

// Main function
int main()
{
    // create pipe for client-server communication
    pipe(g_pipe_client_server);

    // read dictionary
    Dictionary wordlist = create_dictionary();

    // create server
    g_server_pid = create_server("exec/server");
    std::cout << "[MANAGER] Server started" << std::endl;

    // install signal handler
    signal(SIGINT, signal_handler);

    // create clients
    keep_creating_clients(g_pipe_client_server[WRITE], wordlist);

    // wait clients
    for (int i = 0; i < g_generated_clients.size(); i++)
        wait(NULL);
    
    // terminate server
    close_server();

    return EXIT_SUCCESS;
}