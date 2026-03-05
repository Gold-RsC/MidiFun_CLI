#include "CLI/CLI.hpp"
#include "MidiParse/Midi.hpp"
using namespace GoldType::MidiParse;

#define COMMAND(...) 

std::unique_ptr<MidiParser> parser;

int main(int argc, char **argv) {
    CLI::App app{"MidiFun, a tool to parse midi file and print as other format"};
    // -v,--version
    app.set_version_flag("-v,--version", "1.0.0");
    
    
    COMMAND(parse){
        auto parse=app.add_subcommand("parse", "Parse midi file");
        std::string in_file;
        parse->add_option("in_file", in_file, "Input midi file name")->required();
        
        parse->callback([&]{
            std::cout<<"Input file: "<<in_file<<std::endl;
        });
    }
    COMMAND(version){
        auto version=app.add_subcommand("version", "Get version");
        
        version->callback([&]{
            std::cout<<"Version: 1.0.0"<<std::endl;
        });
    }
    

    try{
        CLI11_PARSE(app, argc, argv);
    }
    catch(const CLI::ParseError& e){
        std::cerr << e.what() << std::endl;
        return 1;
    }
    catch(const std::exception& e){
        std::cerr << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}