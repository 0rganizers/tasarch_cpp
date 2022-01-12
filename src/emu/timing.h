#ifndef __TIMING_H
#define __TIMING_H

#include <string>

struct timing_info
{
    void record(double value);

    std::string output();
    void output(FILE* f);

    bool had_record = false;
    double average = 0.0;
    double min = 0.0;
    double max = 0.0;
    double last = 0.0;

    double alpha = 0.01;
};

#endif /* __TIMING_H */