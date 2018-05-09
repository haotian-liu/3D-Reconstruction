//
// Created by Haotian on 18/5/9.
//

#ifndef INC_3DRECONSTRUCTION_TIMER_H
#define INC_3DRECONSTRUCTION_TIMER_H

#include <chrono>

using std::chrono::steady_clock;
using std::chrono::time_point;
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::nanoseconds;

class Timer {
public:
    Timer() {}
    void start() {
        begin = steady_clock::now();
    }
    void stop() {
        end = steady_clock::now();
        accumulation += duration_cast<nanoseconds>(end - begin).count();
    }
    void clear() {
        accumulation = 0;
    }
    long long int elasped() const {
        return duration_cast<milliseconds>(end - begin).count();
    }
    long long int accumulated() const {
        return accumulation / 1000000;
    }

private:
    time_point<steady_clock> begin, end;
    long long int accumulation = 0;

};


#endif //INC_3DRECONSTRUCTION_TIMER_H
