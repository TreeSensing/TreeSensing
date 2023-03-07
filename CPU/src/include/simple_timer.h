/*
 * A timer implemented by std::chrono::system_clock.
 * https://gist.github.com/mcleary/b0bf4fa88830ff7c882d
 */

#pragma once

#include <chrono>

class SimpleTimer {
  public:
    void start() {
        m_StartTime = std::chrono::system_clock::now();
        m_bRunning = true;
    }

    void stop() {
        m_EndTime = std::chrono::system_clock::now();
        m_bRunning = false;
    }

    double elapsedMicroseconds() {
        std::chrono::time_point<std::chrono::system_clock> endTime;

        if (m_bRunning) {
            endTime = std::chrono::system_clock::now();
        } else {
            endTime = m_EndTime;
        }

        return std::chrono::duration_cast<std::chrono::microseconds>(
                   endTime - m_StartTime)
            .count();
    }

    double elapsedMilliseconds() { return elapsedMicroseconds() / 1000.0; }

    double elapsedSeconds() { return elapsedMicroseconds() / 1000000.0; }

  private:
    std::chrono::time_point<std::chrono::system_clock> m_StartTime;
    std::chrono::time_point<std::chrono::system_clock> m_EndTime;
    bool m_bRunning = false;
};
