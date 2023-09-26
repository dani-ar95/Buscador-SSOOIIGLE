/**********************************************************************************************
 * Class implementation of the ClientThreads. These threads will be dynamically created.      *
 * Their job is to create the necessary browsers for the search of a client request and       *
 * establish the communication with the client                                                *
**********************************************************************************************/

#include "Client.cpp"
#include <vector>
#include "Browser.cpp"
#include <filesystem>
#include "config.h"
#include <thread>
#include <map>
#include <sstream>
#include <mutex>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// Shared mutex between all client threads
std::mutex g_sem_browser_list;
std::mutex g_sem_thread_list;

class ClientThread
{
private:
    std::vector<std::thread> browser_threads;
    std::map<std::string, int> file_lengths;
    std::string objective;
    std::vector<Browser> browsers;

    std::mutex *g_sem_modify_clients;

    void create_browsers(int n_lines, int n_browsers, std::string path, std::string objective, std::vector<Browser> &browsers,
                         pid_t client_pid, int type, int &client_balance, int fd) // The balance is passed by value
    {
        int start = 0;

        int len = LINES_PER_THREAD;

        for (int i = 0; i < n_browsers - 1; i++)
        {
            Browser br(start, start + len, path, objective, client_pid, type, client_balance, fd);
            br.balance_mutex = balance_mutex.get();
            br.wait_new_balance_mutex = wait_new_balance_mutex.get();
            br.cv_no_balance = cv_no_balance.get();

            std::unique_lock<std::mutex> ul(g_sem_browser_list);
            browsers.push_back(std::move(br));
            ul.unlock();

            start += len;
        }

        // Length + reminder, in case some the number of lines is not a multiple of the number of threads.
        // In case there is not reminder, the reminder == 0
        Browser br(start, n_lines, path, objective, client_pid, type, client_balance, fd);
        br.balance_mutex = balance_mutex.get();
        br.wait_new_balance_mutex = wait_new_balance_mutex.get();
        br.cv_no_balance = cv_no_balance.get();

        std::unique_lock<std::mutex> ul(g_sem_browser_list);
        browsers.push_back(std::move(br));
        ul.unlock();
    }

    void create_threads(int n_threads, std::vector<Browser> &browser_vector, std::vector<std::thread> &thread_vector)
    {
        for (unsigned i = 0; i < n_threads; i++)
        {
            std::unique_lock<std::mutex> ul(g_sem_thread_list);
            thread_vector.push_back(std::thread(std::ref(browser_vector[i])));
        }
    }

    void remove_client(std::vector<ClientThread *> *g_clients_vector, Client &target, std::vector<std::thread> *g_client_threads)
    {
        int index = 0;
        for (std::vector<ClientThread *>::iterator it = g_clients_vector->begin(); it != g_clients_vector->end(); ++it)
        {
            if ((*it)->client.get_pid() == target.get_pid())
            {
                std::unique_lock<std::mutex> ul(*g_sem_modify_clients);
                g_clients_vector->erase(it);
                g_client_threads->erase(g_client_threads->begin() + index);
                break;
            }
            index++;
        }
    }

    int open_comunication_with_client()
    {
        std::ostringstream path;
        path << PATH_PIPES << "/" << client.get_pid();
        int fd = -1;

        if ((fd = open(path.str().c_str(), O_WRONLY)) < 0)
        {
            std::cerr << "[SERVER] Error opening named pipe" << std::endl;
            exit(EXIT_FAILURE);
        }

        return fd;
    }

    void closing_comunication_with_client(int fd)
    {
        if (close(fd) < 0)
        {
            std::cerr << "[SERVER] Error closing named pipe" << std::endl;
        }
    }

public:
    Client client;
    ClientThread(Client client, std::map<std::string, int> file_lengths, std::mutex *g_sem_modify_clients)
    {
        this->client = client;
        this->file_lengths = file_lengths;
        this->objective = client.get_objective();

        this->balance_mutex = std::make_shared<std::mutex>();
        this->wait_new_balance_mutex = std::make_shared<std::mutex>();
        this->cv_no_balance = std::make_shared<std::condition_variable>();

        this->g_sem_modify_clients = g_sem_modify_clients;
    }

    std::shared_ptr<std::mutex> balance_mutex;
    std::shared_ptr<std::mutex> wait_new_balance_mutex;
    std::shared_ptr<std::condition_variable> cv_no_balance;

    std::vector<ClientThread *> *g_clients_vector;
    std::vector<std::thread> *g_client_threads;
    std::condition_variable *g_cv_server_loop;

    void operator()()
    {
        int n_browsers = 0;
        int start = 0;
        int fd = -1;

        // Open comunication
        fd = open_comunication_with_client();

        for (auto const &[key, val] : file_lengths) // For each file and lines of file
        {
            // Get number of browsers
            if (val < LINES_PER_THREAD)
                n_browsers = 1;
            else
                n_browsers = val / LINES_PER_THREAD;

            // Create new browsers
            create_browsers(val, n_browsers, key, this->objective, this->browsers, this->client.get_pid(), this->client.get_type(), this->client.get_balance(), fd);
        }

        // Create a thread for every browser
        create_threads(browsers.size(), this->browsers, this->browser_threads);

        std::for_each(browser_threads.begin(), browser_threads.end(), std::mem_fn(&std::thread::join));

        std::cout << "[SERVER] Results sent to client " << client.get_pid() << std::endl;

        closing_comunication_with_client(fd);

        // Removing the client and this thread
        remove_client(this->g_clients_vector, this->client, this->g_client_threads);
        g_cv_server_loop->notify_one();
    }
};
