/* C++ implementation of https://github.com/NanXiao/code-for-blog/blob/master/2018/threadoverhead/thread-switch-condvar.c*/
#include <condition_variable>
#include <err.h>
#include <iostream>
#include <mutex>
#include <sys/resource.h>
#include <thread>

constexpr int NUM_ITERATIONS = 100'000;

int main()
{
    std::cout << "Running for " << NUM_ITERATIONS << " iterations\n";

    std::mutex mu;
    std::condition_variable cv;
    std::unique_lock<std::mutex> lock{mu};

    auto child = [&mu, &cv](int num_iterations) {
        std::unique_lock<std::mutex> lock{mu};
        cv.notify_one();

        for (int i = 0; i < num_iterations; i++)
        {
            cv.wait(lock);
            cv.notify_one();
        }
    };

    std::thread child_thread{child, NUM_ITERATIONS};

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < NUM_ITERATIONS; i++)
    {
        cv.wait(lock);
        cv.notify_one();
    }
    lock.unlock();
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed{std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()};

    auto nswitches{NUM_ITERATIONS * 2};
    std::cout << nswitches << " context switches in " << elapsed << " ns ("
              << (static_cast<double>(elapsed) / nswitches) << "ns / switch)\n";

    child_thread.join();

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