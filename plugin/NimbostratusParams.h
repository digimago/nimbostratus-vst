// Parameter ids shared between DSP and UI.

#ifndef NIMBOSTRATUS_PARAMS_H_INCLUDED
#define NIMBOSTRATUS_PARAMS_H_INCLUDED

enum ParamId {
    kParamPosition = 0,
    kParamSize,
    kParamPitch,
    kParamDensity,
    kParamTexture,
    kParamDryWet,
    kParamSpread,
    kParamFeedback,
    kParamReverb,
    kParamFreeze,
    kParamTrigger,
    kParamMode,
    kParamQuality,
    kParamReverse,
    kParamSlice,
    kParamSync,
    kParamActivity,  // output-only: lights the TRIG button on every trigger
    // Appended last on purpose: CLAP ids are the raw parameter index and VST3
    // ids are that index plus a fixed offset, so inserting above this point
    // would move every later parameter and break automation in saved sessions.
    kParamInputGain,
    kParamOutputGain,
    kParamCount
};

// Bounds every parameter value to the range its declaration in initParameter()
// gives it. Hosts restore saved projects and send CLAP events without checking
// those ranges, and the firmware DSP indexes lookup tables straight from these
// values (as does the UI, from the mode), so an out-of-range or NaN value in a
// crafted project would read out of bounds. Keep in step with initParameter().
static inline float clampParameterValue(const uint32_t index, const float value)
{
    float min = 0.0f, max = 1.0f, def = 0.0f;

    switch (index)
    {
    case kParamPosition:
    case kParamSize:
    case kParamDensity:
    case kParamTexture:
    case kParamDryWet:
        def = 0.5f;
        break;
    case kParamPitch:
    case kParamInputGain:
    case kParamOutputGain:
        min = -24.0f; max = 24.0f;
        break;
    case kParamMode:
        max = 7.0f;
        break;
    case kParamQuality:
        max = 3.0f;
        break;
    }

    if (value != value) // NaN
        return def;
    return value < min ? min : value > max ? max : value;
}

// Tempo-sync clock divisions selected by the Density knob while Sync is on.
enum { kNumSyncDivisions = 9 };

static const char* const kSyncDivLabels[kNumSyncDivisions] = {
    "4 bars", "2 bars", "1 bar", "1/2", "1/4", "1/8", "1/8T", "1/16", "1/32"
};

static inline int syncDivIndex(float density01)
{
    int idx = static_cast<int>(density01 * (kNumSyncDivisions - 1) + 0.5f);
    if (idx < 0) idx = 0;
    if (idx > kNumSyncDivisions - 1) idx = kNumSyncDivisions - 1;
    return idx;
}

// Division length in host beats (quarter notes at 4/4).
static inline double syncDivisionBeats(const int idx, const double beatsPerBar)
{
    switch (idx)
    {
    case 0: return 4.0 * beatsPerBar;
    case 1: return 2.0 * beatsPerBar;
    case 2: return beatsPerBar;
    case 3: return 2.0;
    case 4: return 1.0;
    case 5: return 0.5;
    case 6: return 1.0 / 3.0;
    case 7: return 0.25;
    default: return 0.125;
    }
}

#endif // NIMBOSTRATUS_PARAMS_H_INCLUDED
