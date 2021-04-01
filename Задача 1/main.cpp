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
    return 4 * k / N;
}

void pie(double xm, const int N, double &K)
{
    double k = 0;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dist(0,1);
    std::uniform_real_distribution<> dist_2(xm, xm + 0.25);
    for(auto i = 0; i < N; ++i) {
        double x = dist_2(gen);
        double y = dist(gen);
        if( x*x + y*y <= 1) {
            ++k;
        }
    }

    g_mutex.lock();
    K += k;
    g_mutex.unlock();
}

double pie_par()
{
    const int N = 100000;
    int num = std::thread::hardware_concurrency();
    int n = N / num;
    double k = 0;

    std::vector < std::thread > th(num);

    for (std::size_t i = 0; i < num; ++i)
	{
		th[i] = std::thread(
			pie, static_cast<double>(i) / static_cast<double>(num),
            static_cast<double>(i + 1) / static_cast<double>(num),
            n, std::ref(k));

	}

	for(int i = 0; i < num; ++i)
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
