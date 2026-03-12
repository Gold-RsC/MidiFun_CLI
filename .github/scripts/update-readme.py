#!/usr/bin/env python3
"""
Automatically update README.md with the output of `midifun -h` and all subcommand helps.
Placeholders in README.md:
    <!-- AUTO_HELP_START -->
    <!-- AUTO_HELP_END -->
The content between these markers will be replaced.

Usage:
    Ensure midifun executable is available, then run:
        python update_readme.py

    Optionally set environment variable MIDIFUN_CMD to specify the path to midifun.
    Default is "./midifun" (relative to current directory).
"""

import os
import re
import subprocess
import sys

README_TEMPLATE = ".github/scripts/README.md.template"
README_FILE = "README.md"
MIDIFUN_CMD = os.environ.get("MIDIFUN_CMD", "./bin/midifun.exe")
MIDIPLAY_CMD = os.environ.get("MIDIPLAY_WIN_CMD", "./bin/midiplay-win.exe")
MARKER_START = "<!-- AUTO_HELP_START -->"
MARKER_END = "<!-- AUTO_HELP_END -->"

def run_cmd(cmd_list):
    """Run command and return stdout. Exit on error."""
    try:
        result = subprocess.run(cmd_list, capture_output=True, text=True, check=True)
        return result.stdout
    except subprocess.CalledProcessError as e:
        print(f"Error running {' '.join(cmd_list)}:\n{e.stderr}", file=sys.stderr)
        sys.exit(1)

def extract_subcommands(help_text):
    """
    Extract subcommand names from the main help text.
    Assumes the help contains a line starting with 'SUBCOMMANDS:' followed by indented lines.
    Adjust this function if your output format differs.
    """
    subcmds = []
    in_subcmd_section = False
    for line in help_text.splitlines():
        stripped = line.strip()
        if stripped.startswith("SUBCOMMANDS:"):
            in_subcmd_section = True
            continue
        if in_subcmd_section:
            if not stripped:
                break  # empty line ends the section
            # Subcommand lines typically start with 2 spaces, then the name
            parts = stripped.split()
            if parts:
                subcmds.append(parts[0])
    return subcmds

def generate_help_block():
    """Return a string containing all help texts formatted for README."""
    if not os.path.exists(MIDIFUN_CMD):
        print(f"Executable not found: {MIDIFUN_CMD}", file=sys.stderr)
        print("Set MIDIFUN_CMD environment variable to the correct path.", file=sys.stderr)
        sys.exit(1)

    # Main help
    main_help = run_cmd([MIDIFUN_CMD, "-h"])

    # Get subcommands list
    subcommands = extract_subcommands(main_help)

    # Build output
    lines = []
    lines.append("## Command Line Help")
    lines.append("")
    lines.append("### Global options")
    lines.append("")
    lines.append("```text")
    lines.extend(main_help.strip().splitlines())
    lines.append("```")
    lines.append("")

    for sub in subcommands:
        if sub != "play":
            help_text = run_cmd([MIDIFUN_CMD, sub, "-h"])
            lines.append(f"### `{sub}` subcommand")
            lines.append("")
            lines.append("```text")
            lines.extend(help_text.strip().splitlines())
            lines.append("```")
            lines.append("")
        else:
            help_text = run_cmd([MIDIPLAY_CMD, "-h"])
            lines.append(f"### `{sub}` subcommand")
            lines.append("")
            lines.append("```text")
            lines.extend(help_text.strip().splitlines())
            lines.append("```")
            lines.append("")

    return "\n".join(lines)

def update_readme():
    try:
        with open(README_TEMPLATE, "r", encoding="utf-8") as f:
            content = f.read()
    except FileNotFoundError:
        print(f"README file '{README_TEMPLATE}' not found in current directory.", file=sys.stderr)
        sys.exit(1)

    new_block = generate_help_block()

    # Replace content between markers (including markers)
    pattern = re.escape(MARKER_START) + r".*?" + re.escape(MARKER_END)
    replacement = MARKER_START + "\n" + new_block + "\n" + MARKER_END
    new_content, count = re.subn(pattern, replacement, content, flags=re.DOTALL)

    if count == 0:
        print(f"Warning: Markers '{MARKER_START}' and '{MARKER_END}' not found in {README_TEMPLATE}.", file=sys.stderr)
        sys.exit(1)

    if new_content == content:
        print("No changes to README.md")
    else:
        with open(README_FILE, "w", encoding="utf-8") as f:
            f.write(new_content)
        print("README.md updated successfully.")

if __name__ == "__main__":
    update_readme()