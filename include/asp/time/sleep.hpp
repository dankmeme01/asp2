#pragma once

namespace asp::time {
    class Duration;
    class SystemTime;

    void sleep(const Duration& dur);
    void sleepUntil(const SystemTime& st);
}