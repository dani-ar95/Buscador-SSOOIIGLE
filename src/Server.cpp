/***********************************************************************************************
 * Class implementation for the Server. Its job is to pick a client from the client requests   *
 * queue and create a clientThread that will start the browsing threads for that client        *
***********************************************************************************************/

#include <vector>
#include <queue>
#include <iostream>
#include "ClientThread.cpp"
#include <condition_variable>
#include <thread>
#include <map>
#include <unistd.h>

// Global variables
std::condition_variable g_cv_server_loop;
std::mutex g_sem_server_loop;
std::queue<Client> g_clients_queue_nopremium;
std::queue<Client> g_clients_queue_premium;

std::vector<ClientThread *> g_clients_vector;
std::vector<std::thread> g_client_threads;

std::mutex g_sem_modify_clients;

class Server
{
private:
    std::map<std::string, int> file_lengths;

    Client pick_client()
    {
        Client new_client;
        int chance = rand() % 100;
        if (chance < PREMIUM_PROPORTION)
        {
            if (!g_clients_queue_premium.empty())
            {
                new_client = g_clients_queue_premium.front();
                g_clients_queue_premium.pop();
                std::cout << "[SERVER] Selected a Premium Client" << std::endl;
            }
            else
            {
                new_client = g_clients_queue_nopremium.front();
                g_clients_queue_nopremium.pop();
                std::cout << "[SERVER] Selected a Premium Client but queue of premiums was empty. Selecting normal one..." << std::endl;
            }
        }
        else if (chance >= PREMIUM_PROPORTION)
        { // No premium
            if (!g_clients_queue_nopremium.empty())
            {
                new_client = g_clients_queue_nopremium.front();
                g_clients_queue_nopremium.pop();
                std::cout << "[SERVER] Selected a No-Premium Client" << std::endl;
            }
            else
            {
                new_client = g_clients_queue_premium.front();
                g_clients_queue_premium.pop();
                std::cout << "[SERVER] Selected a No-Premium Client but queue of no-premiums was empty. Selecting a premium one..." << std::endl;
            }
        }
        return new_client;
    }

public:
    Server() = default;

    Server(std::map<std::string, int> file_lengths)
    {
        this->file_lengths = file_lengths;
    }

    void operator()()
    {
        srand(getpid());
        std::unique_lock<std::mutex> ul(g_sem_server_loop);
        Client current_client;

        // Server loop
        std::cout << g_clients_vector.size() << std::endl;
        std::cout << "[SERVER] Starting server loop..." << std::endl;

        while (true)
        {
            g_cv_server_loop.wait(ul,
                                []
                                {
                                    return (!g_clients_queue_nopremium.empty() || !g_clients_queue_premium.empty()) && g_clients_vector.size() < CONCURRENT_CLIENTS;
                                }); // if there are new clients we continue looping

            // New client arrived
            current_client = pick_client(); // Pick a client from the queues

            std::unique_lock<std::mutex> ul(g_sem_modify_clients);
            ClientThread *ct = new ClientThread(current_client, file_lengths, &g_sem_modify_clients);
            ct->g_client_threads = &g_client_threads;
            ct->g_clients_vector = &g_clients_vector;
            ct->g_cv_server_loop = &g_cv_server_loop;

            g_clients_vector.push_back(std::move(ct));
            g_client_threads.push_back(std::thread(std::ref(*g_clients_vector.back())));

            g_client_threads.back().detach();
        }
    }
};