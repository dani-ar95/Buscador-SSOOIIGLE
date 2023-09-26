/**************************************************************************************************
 * Class implementation for the Dictionary from which the words to be used by clients will be     *
 * extracted                                                                                      *
**************************************************************************************************/

#include <iostream>
#include <vector>

class Dictionary
{
private:
    std::vector<std::string> words;

public:
    Dictionary() = default;

    Dictionary(std::vector<std::string> words)
    {
        this->words = words;
    }

    std::string get_random_word()
    {
        srand(rand() ^ time(NULL));
        return words[rand() % words.size()];
    }
};