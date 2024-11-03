#pragma once

#include <chrono>

class Timer {
public:
    Timer() 
        : m_startTime(std::chrono::steady_clock::now())
    {}

    void Reset() { m_startTime = std::chrono::steady_clock::now(); }

    float GetElapsedTime() const noexcept
    {
        m_endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(m_endTime - m_startTime);
        
        return duration.count();
    }

private:
    std::chrono::steady_clock::time_point m_startTime;
    mutable std::chrono::steady_clock::time_point m_endTime;
};