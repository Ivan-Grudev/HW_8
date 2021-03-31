#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <ctime>
#include <thread>
#include <mutex>
#include <atomic>
#include <future>
#include <iterator>
#include <numeric>
#include <string>
using namespace std;

void Rand_Init (int n)
{
}

//int Rand(int n)
//{
//    return dist(gen);
//}


class Threads_Guard
{
public:

	explicit Threads_Guard(std::vector < std::thread > & threads) :
		m_threads(threads)
	{}

	Threads_Guard(Threads_Guard const &) = delete;

	Threads_Guard& operator=(Threads_Guard const &) = delete;

	~Threads_Guard() noexcept
	{
		try
		{
			for (std::size_t i = 0; i < m_threads.size(); ++i)
			{
				if (m_threads[i].joinable())
				{
					m_threads[i].join();
				}
			}
		}
		catch (...)
		{
			// std::abort();
		}
	}

private:

	std::vector < std::thread > & m_threads;
};

template < typename Iterator, typename T >
struct Searcher
{
	void operator()(Iterator first, Iterator last, T element,
		std::promise < Iterator > & result, std::atomic < bool > & flag) noexcept
	{
		try
		{
			for (; (first != last) && !flag.load(); ++first)
			{
				if (*first == element)
				{
					result.set_value(first);
					flag.store(true);
					return;
				}
			}
		}
		catch (...)
		{
			try
			{
				result.set_exception(std::current_exception());
				flag.store(true);
			}
			catch (...)
			{
				// ...
			}
		}
	}
};

template < typename Iterator, typename T >
Iterator parallel_find(Iterator first, Iterator last, T element)
{
	const std::size_t length = std::distance(first, last);

	if (!length)
		return last;

	const std::size_t min_per_thread = 25;
	const std::size_t max_threads =
		(length + min_per_thread - 1) / min_per_thread;

	const std::size_t hardware_threads =
		std::thread::hardware_concurrency();

	const std::size_t num_threads =
		std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);

	const std::size_t block_size = length / num_threads;

	std::promise < Iterator > result;
	std::atomic < bool > flag(false);
	std::vector < std::thread > threads(num_threads - 1);

	{
		Threads_Guard guard(threads);

		Iterator block_start = first;

		for (std::size_t i = 0; i < (num_threads - 1); ++i)
		{
			Iterator block_end = block_start;
			std::advance(block_end, block_size);

			threads[i] = std::thread(Searcher < Iterator, T > (),
				block_start, block_end, element, std::ref(result), std::ref(flag));

			block_start = block_end;
		}

		Searcher < Iterator, T > ()(block_start, last, element, result, flag);
	}

	if (!flag.load())
	{
		return last;
	}

	return result.get_future().get();
}

int main()
{
    vector <char> dna = {'A', 'G', 'T', 'C'};
//    vector <string> dna = {"A", "G", "T", "C"};

    string st = "";

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 3);
    int n;
    for (int i = 0; i < 10; i++) {
        n = dist(gen);
        st = st + dna[n];
    }
    cout << st << endl;

    vector <char> f = {'T'};

//    cout << "find: " << f[0] << endl;
    for (size_t i = 0; i < st.length(); i++) {

        auto result = parallel_find(st.begin() + i, st.end(), f[0]);
//        auto result = parallel_find(st.begin() + i, st.begin() + i, f[0]);
        if (result != st.end())
//        if (result == st.begin() + i)
            cout << "i: " << i + 1 << endl;

//        result = parallel_find(result + 1, st.end(), f[1]);
//	auto result = parallel_find(1, 10, "CAT");
    }
//	if (result != st.end())
//	{
//		std::cout << "Element found: " << *result << std::endl;
////		std::cout << "Element found: " << result << std::endl;
//	}
//	else {
//		std::cout << "Element not found." << std::endl;
//	}
    return 0;
}
