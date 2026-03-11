#include "CLI/CLI.hpp"
#define MIDI_DEBUG
#include "MidiParse/MidiParser.hpp"
#define MIDIPLAYER_NO_WARNING
#include "MidiParse/MidiPlayer.hpp"
using namespace GoldType::MidiParse;

#define VERSION "win-1.0.0"

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

std::map<std::string, MidiTimeMode> time_mode_map = {{"tick", MidiTimeMode::tick},
                                                     {"microsecond", MidiTimeMode::microsecond}};
std::map<std::string, MidiMetaType> text_type_map = {
    {"track_text", MidiMetaType::track_text},
    {"song_copyright", MidiMetaType::song_copyright},
    {"track_name", MidiMetaType::track_name},
    {"instrument_name", MidiMetaType::instrument_name},
    {"lyric", MidiMetaType::lyric},
    {"marker", MidiMetaType::marker},
    {"start_point", MidiMetaType::start_point},
    {"program_name", MidiMetaType::program_name},
    {"device_name", MidiMetaType::device_name},
};

enum class NoteVariable : uint8_t {
    time = 0,
    track = 1,
    channel = 2,
    pitch = 3,
    velocity = 4,
    instrument = 5,
    bar = 6,
    beat = 7
};
std::map<std::string, NoteVariable> note_content_map{
    {"time", NoteVariable::time},   {"track", NoteVariable::track},       {"channel", NoteVariable::channel},
    {"pitch", NoteVariable::pitch}, {"velocity", NoteVariable::velocity}, {"instrument", NoteVariable::instrument},
    {"bar", NoteVariable::bar},     {"beat", NoteVariable::beat},
};
enum class NotePairVariable : uint8_t {
    time = 0,
    track = 1,
    channel = 2,
    pitch = 3,
    velocity = 4,
    instrument = 5,
    bar = 6,
    beat = 7,
    duration = 8,
    bar_diff = 9,
    beat_diff = 10
};
std::map<std::string, NotePairVariable> notepair_content_map{
    {"time", NotePairVariable::time},         {"track", NotePairVariable::track},
    {"channel", NotePairVariable::channel},   {"pitch", NotePairVariable::pitch},
    {"velocity", NotePairVariable::velocity}, {"instrument", NotePairVariable::instrument},
    {"bar", NotePairVariable::bar},           {"beat", NotePairVariable::beat},
};

enum class MidiPitchMode : uint8_t {
    name = 0,
    number = 1,
};
std::map<std::string, MidiPitchMode> pitch_mode_map = {
    {"name", MidiPitchMode::name},
    {"number", MidiPitchMode::number},
};
bool isCtrlSpacePressed = false;
bool isShiftSpacePressed = false;
int main(int argc, char** argv) {
    CLI::App app{"MidiPlay, a tool to play midi file easily"};
    // -v,--version
    app.set_version_flag("-v,--version", VERSION);
    SUBCOMMAND(note) {
        auto subcommand = app.add_subcommand("note", "Play note");
        uint32_t pitch = 0;
        subcommand->add_option("pitch", pitch, "Pitch")->required()->check(CLI::Range(0, 127));
        uint32_t velocity = 0;
        subcommand->add_option("--velocity", velocity, "Velocity")->default_val(127)->check(CLI::Range(0, 127));
        double duration = 0;
        subcommand->add_option("--duration", duration, "Duration")->default_val(1.0);
        uint32_t channel = 0;
        subcommand->add_option("--channel", channel, "Channel")->default_val(0);
        uint32_t instrument = 0;
        subcommand->add_option("--instrument", instrument, "Instrument")->default_val(0);
        subcommand->callback([&] {
            MidiPlayer(NotePairList{NotePair(0, duration * 1e6, MidiTimeMode::microsecond, 0, channel, pitch, velocity,
                                             instrument)})
                .start_normal();
        });
    }
    SUBCOMMAND(midi) {
        auto subcommand = app.add_subcommand("midi", "Play midi file");
        std::string filepath;
        subcommand->add_option("filepath", filepath, "Filepath")->required()->check(CLI::ExistingFile);
        subcommand->add_option("--time-bar", progress_bar::len, "Length of time bar")->default_val(50);
        bool print_time = true;
        subcommand->add_option("--time", print_time, "Print time")->default_val(true);
        double speed = 1.0;
        subcommand->add_option("--speed", speed, "Speed")->default_val(1.0);
        bool loop = false;
        subcommand->add_flag("--loop", loop, "Loop")->default_val(false);
        subcommand->callback([&] {
            uint64_t max_time = 0;
            MidiParser parser(filepath, MidiTimeMode::microsecond);
            parser.noteMap.for_list([&max_time](const NoteList& e) {
                if (!e.empty()) {
                    max_time = std::max(max_time, e.back().time);
                }
            });
            MidiPlayer player(parser.noteMap);
            player.set_speed(speed);
            if (loop) {
                player.start_loop();
            }
            else {
                player.start_normal();
            }
            std::cout << "Press ctrl + space to pause" << std::endl;
            std::cout << "Press shift + space to stop" << std::endl;
            uint64_t last_time = std::numeric_limits<uint64_t>::max();
            while (!player.is_stopped()) {
                if (print_time) {
                    uint64_t time = player.get_time();
                    if (time / 1000000 != last_time / 1000000) {
                        std ::cout << "\r" << str_time(time, MidiTimeMode::microsecond) << " : "
                                   << progress_bar({0, time}, {0, max_time}) << " "
                                   << str_time(max_time, MidiTimeMode::microsecond) << std::flush;
                        last_time = time;
                    }
                }
                if ((GetAsyncKeyState(VK_SPACE) & 0x8000) && (GetAsyncKeyState(VK_CONTROL) & 0x8000)) {
                    if (!isCtrlSpacePressed) {
                        if (player.is_playing()) {
                            player.pause();
                        }
                        else if (player.is_paused()) {
                            player.play();
                        }
                        isCtrlSpacePressed = true;
                    }
                }
                else {
                    isCtrlSpacePressed = false;
                }

                if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) && (GetAsyncKeyState(VK_SPACE) & 0x8000)) {
                    if (!isShiftSpacePressed) {
                        player.stop();
                        isShiftSpacePressed = true;
                        break;
                    }
                }
                else {
                    isShiftSpacePressed = false;
                }
                Sleep(100);
            }
            std::cout << std::endl << "Finished" << std::flush;
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
