//
// Created by 91410 on 2020/2/26.
//
#include "inc.h"

#ifndef ANDROIDLANGUAGEPROCESS_TIMEINTERVAL_H
#define ANDROIDLANGUAGEPROCESS_TIMEINTERVAL_H


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
		uint64_t interval = std::chrono::duration_cast<std::chrono::milliseconds>(m_stop - m_start).count();
		std::string stringD = (m_flag + "  end!");
		EZLOGI << "time: " << interval<<"ms.";
	}

	inline uint64_t GetMillisecondsUpToNOW()
	{
		m_stop = std::chrono::steady_clock::now();
		uint64_t interval = std::chrono::duration_cast<std::chrono::milliseconds>(m_stop - m_start).count();
		return interval;
	}


protected:

	inline void init()
	{
		m_start = std::chrono::steady_clock::now();
		std::string stringD = (m_flag + " #### begin!\n");
		EZLOGI << stringD;
	}

private:
	std::string m_flag;

	std::chrono::steady_clock::time_point m_start;
	std::chrono::steady_clock::time_point m_stop;

};


#endif //ANDROIDLANGUAGEPROCESS_TIMEINTERVAL_H
