#ifndef ANDROIDLANGUAGEPROCESS_TIMEINTERVAL_H
#define ANDROIDLANGUAGEPROCESS_TIMEINTERVAL_H
#include<string>
#include <iostream>
#include <chrono>

#ifndef SimpleTimerCout
#define SimpleTimerCout std::cout
#endif

class SimpleTimer
{
public:
	inline SimpleTimer(bool autoPrint = true) : m_flag("timer: "), m_closed(!autoPrint) { init(); }

	inline SimpleTimer(const std::string &_detail) : m_flag(_detail)
	{
		init();
	}

	inline SimpleTimer(const int32_t _detail) : m_flag(std::to_string(_detail))
	{
		init();
	}

	inline ~SimpleTimer()
	{
		if (m_closed) { return; }
		m_stop = std::chrono::steady_clock::now();
		uint64_t interval = std::chrono::duration_cast<std::chrono::microseconds>(m_stop - m_start).count();
		std::string stringD = (m_flag + "  ####  end!");
		SimpleTimerCout << "time: " << interval << "us.\n";
	}

	inline void close() { m_closed = true; }

	inline uint64_t GetMillisecondsUpToNOW()
	{
		m_stop = std::chrono::steady_clock::now();
		uint64_t interval = std::chrono::duration_cast<std::chrono::milliseconds>(m_stop - m_start).count();
		return interval;
	}

	inline uint64_t GetMicrosecondsUpToNOW()
	{
		m_stop = std::chrono::steady_clock::now();
		uint64_t interval = std::chrono::duration_cast<std::chrono::microseconds>(m_stop - m_start).count();
		return interval;
	}

	inline uint64_t GetNanosecondsUpToNOW()
	{
		m_stop = std::chrono::steady_clock::now();
		uint64_t interval = std::chrono::duration_cast<std::chrono::nanoseconds>(m_stop - m_start).count();
		return interval;
	}

protected:

	inline void init()
	{
		m_start = std::chrono::steady_clock::now();
		if (m_closed) { return; }
		std::string stringD = (m_flag + " #### begin!\n");
		SimpleTimerCout << stringD << "\n";
	}

private:
	std::string m_flag;

	std::chrono::steady_clock::time_point m_start;
	std::chrono::steady_clock::time_point m_stop;
	bool m_closed{};

};


#endif //ANDROIDLANGUAGEPROCESS_TIMEINTERVAL_H
