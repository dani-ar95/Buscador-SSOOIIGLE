/***************************************************************************************************
 * Class implementation for the Client. It is used inside the server to manage the client data     *
***************************************************************************************************/

#include <sys/types.h>
#include <string>
#include <vector>
#include <thread>

// Class implementation
class Client
{
private:
    pid_t pid;
    int balance;
    int type;
    int results_pipe;
    std::string objective;

public:
    Client() = default;

    Client(pid_t pid, int balance, std::string objective, int type, int results_pipe)
    {
        this->balance = balance;
        this->pid = pid;
        this->objective = objective;
        this->type = type;
        this->results_pipe = results_pipe;
    }

    pid_t get_pid()
    {
        return this->pid;
    }

    int &get_balance()
    {
        return this->balance;
    }

    void set_balance(int new_balance)
    {
        this->balance = new_balance;
    }

    std::string get_objective()
    {
        return this->objective;
    }

    int get_pipe()
    {
        return this->results_pipe;
    }

    int get_type()
    {
        return this->type;
    }
};