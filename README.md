# MidiFun 命令行工具
## 项目简介
MidiFun 是一个基于命令行的 MIDI 文件处理工具，提供解析并输出到文件的功能
当前版本：1.0.0
本项目为一个大学生仅仅为了练手而写的项目，不能保证稳定性和完整性，后续极大概率不会更新，希望有能力的人能够帮忙维护。

## 安装
下载bin目录下的可执行文件即可使用，无需安装。
## 使用方法
在命令行中输入
> midifun -h
> midifun -v
> midifun parse

对于midifun parse命令，可以按照以下格式进行输入：
> midifun parse {in_file} -o {out_file} [-t {tick|microsecond(default)}] [-c {note(default)|note_pair|program|tempo|time_signature|text}] [-f {...}] 



不理解时可以键入`-h`查看帮助
- 当输入`midifun -h`时，会给出：
```
    OPTIONS:
    -h,     --help              Print this help message and exit
    -v,     --version           Display program version information and exit

    SUBCOMMANDS:
    version                     Get version
    parse                       Parse midi file
```

- 当输入`midifun parse -h`时，会给出：
```
midifun.exe parse [OPTIONS] in_file


POSITIONALS:
  in_file TEXT:FILE REQUIRED  Input midi file name

OPTIONS:
  -h,     --help              Print this help message and exit
  -t,     --timemode TEXT:{tick,microsecond} [microsecond]
                              Time mode
  -o,     --out TEXT REQUIRED Output file name
  -c,     --content TEXT:{note,note_pair,program,tempo,time_signature,text} [note]
                              Output content
  -f,     --format TEXT ...   Output format:
                              barbeat: time, track, bar_node, beat_node, denominator, numerator
                              note: time, track, channel, pitch, velocity, instrument, bar,
                              beat
                              note_pair: time, duration, track, channel, pitch, velocity,
                              instrument, bar, bar_diff, beat, beat_diff
                              tempo: time, track, mispqn, time_node
                              text: time, track, type, text
```


## 使用库
- [CLI](https://github.com/CLIUtils/CLI11)
- [MidiParse-v7](https://github.com/Gold-RsC/MidiParse-v7)