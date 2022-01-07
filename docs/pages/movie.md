\page movie tasarch Movie Format

This page outlines the plan for the tasarch Movie Format (TAM from now on).
Hopefully, by first designing it, we can make something that actually works :).

\note While the title (and TAM) might imply this document only deals with the on disk format, it actually deals with both (in memory representation and handling in the app).
Discussing the in memory representation and what we need from it, is a lot more important IMO than the actual serialization format.

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
-# **Different controllers** (e.g. SNES mouse), ~~let's not worry too much about this for now, but we should keep it in mind for future compatibility~~ (nevermind, if we want ds we definitely need this already for the screen :/)
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

In general, TAMs should be efficient to deal with, both for the on disk representation and the in memory one. In particular, the following operations should be fast:

- Serialization
- Editing
- Display
- Scrubbing

## Soft Requirements {#softreq}

In the following, we discuss requirements that could be removed, if there is no way to meet them.

### Save States for Scrubbing, etc. {#savestates}

This is IMO almost a hard requirement. 
The idea is that, like a normal movie, you can jump around in the movie and scrub to certain frames.

@m_class{m-block m-success}

@par Example
You start recording some inputs in real time.
After having recorded 180 frames of input (3 seconds), you notice that you messed up on frame 67.
You pause recording and playback, then you right click on frame 67 and jump to there.
You manually correct the input on frame 67 with the TAS input window (part of a future article).
Finally, you jump to frame 180 and continue recording.

**The problem**: How do we actually implement this? Unlike a normal movie, you cannot just scrub easily, especially with changing inputs.

My proposed solution, would be something like the following:

- Every `x` frames (configurable, per core?), while recording a TAM, a save state is made and saved with the TAM.
- When the user jumps to frame `y`, we do the following:
    - We search for the "next" previous frame `x'`, which has a save state associated with it.
    - We load save state of frame `x'`
    - We start playing back the recording until we reach frame `y`
- When the user edits TAM, all save states created starting from the edit are invalidated and deleted.

This worked quite well IMO with my POC tasarch for pokemon yellow. However, it will eat quite a lot of RAM and processing power for more demanding emulators such as dolphin.
Not sure if we can offer a better solution there though.
We should for sure have some good UI / UX there.
For example, a loading indicator across the emulator window with a loading bar showing the progress of frames rendered from `x'` to `y`.

### Breakpoints

IDEA: can be managed by gdb, by exposing / making them able to be set, via watchpoints on special addresses (64bit kernel addresses should work?) or gdb commands

### Comments & labels

Not sure if labels are really necessary, unless we want to be able to set breakpoints by label.
But IMO comments would be quite nice, just like how you might want to add comments in your exploit script.

# Proposed Design {#design}

I propose the format is designed as follows (C++ classes, not necessarily how it looks on disk!).

\note To solve the problem of polling input multiple times per frame, we introduce the notion of a "subframe".
Every time the input is polled (i.e. `retro_input_poll` called), a new subframe is started.
A new frame starts right before calling into `retro_run`.

```cpp
/*
    This captures the state of a singular input (e.g. a button on a controller or a touch on the screen) during a specific subframe.

    The saved properties directly correspond to the first four arguments of the `retro_input_state_t` callback.
    While with the additional structs introduced below, we would not need to necessarily save them again here, I think it could make sense for ease of access.
    But we will have to see if this bloats it too much.
    We could probably combine all these fields into a single uint64_t, since port is probably always smaller than 8, id smaller than 324 (keyboard max), index smaller than 10 (max num touches) and device at most 32 bits.
    Just counted, if we include everything, we have one bit too much :/.
    So we will have to see if it makes sense to worry about this lul.

    Since libretro didnt bother documenting any of the four parameters properly, I will also do that :)

    \todo Do we want to also save frame and subframe here?
*/
struct InputState {
    /*
        Player # / Port # of the corresponding device.

        MAX VAL: 8 (needs verifying)
        BITS: 3
    */
    uint32_t port;

    /*
        Type of which device to poll for. This can also be a custom one, but libretro api says those should have the subclass masked away lul.

        MAX VAL: UINT32_MAX (but if we consider that we mask the subclass away, then it would only be 255)
        BITS: 32 (8)
    */
    uint32_t device;

    /*
        The instance number of this specific input (i.e. a button that is somehow present multiple times?).
        This is only really used in two cases:
        - For touches, every index corresponds to a specific registered touch.
        - For analog, the index corresponds to the part of the analog controller queried (left stick, right stick and any of the buttons, clarified by id).
        This is imo kinda an abuse of the system, but whatever lul.

        MAX VAL: 10 (unless you somehow 11 fingers :P)
        BITS: 4
    */
    uint32_t index;

    /*
        The identifier of the part of the device (e.g. which button) queried
    */
    uint32_t id;

    /*
        The returned value. See libretro.h for details on what it means for different devices.
    */
    int16_t value;

    /*
        Whether this specific combination of port, device, index and id was actually read by the core.
        We can use this to detect potential desynchs:

        Let's imagine we record an input and (for some reason), the game only actually reads the inputs of two buttons of the controller, during recording.
        Now while playing back, it suddenly reads all inputs. Clearly this could lead to desynchs!
    */
    bool polled;
};

/*
    As previously established, the four parameters to the retro_input_state function can fit together in a 64 bit int.
    Therefore, we use this as an index into a hashmap.

    \note I would probably actually implement this as a custom class.
*/
using InputStateKey = uint64_t;

/*
    Represents all available information about any inputs we have for a given subframe.
*/
using FullInputState = std::unordered_map<InputStateKey, InputState>;

/*
    Metadata that can be present on both frames and subframes.
*/
struct MetaData {
    /*
        Short label identifying this
        \todo Do we really want this?
    */
    std::string label;

    /*
        Long form comment
    */
    std::string comment;

    /*
        List of breakpoints for this location.
        Why multiple? Well, since gdb allows that, we can to support that as well!
    */
    std::vector<tasarch::dbg::breakpoint> breakpoints;
};

struct SubFrame {
    // TODO: keep track of these?
    size_t frame;
    size_t subframe;

    MetaData metadata;

    FullInputState full_input_state;
};

/*
    This is to support stuff like, resetting the hardware, plugging in a new controller, etc.
    While this is represented using inheritance here, in e.g. protobuf this would be like a oneof.
    So this builds the base "struct"
*/
struct FrameEvent {};

/*
    Happens when the emulator is reset.
*/
struct ResetEvent : public FrameEvent {};

/*
    Happens when the controller is changed.
*/
struct SetControllerEvent : public FrameEvent {
    uint32_t port;
    uint32_t device;
};

/*
    This is only really for the server.
    Since we don't want to encode & save a lot of long videos, we should restrict the amount of time you can request to be recorded.
    This is done via this event.
*/
struct StartRecording : public FrameEvent {
    /*
        How many frames to record for.
    */
    size_t length;
}

struct Frame {
    size_t num;
    std::vector<SubFrame> sub_frames;
    // Processed befoe the frame is rendered
    std::vector<FrameEvent> events;
    // TODO save states?
};

struct ROMInfo {
    /*
        The name of the rom as reported by libretro.
    */
    std::string name;

    /*
        MD5 of the rom file, used for warnings.
        \todo This is actually kinda stupid, since for our purpose the rom on the server is always different!
    */
    std::string MD5;
}

struct CoreInfo {
    /*
        The name as reported by libretro.
    */
    std::string name;

    /*
        The version as reported by libretro.
    */
    std::string version;
}

struct SaveState {
    /*
        (Optional) name of the save state.
    */
    std::string name;

    /*
        (Optional) description of the save state.
        Both only really used for initial save state.
    */
    std::string description;

    /*
        We might want to have these be external to the input and stuff.
    */
    std::vector<uint8_t> data;
}

struct TAM {
    ROMInfo rom;
    CoreInfo core;
    SaveState initial;

    /*
        \todo What would this be exactly?
    */
    std::string team_token;
    
    std::vector<Frame> frames;
};
```

# Serialization

There are multiple possibilities for how we could serialize such a format to disk.
flatbuffers? protobuf? both probably require writing wrappers around the actual serialized classes.
Custom binary format? way more error prone lol