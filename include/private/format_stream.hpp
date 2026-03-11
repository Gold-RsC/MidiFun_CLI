#ifndef FORMAT_STREAM_HPP
#define FORMAT_STREAM_HPP
#include <iostream>

// time
struct str_time {
    MidiTime time;
    MidiTimeMode mode;

    str_time(MidiTime _time, MidiTimeMode _mode)
        : time(_time),
          mode(_mode) {
    }
};
std::ostream& operator<<(std::ostream& os, const str_time& get) {
    if (get.mode == MidiTimeMode::microsecond) {
        uint64_t millisecond = get.time / 1000;
        os << std::setw(2) << std::setfill('0') << millisecond / 1000 / 60 << ": " << std::setw(2) << std::setfill('0')
           << millisecond / 1000 % 60 << ". " << std::setw(2) << std::setfill('0') << millisecond % 1000 / 10;
    }
    else if (get.mode == MidiTimeMode::tick) {
        os << std::setw(5) << get.time << " tick";
    }
    return os;
}

// time range
struct str_time_start_end {
    std::pair<MidiTime, MidiTime> time_min_max;
    MidiTimeMode mode;
    str_time_start_end(std::pair<MidiTime, MidiTime> _time_min_max, MidiTimeMode _mode)
        : time_min_max(_time_min_max),
          mode(_mode) {
    }
};
std::ostream& operator<<(std::ostream& os, const str_time_start_end& set) {
    os << str_time(set.time_min_max.first, set.mode) << " - " << str_time(set.time_min_max.second, set.mode);
    return os;
}

// fill str
struct str_fill {
    const char* c;
    size_t len;
    str_fill(const char* _c, size_t _len)
        : c(_c),
          len(_len) {
    }
};
std::ostream& operator<<(std::ostream& os, const str_fill& fill) {
    for (size_t i = 0; i < fill.len; ++i) {
        os << fill.c;
    }
    return os;
}

// safely print str
struct safe_print {
    const std::string& str;
    safe_print(const std::string& _str)
        : str(_str) {
    }
};
std::ostream& operator<<(std::ostream& os, const safe_print& sp) {
    const std::string& input = sp.str;
    for (size_t i = 0; i < input.length(); ++i) {
        uint8_t c = input[i];
        if (c == '\n') {
            os << "\\n";
        }
        else if (c == '\t') {
            os << "\\t";
        }
        else if (c == '\r') {
            os << "\\r";
        }
        else if (c < 32) {
            os << "[\\x" << std::hex << (int)c << std::dec << ']';
        }
        else if (c >= 128) {
            if (i + 1 < input.length() && (c & 0xE0) == 0xC0) {
                os << input.substr(i, 2);
                i++;
            }
            else if (i + 2 < input.length() && (c & 0xF0) == 0xE0) {
                os << input.substr(i, 3);
                i += 2;
            }
            else {
                os << "[\\x" << std::hex << (int)c << std::dec << ']';
            }
        }
        else {
            os << c;
        }
    }
    return os;
}

// progress bar
struct progress_bar {
    static size_t len;
    static size_t get_pos(MidiTime _time, std::pair<MidiTime, MidiTime> _song_time_min_max) {
        return (_time - _song_time_min_max.first) * progress_bar::len /
               (_song_time_min_max.second - _song_time_min_max.first);
    }
    std::pair<MidiTime, MidiTime> song_time_min_max;
    std::pair<MidiTime, MidiTime> certain_time_min_max;
    progress_bar(std::pair<MidiTime, MidiTime> _certain_time_min_max, std::pair<MidiTime, MidiTime> _song_time_min_max)
        : certain_time_min_max(_certain_time_min_max),
          song_time_min_max(_song_time_min_max) {
    }
};
size_t progress_bar::len = 50;
std::ostream& operator<<(std::ostream& os, const progress_bar& bar) {
    size_t a, b, c;
    a = (bar.certain_time_min_max.first - bar.song_time_min_max.first) * progress_bar::len /
        (bar.song_time_min_max.second - bar.song_time_min_max.first);
    b = (bar.certain_time_min_max.second - bar.certain_time_min_max.first) * progress_bar::len /
        (bar.song_time_min_max.second - bar.song_time_min_max.first);
    c = progress_bar::len - a - b;
    os << '[' << str_fill(".", a) << str_fill("¨€", b) << str_fill(".", c) << ']';
    return os;
}

// join str
template <typename iterator>
struct str_join {
    std::pair<iterator, iterator> range;
    std::string step;
    str_join(std::pair<iterator, iterator> _range, std::string _step)
        : range(_range),
          step(_step) {
    }
};
template <typename iterator>
std::ostream& operator<<(std::ostream& os, const str_join<iterator>& join) {
    for (iterator it = join.range.first; it != join.range.second; ++it) {
        if (it != join.range.first) {
            os << join.step;
        }
        os << *it;
    }
    return os;
}
#endif
