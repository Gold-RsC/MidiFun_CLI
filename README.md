# MidiFun

MidiFun is a command-line tool for parsing and processing MIDI files, written in C++.

It's provides various subcommands to extract information, and perform MIDI in simple operations.

## Features

- Display MIDI file information

- Extract note (or notepair) data with customizable output fields

- Play MIDI files (Windows Only)

- Support for different time modes: tick(midi tick), microsecond(which means us)

- Filter events in some way

## Dependencies

- [CLI11](https://github.com/CLIUtils/CLI11) - A modern, C++11/14/17 command line parser

- [MidiParse-v7](https://github.com/Gold-RsC/MidiParse-v7) - A MIDI file parser library written by me

## Usage

Download the `.exe` files from the repository.

And then run the executable file with the following command:

```
./MidiFun.exe [subcommand] [options]
```

<!-- AUTO_HELP_START -->
## Command Line Help

### Global options

```text
MidiFun, a tool to parse midi file easily


./bin/midifun.exe [OPTIONS] [SUBCOMMANDS]


OPTIONS:
  -h,     --help              Print this help message and exit
  -v,     --version           Display program version information and exit

SUBCOMMANDS:
  version                     Get version
  info                        Get MIDI info
  get-note                    Get MIDI notes
  get-notepair                Get MIDI note pairs
  play                        Play MIDI file
```

### `version` subcommand

```text
Get version


./bin/midifun.exe version [OPTIONS]


OPTIONS:
  -h,     --help              Print this help message and exit
```

### `info` subcommand

```text
Get MIDI info


./bin/midifun.exe info [OPTIONS] filepath


POSITIONALS:
  filepath TEXT:FILE REQUIRED Input MIDI file path

OPTIONS:
  -h,     --help              Print this help message and exit
  -v,     --verbose [0]       Print verbose info
          --head [0]          Print head info
          --track [0]         Print track info
          --time [0]          Print time info
          --text [0]          Print text info
          --note [0]          Print note info
          --bpm [0]           Print bpm info
          --channel [0]       Print channel info
          --time-mode ENUM:{microsecond->1,tick->0} [1]  
                              Time mode
          --time-bar UINT [50]  
                              Length of time bar
          --text-type ENUM:{device_name->9,instrument_name->4,lyric->5,marker->6,program_name->8,song_copyright->2,start_point->7,track_name->3,track_text->1} 
                              Text type
```

### `get-note` subcommand

```text
Get MIDI notes


./bin/midifun.exe get-note [OPTIONS] filepath


POSITIONALS:
  filepath TEXT:FILE REQUIRED Input MIDI file path

OPTIONS:
  -h,     --help              Print this help message and exit
          --time-mode ENUM:{microsecond->1,tick->0} [1]  
                              Time mode
          --label             Print label
          --filter-track UINT Track numbers
          --filter-channel UINT 
                              Channel numbers
          --content ENUM:{bar->6,beat->7,channel->2,instrument->5,pitch->3,time->0,track->1,velocity->4} ... 
                              Content to print
```

### `get-notepair` subcommand

```text
Get MIDI note pairs


./bin/midifun.exe get-notepair [OPTIONS] filepath


POSITIONALS:
  filepath TEXT:FILE REQUIRED Input MIDI file path

OPTIONS:
  -h,     --help              Print this help message and exit
          --time-mode ENUM:{microsecond->1,tick->0} [1]  
                              Time mode
          --label             Print label
          --filter-track UINT Track numbers
          --filter-channel UINT 
                              Channel numbers
          --content ENUM:{bar->6,beat->7,channel->2,instrument->5,pitch->3,time->0,track->1,velocity->4} ... 
                              Content to print
```

### `play` subcommand

```text
MidiPlay, a tool to play midi file easily


./bin/midiplay-win.exe [OPTIONS] [SUBCOMMANDS]


OPTIONS:
  -h,     --help              Print this help message and exit
  -v,     --version           Display program version information and exit

SUBCOMMANDS:
  note                        Play note
  midi                        Play midi file
```

<!-- AUTO_HELP_END -->

## License

MIT License