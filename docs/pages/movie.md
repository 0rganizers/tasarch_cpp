\page movie tasarch Movie Format

This page outlines the plan for the tasarch Movie Format (TAM from now on).
Hopefully, by first designing it, we can make something that actually works :)

\tableofcontents

# General Idea {#general}

The general idea of TAM is to provide a recording of inputs to tasarch, such that it can be played back on any machine and arrive at the same state.
This will be used in retroCTF to support challenges, where players are only allowed to send inputs.

While in theory this sounds relatively simple, there are a few hard requirements that make it much more difficult.
All requirements will be discussed next.

# Requirements

## Hard Requirements {#hardreq}

In the following, we discuss requirements that must be met under all circumstances.

### Storing Inputs {#inputs}

Clearly the most important aspect of TAM and hence everything should focus on this :).
Unfortunately, there are a few important aspects we need to support:

-# **Multiple controllers** (e.g. Player 1 and Player 2)
-# **Different controllers** (e.g. SNES mouse), let's not worry too much about this for now, but we should keep it in mind for future compatibility
-# **Correct synchronization to frames of core**. \todo Figure out how this is done in BizHawk. What happens if you lag???

-# **Support polling multiple times per frame**, see https://tasvideos.org/7245S why this is necessary.

I think supporting 4. and 2. will be the most difficult part.

### Initial (Save) State {#initstate}

There should be an option to have a TAM specify an initialization state for the Core.
This is necessary, as with certain challenges, we might want start at a certain point in the game.
No point in the server being used to render the first 5 minutes of a game that is just cutscenes ;)

In my mind, the initial state is slightly different from the save states for scrubbing (see \ref savestates "Save States").
I think, if enabled, the Core should always reset to that state, never to full uninit, unless explicitly done via some menu.
However, in light that people might want to have multiple TAM at the same time, it might also make more sense to remove the initial state from TAM and solely use it in workspaces.
(Future article discussing that is planned).
I think it makes sense to include the initial state for now, it will bloat the size a lot though lul.

### Various Important Metdata {#metadata}

We should save various important metadata in TAM, such as:

- MD5 of ROM (not necessarily bail on failure, but warn)
- Core (with version)
- Team Token / ID? -> nice for stats
- \todo Probably more

### IDK

\todo Is there other stuff that are truly hard requirements?

## Performance Requirements {#perfreq}

In the following, we discuss certain requirements regarding performance (such as size in memory / on disk), that dont really have hard lines, but should still be met.

## Soft Requirements {#softreq}

In the following, we discuss requirements that could be removed, if there is no way to meet them.

### Save States for Scrubbing, etc. {#savestates}

# Proposed Design {#design}

I propose the format is designed as follows (C++ classes, not necessarily how it looks on disk!).

```cpp
class Test {
    size_t idx;
}
```

# Serialization

There are multiple possibilities for how we could serialize such a format to disk.