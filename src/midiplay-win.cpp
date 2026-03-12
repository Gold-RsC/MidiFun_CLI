#include "CLI/CLI.hpp"
#define MIDI_DEBUG
#include "MidiParse/MidiParser.hpp"
#define MIDIPLAYER_NO_WARNING
#include "MidiParse/MidiPlayer.hpp"
using namespace GoldType::MidiParse;

#include "private/format_stream.hpp"

#define VERSION "win-1.0.0"

#define SUBCOMMAND(...)


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

        bool print_time = true;
        subcommand->add_flag("--time-show", print_time, "Show time")->default_val(true);
        subcommand->add_option("--time-bar", progress_bar::len, "Length of time bar")->default_val(50);
        uint64_t time_begin = 0;
        subcommand->add_option("--time-begin", time_begin, "Time begin (microsecond / us)")->default_val(0);
        uint64_t time_end = 0;
        subcommand->add_option("--time-end", time_end, "Time end (microsecond / us)")
            ->default_val(std::numeric_limits<uint64_t>::max());

        bool print_lyrics = false;
        subcommand->add_flag("--lyrics-show", print_lyrics, "Show lyrics")->default_val(false);
        uint32_t print_lyrics_num = 1;
        subcommand->add_option("--lyrics-num", print_lyrics_num, "Number of lyrics to show")->default_val(1);
        bool lyrics_emphasis = false;
        subcommand->add_flag("--lyrics-emphasis", lyrics_emphasis, "Emphasis lyrics")->default_val(false);

        double speed = 1.0;
        subcommand->add_option("--speed", speed, "Speed")->default_val(1.0);

        bool loop = false;
        subcommand->add_flag("--loop", loop, "Loop")->default_val(false);

        subcommand->callback([&] {
            if (time_begin > time_end) {
                throw CLI::ValidationError("--time-end", "Time end must be greater than time begin");
            }
            uint64_t max_time = 0;
            MidiFile file(filepath);
            file.read();
            MidiParser parser(file, MidiTimeMode::microsecond);
            TextList lyrics =
                parser.textMap.filter_event_if([](const Text& e) { return e.type == MidiMetaType::lyric; })
                    .merge_event();
            lyrics.sort();
            MidiPlayer player(file);
            if (!player.messageList.empty()) {
                max_time = std::max(max_time, player.messageList.back().time);
            }
            player.set_speed(speed);
            if (time_begin) {
                player.set_time(time_begin);
            }
            if (loop) {
                player.start_loop();
            }
            else {
                player.start_normal();
            }
            std::cout << "Press ctrl + space to pause or continue" << std::endl;
            std::cout << "Press shift + space to stop" << std::endl;
            uint64_t last_time = std::numeric_limits<uint64_t>::max();
            TextList::iterator lyrics_it = lyrics.begin();
            if (print_time || print_lyrics) {
                std::cout << "\033[s";
                std::cout << "\033[?25l";
            }
            while (!player.is_stopped()) {
                if (time_end != std::numeric_limits<uint64_t>::max() && player.get_time() > time_end) {
                    if (loop) {
                        player.set_time(time_begin);
                    }
                    else {
                        player.stop();
                    }
                }
                if (print_time || print_lyrics) {
                    std::cout << "\033[u";
                }
                uint64_t time = player.get_time();
                if (print_time) {
                    if (time / 1000000 != last_time / 1000000) {
                        std::cout << "\033[2K\r" << str_time(time, MidiTimeMode::microsecond) << " : "
                                  << progress_bar({0, time}, {0, max_time}) << " "
                                  << str_time(max_time, MidiTimeMode::microsecond);
                        last_time = time;
                    }
                    std::cout << "\033[1B\r";
                }
                if (print_lyrics) {
                    if (lyrics_it != lyrics.end() && time >= lyrics_it->time) {
                        std::cout << "\033[2K\r";
                    }
                    while (lyrics_it != lyrics.end() && time >= lyrics_it->time) {
                        if (lyrics_emphasis) {
                            std::cout << "\033[7m";
                        }
                        std::cout << lyrics_it->text;
                        if (lyrics_emphasis) {
                            std::cout << "\033[0m";
                        }
                        for (uint32_t i = 1; (lyrics_it + i) != lyrics.end() && i < print_lyrics_num; ++i) {
                            std::cout << ' ' << (lyrics_it + i)->text;
                        }
                        ++lyrics_it;
                    }
                    std::cout << "\033[1B\r";
                }
                if (print_time || print_lyrics) {
                    std::cout << std::flush;
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
            if (print_time || print_lyrics) {
                std::cout << "\033[?25h";
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
