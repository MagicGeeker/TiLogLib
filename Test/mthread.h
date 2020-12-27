#ifndef EZLOG_MTHREAD_H
#define EZLOG_MTHREAD_H

#include <thread>

template <typename Initer>
class MThread : public std::thread
{
	using Thread = std::thread;

public:
	MThread() {}

	template <typename Callable, typename... Args>
	explicit MThread(Callable&& f, Args&&... args)
		: Thread(&callfunc<Callable, Args...>, std::forward<Callable>(f), std::forward<Args>(args)...)
	{
	}

	MThread(MThread&& rhs) : Thread(std::move(rhs)) {}

	MThread(const MThread& rhs) = delete;


protected:
	template <typename Callable, typename... Args>
	static void callfunc(Callable&& f, Args&&... args)
	{
		Initer{}();
		f(std::forward<Args>(args)...);
	}
};

#endif	  // EZLOG_MTHREAD_H
