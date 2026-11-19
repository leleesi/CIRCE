#pragma once
#include <cstddef>
#include <initializer_list>

namespace language {
enum class Id : unsigned { Portuguese = 2070, English = 1033 };
inline Id active = Id::Portuguese; // Set once at startup, before starting workers.

constexpr Id select(unsigned installed, unsigned saved, unsigned system) {
    for (unsigned id : {installed, saved}) {
        if (id == static_cast<unsigned>(Id::English) || id == static_cast<unsigned>(Id::Portuguese))
            return static_cast<Id>(id);
    }
    // The primary language covers both Portugal and Brazil when there is no explicit choice.
    return (system & 0x3ff) == 0x16 ? Id::Portuguese : Id::English;
}
constexpr unsigned tag(const char* data, std::size_t size) {
    if (size < 2 || size > 4)
        return 0;
    if (size > 2 && data[size - 1] == '\n')
        --size;
    if (size > 2 && data[size - 1] == '\r')
        --size;
    if (size != 2)
        return 0;
    if (data[0] == 'p' && data[1] == 't')
        return static_cast<unsigned>(Id::Portuguese);
    if (data[0] == 'e' && data[1] == 'n')
        return static_cast<unsigned>(Id::English);
    return 0;
}
inline bool english() { return active == Id::English; }
void initialize();
} // namespace language
