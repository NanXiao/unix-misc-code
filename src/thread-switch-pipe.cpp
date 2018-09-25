/* C++ implementation of https://github.com/eliben/code-for-blog/blob/master/2018/threadoverhead/thread-switch-pipe.c*/
#include <chrono>
#include <err.h>
#include <iostream>
#include <sys/resource.h>
#include <thread>
#include <unistd.h>

constexpr int NUM_ITERATIONS = 100'000;
constexpr char TEST_CHAR = 'j';

void ping_pong(int read_fd, int write_fd, int num_iterations)
{
    char test_char{TEST_CHAR};
    for (int i = 0; i < num_iterations; i++)
    {
        if (::write(write_fd, &test_char, sizeof(test_char)) != sizeof(test_char))
        {
            ::err(1, "write");
        }

        if (::read(read_fd, &test_char, sizeof(test_char)) != sizeof(test_char))
        {
            ::err(1, "read");
        }
    }
}

void thread_ping_pong(int read_fd, int write_fd, int num_iterations)
{
    std::cout << "Thread " << std::this_thread::get_id() << " " << __func__ << '\n';
    std::cout << " read_fd " << read_fd << "; write_fd " << write_fd << '\n';

    ping_pong(read_fd, write_fd, num_iterations);
}

void measure_self_pipeline(int num_iterations)
{
    int pfd[2];
    char write_char{TEST_CHAR}, read_char{};

    if (::pipe(pfd) == -1)
    {
        ::err(1, "pipe");
    }

    auto write_and_read = [&](){
        if (::write(pfd[1], &write_char, sizeof(write_char)) != sizeof(write_char))
        {
            ::err(1, "write");
        }

        if (::read(pfd[0], &read_char, sizeof(read_char)) != sizeof(read_char))
        {
            ::err(1, "read");
        }
    };

    write_and_read();

    if (read_char != TEST_CHAR)
    {
        ::err(1, "read");
    }

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_iterations; i++)
    {
        write_and_read();
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed{std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()};

    std::cout << __func__ << ": " << elapsed << " ns for " << num_iterations
        << " iterations (" << static_cast<double>(elapsed) / num_iterations  << "ns / iter)\n";
}

int main()
{
    measure_self_pipeline(NUM_ITERATIONS);

    int main_to_child[2];
    if (::pipe(main_to_child) == -1)
    {
        ::err(1, "pipe");
    }

    int child_to_main[2];
    if (::pipe(child_to_main) == -1)
    {
        ::err(1, "pipe");
    }

    std::thread child{thread_ping_pong, main_to_child[0], child_to_main[1], NUM_ITERATIONS};

    char test_char{TEST_CHAR};
    if (::write(main_to_child[1], &test_char, sizeof(test_char)) != sizeof(test_char))
    {
        ::err(1, "write");
    }

    auto start = std::chrono::high_resolution_clock::now();
    thread_ping_pong(child_to_main[0], main_to_child[1], NUM_ITERATIONS);
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed{std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()};

    auto nswitches{NUM_ITERATIONS * 2};
    std::cout << nswitches << " context switches in " << elapsed << " ns ("
        << (static_cast<double>(elapsed) / nswitches) << "ns / switch)\n";

    child.join();

    struct rusage ru;
    if (getrusage(RUSAGE_SELF, &ru)) {
        ::err(1, "getrusage");
    } else {
        std::cout << "From getrusage:\n";
        std::cout << "  voluntary switches = " << ru.ru_nvcsw << '\n';
        std::cout <<"  involuntary switches = " << ru.ru_nivcsw << '\n';
    }

    return 0;
}
