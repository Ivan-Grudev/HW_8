#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <stack>
#include <stdexcept>
#include <boost/lockfree/stack.hpp>
#include <boost/lockfree/queue.hpp>
#include <thread>
#include <vector>
#include <functional>
#include <condition_variable>
#include <queue>

template < typename T >
class Threadsafe_Stack
{
public:

	Threadsafe_Stack() = default;

	Threadsafe_Stack(const Threadsafe_Stack& other)
	{
		std::lock_guard <std::mutex> lock(other.m_mutex);
		m_data = other.m_data;
	}

	Threadsafe_Stack& operator=(const Threadsafe_Stack&) = delete;

public:

	void push(T value)
	{
		std::lock_guard < std::mutex > lock(m_mutex);
		m_data.push(value);
	}

	std::shared_ptr < T > pop()
	{
		std::lock_guard < std::mutex > lock(m_mutex);

		if (m_data.empty())
		{
			throw std::range_error("empty stack");
		}

		const auto result = std::make_shared < T >(m_data.top());
		m_data.pop();

		return result;
	}

	void pop(T& value)
	{
		std::lock_guard < std::mutex > lock(m_mutex);

		if (m_data.empty())
		{
			throw std::range_error("empty stack");
		}

		value = m_data.top();
		m_data.pop();
	}

	bool empty() const
	{
		std::lock_guard < std::mutex > lock(m_mutex);
		return m_data.empty();
	}

private:

	std::stack <T> m_data;

private:

	mutable std::mutex m_mutex;
};


template < typename T >
class Threadsafe_Queue
{
public:

	Threadsafe_Queue() = default;

	Threadsafe_Queue(const Threadsafe_Queue& other)
	{
		std::lock_guard <std::mutex> lock(other.m_mutex);
		m_queue = other.m_queue;
	}

	Threadsafe_Queue& operator=(const Threadsafe_Queue& other)
	{
		std::lock_guard <std::mutex> lock(other.m_mutex);
		m_queue = other.m_queue;
	}

public:

	void push(T value)
	{
		std::lock_guard < std::mutex > lock(m_mutex);
		m_queue.push(value);
		m_condition_variable.notify_one();
	}

	void wait_and_pop(T& value)
	{
		std::unique_lock < std::mutex > lock(m_mutex);

		m_condition_variable.wait(lock, [this] {return !m_queue.empty(); });
		value = m_queue.front();
		m_queue.pop();
	}

	std::shared_ptr < T > wait_and_pop()
	{
		std::unique_lock < std::mutex > lock(m_mutex);

		m_condition_variable.wait(lock, [this] {return !m_queue.empty(); });
		auto result = std::make_shared < T >(m_queue.front());
		m_queue.pop();

		return result;
	}

	bool try_pop(T& value)
	{
		std::lock_guard < std::mutex > lock(m_mutex);

		if (m_queue.empty())
		{
			return false;
		}

		value = m_queue.front();
		m_queue.pop();

		return true;
	}

	std::shared_ptr < T > try_pop()
	{
		std::lock_guard < std::mutex > lock(m_mutex);

		if (m_queue.empty())
		{
			return std::shared_ptr < T >();
		}

		auto result = std::make_shared < T >(m_queue.front());
		m_queue.pop();

		return result;
	}

	bool empty() const
	{
		std::lock_guard < std::mutex > lock(m_mutex);
		return m_queue.empty();
	}

private:

	std::queue <T> m_queue;
	std::condition_variable m_condition_variable;

private:

	mutable std::mutex m_mutex;
};



template <typename T>
void Threadsafe_Stack_Add(Threadsafe_Stack <T>& stack, const size_t M)
{
	for (int i = 0; i < M; i++)
	{
		stack.push(i);
	}
}

template <typename T>
void Threadsafe_Stack_Remove(Threadsafe_Stack <T>& stack, const size_t M)
{
	for (int i = 0; i < M; i++)
	{
		stack.pop(i);
	}
}


template <typename T>
void Boost_Stack_Add(boost::lockfree::stack<T>& stack, const size_t M)
{
	for (int i = 0; i < M; i++)
	{
		stack.push(i);
	}
}

template <typename T>
void Boost_Stack_Removing(boost::lockfree::stack<T>& stack, const size_t M)
{
	for (int i = 0; i < M; i++)
	{
		stack.pop(i);
	}
}



template <typename T>
void Threadsafe_Queue_Add(Threadsafe_Queue <T>& queue, const size_t M)
{
	for (int i = 0; i < M; i++)
	{
		queue.push(i);
	}
}

template <typename T>
void Threadsafe_Queue_Remove(Threadsafe_Queue <T>& queue, const size_t M)
{
	for (int i = 0; i < M; i++)
	{
		queue.try_pop(i);
	}
}


template <typename T>
void Boost_Queue_Add(boost::lockfree::queue<T>& queue, const size_t M)
{
	for (int i = 0; i < M; i++)
	{
		queue.push(i);
	}
}

template <typename T>
void Boost_Queue_Remove(boost::lockfree::queue<T>& queue, const size_t M)
{
	for (int i = 0; i < M; i++)
	{
		queue.pop(i);
	}
}

template <typename T>
void Test(Threadsafe_Stack <T>& stack1, boost::lockfree::stack<T>& stack2, Threadsafe_Queue <T>& queue1, boost::lockfree::queue<T>& queue2, const int N, const int M)
{
	std::vector<std::thread> Threads(2 * N);
	for (size_t i = 0; i < N; i++)
	{
		Threads[i] = std::thread(Threadsafe_Stack_Add <T>, std::ref(stack1), M);
	}
	for (size_t i = N; i < 2 * N; i++)
	{
		Threads[i] = std::thread(Threadsafe_Stack_Remove <T>, std::ref(stack1), M);
	}
	auto now1 = std::chrono::system_clock::now();
	std::for_each(Threads.begin(), Threads.end(), std::mem_fn(&std::thread::join));
	auto end1 = std::chrono::system_clock::now();
	auto elapsed1 = std::chrono::duration_cast<std::chrono::microseconds>(end1 - now1);
	std::cout << "Threadsafe_Stack processing time(microseconds): " << elapsed1.count() << std::endl;

	for (size_t i = 0; i < N; i++)
	{
		Threads[i] = std::thread(Boost_Stack_Add <T>, std::ref(stack2), M);
	}
	for (size_t i = N; i < 2 * N; i++)
	{
		Threads[i] = std::thread(Boost_Stack_Removing <T>, std::ref(stack2), M);
	}
	auto now2 = std::chrono::system_clock::now();
	std::for_each(Threads.begin(), Threads.end(), std::mem_fn(&std::thread::join));
	auto end2 = std::chrono::system_clock::now();
	auto elapsed2 = std::chrono::duration_cast<std::chrono::microseconds>(end2 - now2);
	std::cout << "Boost_Stack processing time(microseconds): " << elapsed2.count() << std::endl;



	for (size_t i = 0; i < N; i++)
	{
		Threads[i] = std::thread(Threadsafe_Queue_Add <T>, std::ref(queue1), M);
	}
	for (size_t i = N; i < 2 * N; i++)
	{
		Threads[i] = std::thread(Threadsafe_Queue_Remove <T>, std::ref(queue1), M);
	}
	auto now3 = std::chrono::system_clock::now();
	std::for_each(Threads.begin(), Threads.end(), std::mem_fn(&std::thread::join));
	auto end3 = std::chrono::system_clock::now();
	auto elapsed3 = std::chrono::duration_cast<std::chrono::microseconds>(end3 - now3);
	std::cout << "Threadsafe_Queue processing time(microseconds): " << elapsed3.count() << std::endl;


	for (size_t i = 0; i < N; i++)
	{
		Threads[i] = std::thread(Boost_Queue_Add <T>, std::ref(queue2), M);
	}
	for (size_t i = N; i < 2 * N; i++)
	{
		Threads[i] = std::thread(Boost_Queue_Remove <T>, std::ref(queue2), M);
	}
	auto now4 = std::chrono::system_clock::now();
	std::for_each(Threads.begin(), Threads.end(), std::mem_fn(&std::thread::join));
	auto end4 = std::chrono::system_clock::now();
	auto elapsed4 = std::chrono::duration_cast<std::chrono::microseconds>(end4 - now4);
	std::cout << "Boost_Queue processing time(microseconds): " << elapsed4.count() << std::endl;
}



int main()
{
	Threadsafe_Stack <int> stack1;
	boost::lockfree::stack<int> stack2(1);
	Threadsafe_Queue <int> queue1;
	boost::lockfree::queue<int> queue2(1);
	int N = 0, M = 0;
	std::cout << "Incert the number of threads: ";
	std::cin >> N;
	std::cout << "Incert the number of elements: ";
	std::cin >> M;
	std::cout << std::endl;
	Test(stack1, stack2, queue1, queue2, N, M);

	return 0;
}
