\page gdbcmds Additional GDB Commands

\tableofcontents

The commands below can be executed using `monitor cmd` inside gdb.
\todo mention how to in IDA, ghidra and radare. also is it really monitor? dont think it ever worked for me lul.

\todo sort this list alphabetically!

# `help [cmd=]`

Shows the usage of the command `cmd`. If no command is specified, it shows help of all commands.

# `frame [num=1]`

Steps until `num` frames have finished rendering.
\note This means, if you for example hit a breakpoint in the middle of rendering a frame, `frame 1` will step until the current frame is finished.

\note `num` must be at least 1, use [scrubbing](#scrub_rel) for "stepping" backwards.

# `ipoll [num=1]`

Steps until input has been polled `num` times.
This is probably only useful for the Super Mario Bros 3 TAS lul.

# `fbreak [num]` (aliases: `fb`)

Set a breakpoint before frame `num` of the current TAM.

# `tam [subcmd]`

Provides various commands to interface with the current TAM controls.

## `tam scrub abs [frame]` (aliases: `tam sa`)

## `tam scrub rel [num=1]` (aliases: `tam sr`) {#scrub_rel}

Takes the number of the last fully rendered frame (`last`), adds `num` and then scrubs to the end of the resulting frame being rendered.

\note `num` can be negative or zero!

To make things clearer, see this example:

@m_class{m-block m-success}

@par Example
You have a TAM file with three frames of recorded input: 0, 1, 2.
You start playback and hit an unrelated breakpoint before frame 1 is fully rendered.
You then execute `tam s 1`. `last = 0`, so we scrub to the end of rendering frame 1.
Therefore, the next frame being rendered would be frame 2.
You now execute `tam s -1`, `last = 1`, so we scrub to the end of rendering frame 0.