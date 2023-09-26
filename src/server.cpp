/***************************************************************************************************
 * The server process job is to wait for any request from a client. When a new request arrives,    *
 * it will attend it depending on the type of request                                              *
***************************************************************************************************/

#include "Server.cpp"
#include "PaymentGateway.cpp"
#include <thread>
#include <unistd.h>
#include <map>
#include <filesystem>
#include <fstream>

std::map<std::string, int> calculate_dictionary_lines();
int get_lines_of_file(std::string path);
void redirect_request(struct st_browser_req *req);

// Main function
int main(int argc, char const *argv[])
{
    Server server(calculate_dictionary_lines());
    PaymentGateway gw;

    // Create a server thread. This thread will be creating threads for attending client requests
    std::thread server_thread(server);
    // Creating thread for the payment gateway
    std::thread gw_thread(gw);
    // Send them to background
    server_thread.detach();
    gw_thread.detach();

    // Pipe for receiving client requests
    int requests_fd = atoi(argv[0]);

    // Buffer for receiving requests
    struct st_browser_req *req = (st_browser_req *)malloc(sizeof(st_browser_req));

    // Loop for reading requests from a pipe
    while (read(requests_fd, req, sizeof(st_browser_req)) > 0)
    {
        std::cout << "[SERVER] Received request from client " << req->pid << ", objective: " << req->objective << ", balance: " << req->balance
                  << ", type: " << req->type << ", pipe: " << req->results_pipe << std::endl;

        redirect_request(req);
    }
    return 0;
}

// Returns a dictionary with <file, length> of the books' directory
std::map<std::string, int> calculate_dictionary_lines()
{
    std::map<std::string, int> books;
    int n_lines;

    for (const auto &entry : std::filesystem::directory_iterator(PATH_BOOKS))
    {
        n_lines = get_lines_of_file(entry.path());
        books.insert({entry.path(), n_lines});
    }
    return books;
}

int get_lines_of_file(std::string path)
{
    std::ifstream file_stream(path);
    std::string line;
    int n_lines = 0;

    while (std::getline(file_stream, line))
        n_lines++;

    n_lines++;
    return n_lines;
}

// Function to classify and redirect requests to the corresponding queues
void redirect_request(struct st_browser_req *req)
{
    Client client;
    switch (req->type)
    {
    case NO_PREMIUM: // Redirect to no_premium queue
        client = Client(req->pid, req->balance, std::string(req->objective), req->type, req->results_pipe);
        g_clients_queue_nopremium.push(client);
        g_cv_server_loop.notify_one();
        break;
    case PAYMENT: // Create payment request and send to gateway
        struct st_payment_req new_payment;
        new_payment.new_balance = req->balance;
        new_payment.pid = req->pid;
        g_payment_requests_queue.push(new_payment);
        std::cout << "[server] Redirecting to gateway" << std::endl;
        g_cv_gateway.notify_one();
        break;
    default: // Redirect to premium queue
        client = Client(req->pid, req->balance, std::string(req->objective), req->type, req->results_pipe);
        g_clients_queue_premium.push(client);
        g_cv_server_loop.notify_one();
        break;
    }
}
