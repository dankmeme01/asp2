#pragma once

namespace asp {

class NumberCycle {
public:
    NumberCycle(int from, int to);
    NumberCycle(int val, int from, int to);

    NumberCycle(const NumberCycle& other) = default;
    NumberCycle(NumberCycle&& other) = default;

    NumberCycle& operator=(const NumberCycle& other) = default;
    NumberCycle& operator=(NumberCycle&& other) = default;

    operator int() const;
    int get() const;
    void set(int val);

    NumberCycle& operator++();
    NumberCycle& operator--();
    NumberCycle& operator++(int);
    NumberCycle& operator--(int);

    void increment();
    void decrement();

    NumberCycle operator+(int) const;
    NumberCycle operator-(int) const;

private:
    int from, to, current;
};

}