#pragma once

namespace asp {

class NumberCycle {
public:
    NumberCycle(int from, int to) : from(from), to(to), current(from) {}
    NumberCycle(int val, int from, int to) : from(from), to(to), current(val) {}

    NumberCycle(const NumberCycle& other) = default;
    NumberCycle(NumberCycle&& other) = default;

    NumberCycle& operator=(const NumberCycle& other) = default;
    NumberCycle& operator=(NumberCycle&& other) = default;

    operator int() const {
        return current;
    }

    int get() const {
        return current;
    }

    void set(int val) {
        // clamp
        current = (val > to) ? to : (val < from ? from : val);
    }

    NumberCycle& operator++() {
        this->increment();
        return *this;
    }

    NumberCycle& operator--() {
        this->decrement();
        return *this;
    }

    NumberCycle& operator++(int) {
        this->increment();
        return *this;
    }

    NumberCycle& operator--(int) {
        this->decrement();
        return *this;
    }

    void increment() {
        current++;
        if (current > to) current = from;
    }

    void decrement() {
        current--;
        if (current < from) current = to;
    }

    NumberCycle operator+(int) const {
        int n = current + 1;
        return NumberCycle(n > to ? from : n, from, to);
    }

    NumberCycle operator-(int) const {
        int n = current - 1;
        return NumberCycle(n < from ? to : n, from, to);
    }

private:
    int from, to, current;
};

}