#define TOTAL_CLIENTS 50
#define CONCURRENT_CLIENTS 4
#define WAIT_BETWEEN_CLIENTS 5
#define CLIENTS_SUBSET 8
#define PREMIUM_PROPORTION 80
#define PATH_BOOKS "Libros"
#define PATH_RESULTS "Results"
#define PATH_PIPES "Pipes"
#define PATH_PAYMENTS "Pipes_Payments"

#define PREMIUM_UNLIMITED 0
#define PREMIUM_LIMITED 1
#define NO_PREMIUM 2
#define PAYMENT 10

#define READ 0
#define WRITE 1

#include <thread>
#define LINES_PER_THREAD 32000 / std::thread::hardware_concurrency()
#define CHANCE_OF_WORD 5
#define DICTIONARY_SIZE 25
