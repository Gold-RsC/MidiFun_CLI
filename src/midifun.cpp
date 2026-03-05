#include "CLI/CLI.hpp"
#include "MidiParse/Midi.hpp"
using namespace GoldType::MidiParse;

#define VERSION "1.0.0"

#define COMMAND(...) 

std::vector<std::string> contents={"note","note_pair","program","tempo","time_signature","text"};
std::map<std::string,std::vector<std::string>> content_defaultFormats={
    {"note",{"time","track","channel","pitch","velocity","instrument","bar","beat"}},
    {"note_pair",{"time","duration","track","channel","pitch","velocity","instrument","bar","bar_diff","beat","beat_diff"}},
    {"tempo",{"time","track","mispqn","time_node"}},
    {"barbeat",{"time","track","bar_node","beat_node","denominator","numerator"}},
    {"text",{"time","track","type","text"}}
};

int main(int argc, char **argv) {
    CLI::App app{"MidiFun, a tool to parse midi file and print as other format"};
    // -v,--version
    app.set_version_flag("-v,--version", VERSION);
    
    COMMAND(version){
        auto version=app.add_subcommand("version", "Get version");
        
        version->callback([&]{
            std::cout<<"Version: " VERSION<<std::endl;
        });
    }
    COMMAND(parse){
        auto parse=app.add_subcommand("parse", "Parse midi file");
        std::string in_file;
        parse->add_option("in_file", in_file, "Input midi file name")->required()->check(CLI::ExistingFile);

        std::string time_mode;
        parse->add_option("-t,--timemode",time_mode,"Time mode")->default_val("microsecond")->transform(CLI::IsMember({"tick","microsecond"}));

        std::string out_file;
        parse->add_option("-o,--out",out_file,"Output file name")->required();

        std::string content;
        parse->add_option("-c,--content",content,"Output content")->default_val("note")->transform(CLI::IsMember(contents));

        std::string help_str="Output format:\n";
        auto join_str_with_comma=[](const std::vector<std::string>&strs)->std::string{
            std::string ret;
            for(const auto&str:strs){
                ret+=str;
                if(&str!=&strs.back()){
                    ret+=", ";
                }
            }
            return ret;
        };
        for(const auto&p:content_defaultFormats){
            help_str+="\t"+p.first+": "+join_str_with_comma(p.second)+"\n";
        }
        std::vector<std::string> formats;
        parse->add_option("-f,--format",formats,help_str)->expected(-1);

        parse->callback([&]{
            if(!in_file.empty()){
                MidiParser parser(in_file,time_mode=="microsecond"?MidiTimeMode::microsecond:MidiTimeMode::tick);
                if(formats.empty()){
                    formats=content_defaultFormats[content];
                }
                std::vector<size_t> idx_list;
                for(const std::string&format:formats){
                    auto it=std::find(content_defaultFormats[content].begin(),content_defaultFormats[content].end(),format);
                    if(it==content_defaultFormats[content].end()){
                        throw CLI::ValidationError(format,"Format "+format+" is not default format for content "+content+".Please use -h,--help to check available formats.");
                    }
                    idx_list.push_back(it-content_defaultFormats[content].begin());
                }
                std::ofstream out(out_file);
                for(const std::string&format:formats){
                    out<<format<<'\t';
                }
                out<<std::endl;
                if(content=="note"){
                    const NoteMap&note_map=parser.noteMap();
                    for_event(note_map,[&](const Note&note){
                        for(size_t i:idx_list){
                            switch(i){
                                case 0:{
                                    out<<std::dec<<note.time<<'\t';
                                    break;
                                }
                                case 1:{
                                    out<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)note.track<<'\t';
                                    break;
                                }
                                case 2:{
                                    out<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)note.channel<<'\t';
                                    break;
                                }
                                case 3:{
                                    out<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)note.pitch<<'\t';
                                    break;
                                }
                                case 4:{
                                    out<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)note.velocity<<'\t';
                                    break;
                                }
                                case 5:{
                                    out<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)note.instrument<<'\t';
                                    break;
                                }
                                case 6:{
                                    out<<note.bar<<'\t';
                                    break;
                                }
                                case 7:{
                                    out<<note.beat<<'\t';
                                    break;
                                }
                            }
                        }
                        out<<std::endl;
                    });
                }
                else if(content=="note_pair"){
                    const NotePairMap&note_pair_map=link_notePair(parser.noteMap());
                    for_event(note_pair_map,[&](const NotePair&note_pair){
                        for(size_t i:idx_list){
                            switch(i){
                                case 0:{
                                    out<<std::dec<<note_pair.time<<'\t';
                                    break;
                                }
                                case 1:{
                                    out<<std::dec<<note_pair.duration<<'\t';
                                    break;
                                }
                                case 2:{
                                    out<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)note_pair.track<<'\t';
                                    break;
                                }
                                case 3:{
                                    out<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)note_pair.channel<<'\t';
                                    break;
                                }
                                case 4:{
                                    out<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)note_pair.pitch<<'\t';
                                    break;
                                }
                                case 5:{
                                    out<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)note_pair.velocity<<'\t';
                                    break;
                                }
                                case 6:{
                                    out<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)note_pair.instrument<<'\t';
                                    break;
                                }
                                case 7:{
                                    out<<note_pair.bar<<'\t';
                                    break;
                                }
                                case 8:{
                                    out<<note_pair.beat<<'\t';
                                    break;
                                }
                                case 9:{
                                    out<<note_pair.bar_diff<<'\t';
                                    break;
                                }
                                case 10:{
                                    out<<note_pair.beat_diff<<'\t';
                                    break;
                                }
                            }
                        }
                        out<<std::endl;
                    });
                }
                else if(content=="barbeat"){
                    const BarBeatMap&bar_beat_map=parser.bbMap();
                    for_event(bar_beat_map,[&](const BarBeat&bar_beat){
                        for(size_t i:idx_list){
                            switch(i){
                                case 0:{
                                    out<<std::dec<<bar_beat.time<<'\t';
                                    break;
                                }
                                case 1:{
                                    out<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)bar_beat.track<<'\t';
                                    break;
                                }
                                case 2:{
                                    out<<bar_beat.barNode<<'\t';
                                    break;
                                }
                                case 3:{
                                    out<<bar_beat.beatNode<<'\t';
                                    break;
                                }
                                case 4:{
                                    out<<std::dec<<bar_beat.denominator<<'\t';
                                    break;
                                }
                                case 5:{
                                    out<<std::dec<<bar_beat.numerator<<'\t';
                                    break;
                                }
                            }
                        }
                        out<<std::endl;
                    });
                }
                else if(content=="tempo"){
                    const TempoMap&tempo_map=parser.tempoMap();
                    for_event(tempo_map,[&](const Tempo&tempo){
                        for(size_t i:idx_list){
                            switch(i){
                                case 0:{
                                    out<<std::dec<<tempo.time<<'\t';
                                    break;
                                }
                                case 1:{
                                    out<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)tempo.track<<'\t';
                                    break;
                                }
                                case 2:{
                                    out<<std::dec<<tempo.mispqn<<'\t';
                                    break;
                                }
                                case 3:{
                                    out<<std::dec<<tempo.timeNode<<'\t';
                                    break;
                                }
                            }
                        }
                        out<<std::endl;
                    });
                }
                
                else if(content=="text"){
                    const TextMap&text_map=parser.textMap();
                    for_event(text_map,[&](const Text&text){
                        for(size_t i:idx_list){
                            switch(i){
                                case 0:{
                                    out<<std::dec<<text.time<<'\t';
                                    break;
                                }
                                case 1:{
                                    out<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)text.track<<'\t';
                                    break;
                                }
                                case 2:{
                                    out<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)text.type<<'\t';
                                    break;
                                }
                                case 3:{
                                    out<<text.text<<'\t';
                                    break;
                                }
                            }
                        }
                        out<<std::endl;
                    });
                }
            }
        });
    }
    try{
        app.parse(argc, argv);
    }
    catch(CLI::ParseError& e){
        std::cerr << e.what() << std::endl;
        app.exit(e);
    }
    catch(std::exception& e){
        std::cerr << e.what() << std::endl;
    }
    return 0;
}