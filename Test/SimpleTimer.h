#ifndef ANDROIDLANGUAGEPROCESS_TIMEINTERVAL_H
#define ANDROIDLANGUAGEPROCESS_TIMEINTERVAL_H
#include<string>
#include <iostream>
#include <chrono>

class SimpleTimer
{
public:
	inline SimpleTimer() : m_flag("timer: ")
	{
		init();
	}

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
		m_stop = std::chrono::steady_clock::now();
		uint64_t interval = std::chrono::duration_cast<std::chrono::microseconds>(m_stop - m_start).count();
		std::string stringD = (m_flag + "  ####  end!");
		EZCOUT << "time: " << interval<<"us.\n";
	}

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
		std::string stringD = (m_flag + " #### begin!\n");
		EZCOUT << stringD << "\n";
	}

private:
	std::string m_flag;

	std::chrono::steady_clock::time_point m_start;
	std::chrono::steady_clock::time_point m_stop;

};


#endif //ANDROIDLANGUAGEPROCESS_TIMEINTERVAL_H
