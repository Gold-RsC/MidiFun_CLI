#include "CLI/CLI.hpp"
#define MIDI_CHECK_LEVEL 3
#include "MidiParse/MidiParser.hpp"
using namespace GoldType::MidiParse;

#define VERSION "1.1.0"

#define SUBCOMMAND(...)

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
        uint64_t second = get.time / 1000000;
        os << std::setw(2) << std::setfill('0') << second / 60 / 60 << ": " << std::setw(2) << std::setfill('0')
           << second / 60 % 60 << ": " << std::setw(2) << std::setfill('0') << second % 60;
    }
    else if (get.mode == MidiTimeMode::tick) {
        os << std::setw(5) << get.time << " tick";
    }
    return os;
}
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
struct progress_bar {
    static size_t len;
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
int main(int argc, char** argv) {
    CLI::App app{"MidiFun, a tool to parse midi file and print as other format"};
    // -v,--version
    app.set_version_flag("-v,--version", VERSION);

    SUBCOMMAND(version) {
        auto version = app.add_subcommand("version", "Get version");

        version->callback([&] { std::cout << "Version: " VERSION << std::endl; });
    }
    SUBCOMMAND(info) {
        auto info = app.add_subcommand("info", "Get MIDI info");
        std::string filepath;
        info->add_option("filepath", filepath, "Input MIDI file path")->required()->check(CLI::ExistingFile);


        info->callback([&filepath] {
            MidiFile file(filepath);
            file.read();

            MidiParser parser(file, MidiTimeMode::microsecond);
            MidiTrackList tracks_microsecond = file.tracks;
            tracks_microsecond.to_abs();
            parser.change_timeMode(tracks_microsecond, MidiTimeMode::microsecond);

            std::array<std::pair<MidiTime, MidiTime>, 128> note_time_min_max = {};
            std::array<std::pair<MidiTime, MidiTime>, 128> track_time_min_max = {};
            for (size_t i = 0; i < 128; ++i) {
                note_time_min_max[i] = {std::numeric_limits<MidiTime>::max(), std::numeric_limits<MidiTime>::min()};
                track_time_min_max[i] = {std::numeric_limits<MidiTime>::max(), std::numeric_limits<MidiTime>::min()};
            }
            std::pair<MidiTime, MidiTime> song_time_min_max = {std::numeric_limits<MidiTime>::max(),
                                                               std::numeric_limits<MidiTime>::min()};
            tracks_microsecond.for_event([&track_time_min_max, &note_time_min_max](const MidiEvent& event) {
                track_time_min_max[event.track].first = std::min(track_time_min_max[event.track].first, event.time);
                track_time_min_max[event.track].second = std::max(track_time_min_max[event.track].second, event.time);
                if (event.type() == MidiEventType::note_on || event.type() == MidiEventType::note_off) {
                    note_time_min_max[event.track].first = std::min(note_time_min_max[event.track].first, event.time);
                    note_time_min_max[event.track].second = std::max(note_time_min_max[event.track].second, event.time);
                }
            });
            for (const auto& p : track_time_min_max) {
                if (p.first <= p.second) {
                    song_time_min_max.first = std::min(song_time_min_max.first, p.first);
                    song_time_min_max.second = std::max(song_time_min_max.second, p.second);
                }
            }
            if (song_time_min_max.first > song_time_min_max.second) {
                song_time_min_max = {0, 0};
            }

            std::cout << "MIDI info:" << std::endl;
            std::cout << "    Filepath:                    " << filepath << std::endl;
            std::cout << "    Head:" << std::endl;
            std::cout << "        Format:                  " << file.head.format << std::endl;
            std::cout << "        Num Of Tracks:           " << file.head.ntracks << std::endl;
            std::cout << "        Ticks Per Quarter Note:  " << file.head.tpqn() << std::endl;
            std::cout << "        (Division):              " << file.head.division << std::endl;
            std::cout << "        Song Time:               " << progress_bar(song_time_min_max, song_time_min_max)
                      << std::endl;
            std::cout << str_fill(" ", 34 + (progress_bar::len - 23) / 2)
                      << str_time_start_end(song_time_min_max, MidiTimeMode::microsecond) << std::endl;
            for (MidiTrackList::const_iterator it = tracks_microsecond.cbegin(); it != tracks_microsecond.cend();
                 ++it) {
                MidiTrackNum trackIdx = it - tracks_microsecond.cbegin();
                std::cout << "    Track " << (uint32_t)trackIdx << ":" << std::endl;
                if (track_time_min_max[trackIdx].first <= track_time_min_max[trackIdx].second) {
                    std::cout << "        Track Time:             "
                              << progress_bar(track_time_min_max[trackIdx], song_time_min_max) << std::endl;
                    std::cout << str_fill(" ", 34 + (progress_bar::len - 23) / 2)
                              << str_time_start_end(track_time_min_max[trackIdx], MidiTimeMode::microsecond)
                              << std::endl;
                }
                if (note_time_min_max[trackIdx].first <= note_time_min_max[trackIdx].second) {
                    std::cout << "        Note Time:              "
                              << progress_bar(note_time_min_max[trackIdx], song_time_min_max) << std::endl;
                    std::cout << str_fill(" ", 34 + (progress_bar::len - 23) / 2)
                              << str_time_start_end(note_time_min_max[trackIdx], MidiTimeMode::microsecond)
                              << std::endl;
                }
                for (const Text& text : parser.textMap[trackIdx]) {
                    std::cout << "        " << std::setw(25) << std::setfill(' ') << std::left
                              << (Text::get_typeName(text.type) + ':') << safe_print(text.text) << std::endl;
                }
            }
        });
    }
    try {
        app.parse(argc, argv);
    }
    catch (CLI::ParseError& e) {
        std::cerr << e.what() << std::endl;
        app.exit(e);
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
