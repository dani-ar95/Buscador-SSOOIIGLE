/**************************************************************************************************
 * Class implementation of the browsers. Their job is to search for the word requested by the     *
 * client in the files and lines specified by the clientThread                                    *
 **************************************************************************************************/

#include <stdio.h>
#include <iostream>
#include <string.h>
#include <fstream>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include "result.cpp"
#include <bits/stdc++.h>
#include <clocale>
#include "config.h"

#include "structs.cpp"

// Needs to be declared outside
bool is_not_alnum(char c)
{
    std::string str = "áéíóúÁÉÍÓÚñÑ"; // Exceptions to read spanish characters

    if (std::find(str.begin(), str.end(), c) != str.end())
    {
        return false; // The character is alphanumeric
    }
    // 1 -> it is alphanumeric
    // 0 -> it is not alphanumeric
    return isalnum(c) == 0;
}

class Browser
{
private:
    int start;
    int end;
    int client_type;
    int &client_balance;
    pid_t client_pid;
    std::string objective;
    std::string path;

    int fd;

    void send_result(int fd, std::thread::id i, int current_line, std::string prev_word, std::string next_word, std::string word)
    {
        Result r(current_line, prev_word, next_word, word, path, start, end);

        struct st_result new_result;
        // Get thread id. Considering its id as the browser index
        std::stringstream ss;
        ss << i;
        sprintf(new_result.thread_id, "%s", ss.str().c_str());
        // Insert result in struct
        sprintf(new_result.msg, "%s", std::string(r).c_str());
        // Insert file name of the result
        sprintf(new_result.filename, "%s", path.c_str());

        if (write(fd, &new_result, sizeof(new_result)) < sizeof(new_result))
        {
            std::cerr << "[SERVER] Error sending results through named pipe" << std::endl;
        }
    }

    std::string find_prev_next(std::vector<std::string> words, int pos, const std::string objective_line, bool next)
    {
        std::string word;
        // General case with exceptions

        if (pos == 0 && !next) // First word and you want the previous
            return get_limit_word(objective_line, true);

        else if (pos == words.size() - 1 && next) // Last word and you want the next one
            return get_limit_word(objective_line, false);

        if (next)
            return words[pos + 1];
        else
            return words[pos - 1];

        return " ";
    }

    std::string find_next_line(std::istream &p_file)
    {
        std::string next_line;
        std::vector<std::string> words;

        std::getline(p_file, next_line);
        get_words(next_line, words);
        while (is_empty(next_line))
        {
            if (!std::getline(p_file, next_line)) // Skip empty lines
            {
                next_line = " ";
                break;
            }
        }

        return next_line;
    }

    std::string to_lower(std::string str)
    {
        std::for_each(str.begin(), str.end(), [](char &c)
                      { c = ::tolower(c); });
        return str;
    }

    bool is_empty(std::string check_string)
    {
        std::vector<std::string> words;
        get_words(check_string, words);
        return words.size() < 1;
    }

    std::string get_limit_word(std::string line, bool is_first)
    { // Get the first or the last word of a line, depending on is_first value
        if (line == " ")
            return line;

        std::vector<std::string> words;
        std::string word;

        get_words(line, words);

        if (words.empty())
            return " ";
        if (is_first)
            return words.back();

        return words.front();
    }

public:
    Browser();
    Browser(int start, int end, std::string path, std::string objective, pid_t client_pid, int client_type,
            int &client_balance, int fd)
        : client_balance(client_balance)
    {
        this->start = start;
        this->end = end;
        this->path = path;
        this->objective = objective;
        this->client_pid = client_pid;
        this->client_type = client_type;

        this->fd = fd;
    }
    std::mutex *balance_mutex;
    std::mutex *wait_new_balance_mutex;
    std::condition_variable *cv_no_balance;

    int get_start()
    {
        return this->start;
    }
    int get_end()
    {
        return this->end;
    }
    std::string get_objective()
    {
        return this->objective;
    }
    std::string get_path()
    {
        return this->path;
    }
    void get_words(std::string line, std::vector<std::string> &words)
    {
        std::string word;
        std::stringstream new_line(line); // To iterate with getline

        while (std::getline(new_line, word, ' '))
        {
            word.erase(std::remove_if(word.begin(), word.end(), is_not_alnum), word.end()); // Erase punctuation. Remove if is not alphanumeric

            remove(word.begin(), word.end(), ' ');
            if (word.length() > 0)
                words.push_back(word);
        }
    }
    bool search_word_in_line(int index, std::vector<std::string> &words, std::string objective, std::string prev_line,
                             std::string next_line, int current_line, std::string path, int start, int end, int client_balance)
    {
        std::string prev_word;
        std::string next_word;

        if (to_lower(words[index]) == to_lower(objective))
        {
            prev_word = find_prev_next(words, index, prev_line, false); // Generic method for finding the previous or the next
            next_word = find_prev_next(words, index, next_line, true);  // word based on a boolean value

            // Check balance and insert the word
            return insert_result(current_line, prev_word, next_word, words, index);
        }
        return true;
    }

    bool insert_result(int current_line, std::string prev_word, std::string next_word, std::vector<std::string> &words, int index)
    {
        // Mutex for checking the balance
        std::unique_lock<std::mutex> ul_balance(*balance_mutex);

        switch (client_type)
        {
        case PREMIUM_UNLIMITED:
        {
            send_result(fd, std::this_thread::get_id(), current_line, prev_word, next_word, words[index]);
            break;
        }

        case PREMIUM_LIMITED:
        {
            // Check balance
            if (client_balance > 0)
            {
                client_balance--;
                send_result(fd, std::this_thread::get_id(), current_line, prev_word, next_word, words[index]);
            }
            else // Notify the client that has run out of balance and wait for the new balance
            {
                std::cout << "[BROWSER] Client run out of balance" << std::endl;
                kill(client_pid, SIGUSR1);
                std::cout << "[BROWSER] Signal sent" << std::endl;

                std::unique_lock<std::mutex> ul_new_balance(*wait_new_balance_mutex);
                cv_no_balance->wait(
                    ul_new_balance, [this]
                    { return client_balance > 0; });
                std::cout << "[BROWSER] Post re-balancing" << std::endl;
                send_result(fd, std::this_thread::get_id(), current_line, prev_word, next_word, words[index]);
            }
            break;
        }

        case NO_PREMIUM:
            // Check balance
            if (client_balance > 0)
            {
                client_balance--;
                send_result(fd, std::this_thread::get_id(), current_line, prev_word, next_word, words[index]);
            }
            else
            {
                return false; // We stop searching
            }
            break;
        }
        ul_balance.unlock();

        return true;
    }

    void operator()()
    {
        std::setlocale(LC_ALL, "es_ES");

        std::string line;
        std::string prev_line = " ";
        std::string next_line = " ";
        std::ifstream p_file(path);
        std::vector<std::string> words;
        int current_line = this->start;

        // Arriving to the line we are interested in
        for (int i = 0; i < current_line; i++)
            std::getline(p_file, line);

        // Search the word in the line
        while ((std::getline(p_file, line)) && current_line < end)
        {
            if (!is_empty(line)) // Check if the line is not just punctuation
            {
                get_words(line, words); // Convert a line in a vector of strings/words

                std::streampos oldpos = p_file.tellg(); // Save the file pointer
                next_line = find_next_line(p_file);     // Take the next line (in case we need it)
                p_file.seekg(oldpos);                   // Restore back the pointer

                for (int i = 0; i < words.size(); i++)
                {
                    // If returning false means a no-premium client run out of balance
                    if (!search_word_in_line(i, words, this->objective, prev_line, next_line, current_line,
                                             this->path, this->start, this->end, client_balance))
                        return;
                }

                words.clear();
                prev_line = line;
            }
            current_line++; // Read line ++
        }
        p_file.close();
    }
};
