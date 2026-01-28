#pragma once

namespace asp::inline time {
    class Duration;
    class SystemTime;

    void sleep(const Duration& dur);
    void sleepUntil(const SystemTime& st);
    void yield();
}