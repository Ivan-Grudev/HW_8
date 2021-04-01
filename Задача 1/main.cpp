#include<iostream>
#include<vector>
#include<algorithm>
#include<random>
#include <ctime>
#include <thread>
#include <mutex>

using namespace std;
std::mutex g_mutex;

double pie_seq()
{
    static double k = 0;
    const int N = 100000;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dist(0,1);

    for(auto i = 0; i < N; ++i) {
        double x = dist(gen);
        double y = dist(gen);
        if( (x*x + y*y) <= 1.0) {
            ++k;
        }
    }
    return 4 * static_cast<double>(k) / static_cast<double>(N);
}

void pie(double x_1, double x_2, const int N, double &K)
{
    double k = 0;
    std::random_device rd;
    std::mt19937 mersenne(rd());
    std::uniform_real_distribution<> dist(0, 1);
    std::uniform_real_distribution<> dist_2(x_1, x_2);

    for(auto i = 0; i < N; ++i)
    {
        double x = dist_2(mersenne);
        double y = dist(mersenne);
        if( x*x + y*y <= 1)
        {
            ++k;
        }
    }

    std::lock_guard < std::mutex > lock(mutex);
    K += k;
    g_mutex.unlock();
}

double pie_par()
{
    const int N = 100000;
    int num_core = std::thread::hardware_concurrency();
    int num_threads = num_core != 0 ? num_core : 4;
    int n = N / num_threads;
    double k = 0;
    std::vector < std::thread > th(num_threads);

    for (std::size_t i = 0; i < th.size(); ++i)
	{
		th[i] = std::thread(
			pie, static_cast<double>(i) / static_cast<double>(num_threads),
            static_cast<double>(i + 1) / static_cast<double>(num_threads),
            n, std::ref(k));

	}

	for(int i = 0; i < num_threads; ++i)
    {
        th[i].join();
    }

    return 4 * static_cast<double>(k) / static_cast<double>(N);
}

int main()
{
    cout << "sequential: p = " << pie_seq() << endl;
    cout << "parallel: p = " << pie_par() << endl;

    return EXIT_SUCCESS;
}
