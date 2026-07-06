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
    kParamCount
};

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
