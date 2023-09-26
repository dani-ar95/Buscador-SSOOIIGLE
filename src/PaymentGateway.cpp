/***************************************************************************************************
 * Class implementation for the payment gateway. Its job is to wait for a payment request from a   *
 * client, when a new request arrives, it will modify the balance of the client and will notify it *
***************************************************************************************************/

#include <iostream>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <sys/types.h>
#include <signal.h>

#define SIGNEWBALANCE SIGUSR2

// Global variables
std::mutex g_sem_gateway;
std::condition_variable g_cv_gateway;
std::queue<struct st_payment_req> g_payment_requests_queue;

// Class implementation
class PaymentGateway
{
private:

    // Function to update balance and notify client
    void process_payment_req(struct st_payment_req &pay)
    {
        int new_balance = 0;

        std::cout << "[PAYMENT] Processing " << pay.pid << " request" << std::endl;

        pid_t objective_pid = pay.pid;

        new_balance = pay.new_balance;

        auto it = std::find_if(g_clients_vector.begin(), g_clients_vector.end(), [&objective_pid](ClientThread *c)
                               { return c->client.get_pid() == objective_pid; }); // Iterator of the thread that contains the client with this PID

        auto index = std::distance(g_clients_vector.begin(), it);      // Obtain the position inside the clientThreads vector
        g_clients_vector.at(index)->client.set_balance(new_balance);   // Assign the new balance to the client
        g_clients_vector.at(index)->cv_no_balance.get()->notify_one(); // Notify the condition variable of clientThread
    }

    // Function to notify the client that its balance has been updated
    void notify_client(pid_t pid)
    {
        if (kill(pid, SIGNEWBALANCE) != 0)
        {
            std::cerr << "[PAYMENT] Something went wrong notifying the client its balance update" << std::endl;
        }
    }

public:
    PaymentGateway() = default;

    // Implementation of the payment gateway thread
    void operator()()
    {
        std::unique_lock<std::mutex> ul(g_sem_gateway);
        std::cout << "[PAYMENT] Started" << std::endl;

        // Gateway Loop
        while (true)
        {
            g_cv_gateway.wait(ul, [this]
                            { return !g_payment_requests_queue.empty(); }); // If we have requests, we loop

            std::cout << "[PAYMENT] Received a request" << std::endl;
            struct st_payment_req &new_payment = g_payment_requests_queue.front();
            g_payment_requests_queue.pop();

            process_payment_req(new_payment);
            notify_client(new_payment.pid);  // Notify the client that its balance has been updated
        }
    }
};