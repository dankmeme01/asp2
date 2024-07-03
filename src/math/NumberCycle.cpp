#include <asp/math/NumberCycle.hpp>

namespace asp {

NumberCycle::NumberCycle(int from, int to) : from(from), to(to), current(from) {}
NumberCycle::NumberCycle(int val, int from, int to) : from(from), to(to), current(val) {}

NumberCycle::operator int() const {
    return current;
}

int NumberCycle::get() const {
    return current;
}

void NumberCycle::set(int val) {
    // clamp
    current = (val > to) ? to : (val < from ? from : val);
}

NumberCycle& NumberCycle::operator++() {
    this->increment();
    return *this;
}

NumberCycle& NumberCycle::operator--() {
    this->decrement();
    return *this;
}

void NumberCycle::increment() {
    current++;
    if (current > to) current = from;
}

void NumberCycle::decrement() {
    current--;
    if (current < from) current = to;
}

NumberCycle NumberCycle::operator+(int) const {
    int n = current + 1;
    return NumberCycle(n > to ? from : n, from, to);
}

NumberCycle NumberCycle::operator-(int) const {
    int n = current - 1;
    return NumberCycle(n < from ? to : n, from, to);
}

}