#include "CLI/CLI.hpp"
#define MIDI_DEBUG
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

int main(int argc, char** argv) {
    CLI::App app{"MidiFun, a tool to parse midi file easily"};
    // -v,--version
    app.set_version_flag("-v,--version", VERSION);

    // Add this after having fixed bugs
    // enum class ExportFormat : uint8_t {
    //     table,
    //     json
    // };
    // std::map<std::string, ExportFormat> export_formats_map;
    // std::string content;
    // parse->add_option("--format", content, "out file format")
    //     ->default_val("table")
    //     ->transform(CLI::Transformer(export_formats_map));


    SUBCOMMAND(version) {
        auto version = app.add_subcommand("version", "Get version");

        version->callback([&] { std::cout << "Version: " VERSION << std::endl; });
    }
    SUBCOMMAND(info) {
        auto subcommand = app.add_subcommand("info", "Get MIDI info");
        std::string filepath;
        subcommand->add_option("filepath", filepath, "Input MIDI file path")->required()->check(CLI::ExistingFile);

        bool verbose = false;
        subcommand->add_flag("-v,--verbose", verbose, "Print verbose info")->default_val(false);
        bool print_head = false;
        subcommand->add_flag("--head", print_head, "Print head info")->default_val(false);
        bool print_track = false;
        subcommand->add_flag("--track", print_track, "Print track info")->default_val(false);
        bool print_time = false;
        subcommand->add_flag("--time", print_time, "Print time info")->default_val(false);
        bool print_text = false;
        subcommand->add_flag("--text", print_text, "Print text info")->default_val(false);
        bool print_note = false;
        subcommand->add_flag("--note", print_note, "Print note info")->default_val(false);
        bool print_bpm = false;
        subcommand->add_flag("--bpm", print_bpm, "Print bpm info")->default_val(false);
        bool print_channel = false;
        subcommand->add_flag("--channel", print_channel, "Print channel info")->default_val(false);
        MidiTimeMode time_mode;
        subcommand->add_option("--time-mode", time_mode, "Time mode")
            ->default_val(MidiTimeMode::microsecond)
            ->transform(CLI::Transformer(time_mode_map));
        subcommand->add_option("--time-bar", progress_bar::len, "Length of time bar")->default_val(50);


        std::unordered_set<MidiMetaType> text_type_set;
        subcommand->add_option("--text-type", text_type_set, "Text type")
            ->transform(CLI::Transformer(text_type_map))
            ->expected(1, text_type_map.size())
            ->delimiter(',');

        subcommand->callback([&filepath, &verbose, &print_head, &print_track, &print_time, &print_text, &print_note,
                              &print_channel, &time_mode, &text_type_set, &print_bpm] {
            if (verbose) {
                print_head = true;
                print_track = true;
                print_time = true;
                print_text = true;
                print_note = true;
                print_channel = true;
                print_bpm = true;
            }
            MidiFile file(filepath);
            file.read();

            MidiParser parser(file, time_mode);
            MidiTrackList tracks_microsecond = file.tracks;
            tracks_microsecond.to_abs();
            if (time_mode == MidiTimeMode::microsecond) {
                parser.change_timeMode(tracks_microsecond, MidiTimeMode::microsecond);
            }

            std::vector<std::pair<MidiTime, MidiTime>> note_time_min_max(tracks_microsecond.size(),
                                                                         std::pair<MidiTime, MidiTime>{});
            std::vector<std::pair<MidiTime, MidiTime>> track_time_min_max(tracks_microsecond.size(),
                                                                          std::pair<MidiTime, MidiTime>{});
            for (size_t i = 0; i < tracks_microsecond.size(); ++i) {
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

            size_t events_num = 0;
            tracks_microsecond.for_list([&events_num](const MidiTrack& l) { events_num += l.size(); });
            size_t notes_num = 0;
            parser.noteMap.for_list([&notes_num](const NoteList& n) { notes_num += n.size(); });

            std::vector<std::array<size_t, 16>> track_channel_count_map(tracks_microsecond.size(),
                                                                        std::array<size_t, 16>{});
            tracks_microsecond.for_event([&track_channel_count_map](const MidiEvent& e) {
                if (e.is_normal()) {
                    ++track_channel_count_map[e.track][e.channel()];
                }
            });
            BpmMap bpmMap = generate_bpmMap(parser.tempoMap, parser.bbMap);
            std::cout << "MIDI info:" << std::endl;
            std::cout << "    Filepath:                    " << filepath << std::endl;
            if (print_time) {
                std::cout << "    Song Time:                   " << progress_bar(song_time_min_max, song_time_min_max)
                          << std::endl;
                std::cout << str_fill(" ", 34 + (progress_bar::len - 23) / 2)
                          << str_time_start_end(song_time_min_max, time_mode) << std::endl;
            }
            std::cout << "    Events Counts:               " << events_num << std::endl;
            if (print_note) {
                std::cout << "    Notes Counts:                " << notes_num << std::endl;
            }

            if (print_head) {
                std::cout << "    Head:" << std::endl;
                std::cout << "        Format:                  " << file.head.format << std::endl;
                std::cout << "        Tracks Counts:           " << file.head.ntracks << std::endl;
                std::cout << "        Ticks Per Quarter Note:  " << file.head.tpqn() << std::endl;
                std::cout << "        (Division):              " << file.head.division << std::endl;
            }
            if (print_track) {
                for (MidiTrackList::const_iterator track_it = tracks_microsecond.cbegin();
                     track_it != tracks_microsecond.cend(); ++track_it) {
                    MidiTrackNum trackIdx = track_it - tracks_microsecond.cbegin();
                    std::cout << "    Track " << (uint32_t)trackIdx << ":" << std::endl;
                    std::cout << "        Events Counts:           " << track_it->size() << std::endl;
                    if (print_note) {
                        std::cout << "        Notes Counts:            " << parser.noteMap[trackIdx].size()
                                  << std::endl;
                    }
                    if (print_time && track_time_min_max[trackIdx].first <= track_time_min_max[trackIdx].second) {
                        std::cout << "        Track Time:             "
                                  << progress_bar(track_time_min_max[trackIdx], song_time_min_max) << std::endl;
                        std::cout << str_fill(" ", 34 + (progress_bar::len - 23) / 2)
                                  << str_time_start_end(track_time_min_max[trackIdx], time_mode) << std::endl;
                    }
                    if (print_time && print_note &&
                        note_time_min_max[trackIdx].first <= note_time_min_max[trackIdx].second) {
                        std::cout << "        Note Time:              "
                                  << progress_bar(note_time_min_max[trackIdx], song_time_min_max) << std::endl;
                        std::cout << str_fill(" ", 34 + (progress_bar::len - 23) / 2)
                                  << str_time_start_end(note_time_min_max[trackIdx], time_mode) << std::endl;
                    }
                    if (print_bpm && trackIdx < bpmMap.size() && !bpmMap[trackIdx].empty()) {
                        if (print_time) {
                            std::cout << "        BPM                      "
                                      << str_fill(" ", progress_bar::get_pos(bpmMap[trackIdx].front().time,
                                                                             song_time_min_max))
                                      << "^ " << std::fixed << std::setprecision(2) << bpmMap[trackIdx].front().bpm
                                      << std::endl;
                            for (auto it = bpmMap[trackIdx].begin() + 1; it != bpmMap[trackIdx].end(); ++it) {
                                std::cout << "                                "
                                          << str_fill(" ", progress_bar::get_pos(it->time, song_time_min_max)) << "^ "
                                          << std::fixed << std::setprecision(2) << it->bpm << std::endl;
                            }
                        }
                        else {
                            std::cout << "        BPM (time):              " << std::fixed << std::setprecision(2)
                                      << bpmMap[trackIdx].front().bpm << '('
                                      << str_time(bpmMap[trackIdx].front().time, time_mode) << ')' << std::endl;
                            for (auto it = bpmMap[trackIdx].begin() + 1; it != bpmMap[trackIdx].end(); ++it) {
                                std::cout << "                                " << std::fixed << std::setprecision(2)
                                          << it->bpm << str_time(it->time, time_mode) << std::endl;
                            }
                        }
                    }
                    if (print_channel) {
                        auto cmp = [](const std::pair<size_t, MidiChannelNum>& a,
                                      const std::pair<size_t, MidiChannelNum>& b) {
                            return a.first < b.first || (a.first == b.first && a.second > b.second);
                        };
                        std::priority_queue<std::pair<size_t, MidiChannelNum>,
                                            std::vector<std::pair<size_t, MidiChannelNum>>, decltype(cmp)>
                            pq(cmp);
                        for (MidiChannelNum a = 0; a < track_channel_count_map[trackIdx].size(); ++a) {
                            if (track_channel_count_map[trackIdx][a] > 0) {
                                pq.push({track_channel_count_map[trackIdx][a], a});
                            }
                        }
                        if (!pq.empty()) {
                            std::cout << "        Channels (Event Counts): " << (uint32_t)pq.top().second << " ("
                                      << pq.top().first << ") " << std::endl;
                            pq.pop();
                            for (; !pq.empty(); pq.pop()) {
                                std::cout << "                                 " << (uint32_t)pq.top().second << " ("
                                          << pq.top().first << ") " << std::endl;
                            }
                        }
                    }
                    if (print_text) {
                        for (const Text& text : parser.textMap[trackIdx]) {
                            if (text_type_set.empty() || text_type_set.find(text.type) != text_type_set.end()) {
                                std::cout << "        " << std::setw(25) << std::setfill(' ') << std::left
                                          << ("Text (" + Text::get_typeName(text.type) + "):") << safe_print(text.text)
                                          << std::endl;
                            }
                        }
                    }
                }
            }
        });
    }
    SUBCOMMAND(get_note) {
        auto subcommand = app.add_subcommand("get-note", "Get MIDI notes");

        std::string filepath;
        subcommand->add_option("filepath", filepath, "Input MIDI file path")->required()->check(CLI::ExistingFile);

        MidiTimeMode time_mode;
        subcommand->add_option("--time-mode", time_mode, "Time mode")
            ->default_val(MidiTimeMode::microsecond)
            ->transform(CLI::Transformer(time_mode_map));

        bool print_label = false;
        subcommand->add_flag("--label", print_label, "Print label");

        std::vector<NoteVariable> contents;

        subcommand->add_option("--content", contents, "Content to print")
            ->transform(CLI::Transformer(note_content_map))
            ->expected(1, -1)
            ->delimiter(',');

        subcommand->callback([&filepath, &time_mode, &contents, &print_label] {
            if (contents.empty()) {
                contents = {NoteVariable::time,  NoteVariable::track,    NoteVariable::channel,
                            NoteVariable::pitch, NoteVariable::velocity, NoteVariable::instrument,
                            NoteVariable::bar,   NoteVariable::beat};
            }
            MidiParser parser(filepath, time_mode);
            if (print_label) {
                for (NoteVariable content : contents) {
                    switch (content) {
                        case NoteVariable::time: {
                            std::cout << "time" << '\t';
                            break;
                        }
                        case NoteVariable::track: {
                            std::cout << "track" << '\t';
                            break;
                        }
                        case NoteVariable::channel: {
                            std::cout << "channel" << '\t';
                            break;
                        }
                        case NoteVariable::pitch: {
                            std::cout << "pitch" << '\t';
                            break;
                        }
                        case NoteVariable::velocity: {
                            std::cout << "velocity" << '\t';
                            break;
                        }
                        case NoteVariable::instrument: {
                            std::cout << "instrument" << '\t';
                            break;
                        }
                        case NoteVariable::bar: {
                            std::cout << "bar" << '\t';
                            break;
                        }
                        case NoteVariable::beat: {
                            std::cout << "beat" << '\t';
                            break;
                        }
                        default: {
                            std::cerr << "Unknown content: " << (uint32_t)content << std::endl;
                            break;
                        }
                    }
                }
                std::cout << std::endl;
            }
            parser.noteMap.for_event([&time_mode, &contents](const Note& note) {
                for (NoteVariable content : contents) {
                    switch (content) {
                        case NoteVariable::time: {
                            std::cout << note.time << '\t';
                            break;
                        }
                        case NoteVariable::track: {
                            std::cout << (uint32_t)note.track << '\t';
                            break;
                        }
                        case NoteVariable::channel: {
                            std::cout << (uint32_t)note.channel << '\t';
                            break;
                        }
                        case NoteVariable::pitch: {
                            std::cout << (uint32_t)note.pitch << '\t';
                            break;
                        }
                        case NoteVariable::velocity: {
                            std::cout << (uint32_t)note.velocity << '\t';
                            break;
                        }
                        case NoteVariable::instrument: {
                            std::cout << (uint32_t)note.instrument << '\t';
                            break;
                        }
                        case NoteVariable::bar: {
                            std::cout << std::fixed << std::setprecision(4) << note.bar << '\t';
                            break;
                        }
                        case NoteVariable::beat: {
                            std::cout << std::fixed << std::setprecision(4) << note.beat << '\t';
                            break;
                        }
                        default: {
                            std::cerr << "Unknown content: " << (uint32_t)content << std::endl;
                            break;
                        }
                    }
                }
                if (!contents.empty()) {
                    std::cout << std::endl;
                }
            });
        });
    }
    SUBCOMMAND(get_notepair) {
        auto subcommand = app.add_subcommand("get-notepair", "Get MIDI note pairs");

        std::string filepath;
        subcommand->add_option("filepath", filepath, "Input MIDI file path")->required()->check(CLI::ExistingFile);

        MidiTimeMode time_mode;
        subcommand->add_option("--time-mode", time_mode, "Time mode")
            ->default_val(MidiTimeMode::microsecond)
            ->transform(CLI::Transformer(time_mode_map));


        bool print_label = false;
        subcommand->add_flag("--label", print_label, "Print label");

        std::vector<NotePairVariable> contents;

        subcommand->add_option("--content", contents, "Content to print")
            ->transform(CLI::Transformer(notepair_content_map))
            ->expected(1, -1)
            ->delimiter(',');

        subcommand->callback([&filepath, &time_mode, &contents, &print_label] {
            if (contents.empty()) {
                contents = {NotePairVariable::time,       NotePairVariable::duration, NotePairVariable::track,
                            NotePairVariable::channel,    NotePairVariable::pitch,    NotePairVariable::velocity,
                            NotePairVariable::instrument, NotePairVariable::bar,      NotePairVariable::bar_diff,
                            NotePairVariable::beat,       NotePairVariable::beat_diff};
            }
            MidiParser parser(filepath, time_mode);
            if (print_label) {
                for (NotePairVariable content : contents) {
                    switch (content) {
                        case NotePairVariable::time: {
                            std::cout << "time" << '\t';
                            break;
                        }
                        case NotePairVariable::duration: {
                            std::cout << "duration" << '\t';
                            break;
                        }
                        case NotePairVariable::track: {
                            std::cout << "track" << '\t';
                            break;
                        }
                        case NotePairVariable::channel: {
                            std::cout << "channel" << '\t';
                            break;
                        }
                        case NotePairVariable::pitch: {
                            std::cout << "pitch" << '\t';
                            break;
                        }
                        case NotePairVariable::velocity: {
                            std::cout << "velocity" << '\t';
                            break;
                        }
                        case NotePairVariable::instrument: {
                            std::cout << "instrument" << '\t';
                            break;
                        }
                        case NotePairVariable::bar: {
                            std::cout << "bar" << '\t';
                            break;
                        }
                        case NotePairVariable::bar_diff: {
                            std::cout << "bar_diff" << '\t';
                            break;
                        }
                        case NotePairVariable::beat: {
                            std::cout << "beat" << '\t';
                            break;
                        }
                        case NotePairVariable::beat_diff: {
                            std::cout << "beat_diff" << '\t';
                            break;
                        }
                        default: {
                            std::cerr << "Unknown content: " << (uint32_t)content << std::endl;
                            break;
                        }
                    }
                }
            }
            link_notePair(parser.noteMap).for_event([&time_mode, &contents](const NotePair& notePair) {
                for (NotePairVariable content : contents) {
                    switch (content) {
                        case NotePairVariable::time: {
                            std::cout << notePair.time << '\t';
                            break;
                        }
                        case NotePairVariable::track: {
                            std::cout << (uint32_t)notePair.track << '\t';
                            break;
                        }
                        case NotePairVariable::channel: {
                            std::cout << (uint32_t)notePair.channel << '\t';
                            break;
                        }
                        case NotePairVariable::pitch: {
                            std::cout << (uint32_t)notePair.pitch << '\t';
                            break;
                        }
                        case NotePairVariable::velocity: {
                            std::cout << (uint32_t)notePair.velocity << '\t';
                            break;
                        }
                        case NotePairVariable::instrument: {
                            std::cout << (uint32_t)notePair.instrument << '\t';
                            break;
                        }
                        case NotePairVariable::bar: {
                            std::cout << std::fixed << std::setprecision(4) << notePair.bar << '\t';
                            break;
                        }
                        case NotePairVariable::beat: {
                            std::cout << std::fixed << std::setprecision(4) << notePair.beat << '\t';
                            break;
                        }
                        case NotePairVariable::duration: {
                            std::cout << notePair.duration << '\t';
                            break;
                        }
                        case NotePairVariable::bar_diff: {
                            std::cout << std::fixed << std::setprecision(4) << notePair.bar_diff << '\t';
                            break;
                        }
                        case NotePairVariable::beat_diff: {
                            std::cout << std::fixed << std::setprecision(4) << notePair.beat_diff << '\t';
                            break;
                        }
                        default: {
                            std::cerr << "Unknown content: " << (uint32_t)content << std::endl;
                            break;
                        }
                    }
                }
                if (!contents.empty()) {
                    std::cout << std::endl;
                }
            });
        });
    }
    SUBCOMMAND(play) {
        auto subcommand = app.add_subcommand("play", "Play MIDI file")->prefix_command();
        subcommand->set_help_flag();

        subcommand->callback([subcommand] {
            const auto& args = subcommand->remaining();
            std::string cmd = "midiplay-win.exe";

            if (!args.empty()) {
                if (args.front() == "--note" || args.front() == "--midi") {
                    cmd += " " + args.front().substr(2);
                }
                else {
                    cmd += " " + args.front();
                }
                for (auto it = args.begin() + 1; it != args.end(); it++) {
                    cmd += " " + *it;
                }
            }
            system(cmd.c_str());
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
