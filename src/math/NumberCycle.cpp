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

int NumberCycle::operator++() {
    return this->increment();
}

int NumberCycle::operator--() {
    return this->decrement();
}

int NumberCycle::increment() {
    current++;
    if (current > to) current = from;
    return current;
}

int NumberCycle::decrement() {
    current--;
    if (current < from) current = to;
    return current;
}

int NumberCycle::operator+(int) const {
    int n = current + 1;
    return n > to ? from : n;
}

int NumberCycle::operator-(int) const {
    int n = current - 1;
    return n < from ? to : n;
}

}