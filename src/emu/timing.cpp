#include <cstdio>
#include <string>
#include "timing.h"
#include <fmt/core.h>

void timing_info::record(double value)
{
    this->last = value;
    if (!this->had_record) {
        this->min = value;
        this->max = value;
        this->average = value;
        this->had_record = true;
    }

    if (value < this->min) this->min = value;
    if (value > this->max) this->max = value;

    this->average = this->alpha*value + (1-this->alpha)*this->average;
}

auto timing_info::output() -> std::string
{
    return fmt::format("{: 5.1f} /{: 5.1f} / {: 5.1f} / {: 5.1f}", min, max, average, last);
}

void timing_info::output(FILE* f)
{
    fprintf(f, "% 3.1f / % 3.1f / % 3.1f / % 3.1f", min, max, average, last);
}