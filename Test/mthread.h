#ifndef TILOG_MTHREAD_H
#define TILOG_MTHREAD_H

#include <type_traits>
#include <thread>

template <typename Initer>
class MThread : public std::thread
{
	using Thread = std::thread;

public:
	MThread() = default;
	MThread(MThread&&) = default;
	MThread(const MThread&) = delete;

	template <typename Callable, typename... Args>
	explicit MThread(Callable&& f, Args&&... args)
		: Thread([f, args...] {
			  Initer{}();
			  f(args...);
		  })
	{
	}
};

#endif	  // TILOG_MTHREAD_H
