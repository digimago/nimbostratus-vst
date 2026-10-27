// Drives the built plugin through DPF's PluginExporter with MIDI notes and
// checks the output pitch, the Note Held lamp and Trigger activity.
#include "DistrhoPluginInternal.hpp"
#include "NimbostratusParams.h"

#include <cmath>
#include <cstdio>
#include <vector>

USE_NAMESPACE_DISTRHO

static const double kRate = 48000.0;
static const uint32_t kFrames = 256;
static double phase = 0.0;
static int failures = 0;

struct Host {
    PluginExporter plugin;
    std::vector<float> inL, inR, outL, outR;
    Host() : plugin(nullptr, nullptr, nullptr, nullptr),
             inL(kFrames), inR(kFrames), outL(kFrames), outR(kFrames) {}

    // Runs `seconds` of a 440 Hz sine; the events go into the first block.
    // Returns the output frequency estimated over the last half of the run,
    // and whether Trigger Activity lit at any point.
    double run(double seconds, std::vector<MidiEvent> events, bool* activity = nullptr)
    {
        const uint32_t blocks = static_cast<uint32_t>(seconds * kRate / kFrames);
        std::vector<float> tail;
        bool lit = false;
        for (uint32_t b = 0; b < blocks; ++b)
        {
            for (uint32_t i = 0; i < kFrames; ++i)
            {
                inL[i] = inR[i] = 0.5f * static_cast<float>(std::sin(phase));
                phase += 2.0 * M_PI * 440.0 / kRate;
            }
            const float* ins[2] = { inL.data(), inR.data() };
            float* outs[2] = { outL.data(), outR.data() };
            plugin.run(ins, outs, kFrames, b == 0 ? events.data() : nullptr,
                       b == 0 ? static_cast<uint32_t>(events.size()) : 0);
            lit = lit || plugin.getParameterValue(kParamActivity) > 0.5f;
            if (b >= blocks / 2)
                tail.insert(tail.end(), outL.begin(), outL.end());
        }
        if (activity) *activity = lit;
        // Rising zero crossings, with linear interpolation between samples.
        double first = -1, last = -1; int n = 0;
        for (size_t i = 1; i < tail.size(); ++i)
            if (tail[i - 1] < 0.0f && tail[i] >= 0.0f)
            {
                const double t = (i - 1) + tail[i - 1] / (tail[i - 1] - tail[i]);
                if (first < 0) first = t; else ++n;
                last = t;
            }
        return n > 0 ? n * kRate / (last - first) : 0.0;
    }
};

static MidiEvent note(bool on, uint8_t key)
{
    MidiEvent e = {};
    e.size = 3;
    e.data[0] = on ? 0x90 : 0x80;
    e.data[1] = key;
    e.data[2] = on ? 100 : 0;
    return e;
}

static void expectFreq(const char* what, double got, double want)
{
    const bool ok = std::fabs(got - want) / want < 0.02;
    std::printf("%-44s %7.1f Hz (want %6.1f)  %s\n", what, got, want, ok ? "ok" : "FAIL");
    failures += !ok;
}

static void expect(const char* what, bool cond)
{
    std::printf("%-44s %s\n", what, cond ? "ok" : "FAIL");
    failures += !cond;
}

int main()
{
    d_nextBufferSize = kFrames;
    d_nextSampleRate = kRate;
    Host h;
    h.plugin.setSampleRate(kRate);
    h.plugin.setBufferSize(kFrames);

    const auto held = [&] { return h.plugin.getParameterValue(kParamNoteHeld) > 0.5f; };
    const std::vector<MidiEvent> none;

    // Pitch/Stretch, fully wet, so the output pitch is the engine's alone.
    h.plugin.setParameterValue(kParamMode, 1.0f);
    h.plugin.setParameterValue(kParamDryWet, 1.0f);
    h.plugin.activate();

    expect("defaults to Off", h.plugin.getParameterValue(kParamNoteInput) == 0.0f);
    expectFreq("Off: note 72 ignored", h.run(1.5, { note(true, 72) }), 440.0);
    expect("Off: lamp stays dark", !held());
    h.run(0.1, { note(false, 72) });

    h.plugin.setParameterValue(kParamNoteInput, kNoteInputPitch);
    expectFreq("Pitch: no note yet, knob pitch", h.run(1.0, none), 440.0);
    bool act = false;
    expectFreq("Pitch: note 72 -> +12", h.run(1.5, { note(true, 72) }, &act), 880.0);
    expect("Pitch: lamp lit while held", held());
    expect("Pitch: note-on does not fire Trigger", !act);
    expectFreq("Pitch: 67 over 72 -> +7", h.run(1.5, { note(true, 67) }), 440.0 * std::pow(2.0, 7.0 / 12.0));
    expectFreq("Pitch: release 67 -> back to 72", h.run(1.5, { note(false, 67) }), 880.0);
    expectFreq("Pitch: release 72 -> holds +12", h.run(1.5, { note(false, 72) }), 880.0);
    expect("Pitch: lamp dark after release", !held());

    h.plugin.setParameterValue(kParamPitch, -12.0f);
    expectFreq("Pitch: knob -12 + held note +12", h.run(1.5, none), 440.0);
    h.plugin.setParameterValue(kParamPitch, 0.0f);

    h.plugin.setParameterValue(kParamNoteInput, kNoteInputOff);
    expectFreq("Off again: back to the knob", h.run(1.5, none), 440.0);

    h.plugin.setParameterValue(kParamNoteInput, kNoteInputPitchTrigger);
    act = false;
    expectFreq("Pitch+Trig: note 48 -> -12", h.run(1.5, { note(true, 48) }, &act), 220.0);
    expect("Pitch+Trig: note-on fires Trigger", act);

    MidiEvent allOff = {};
    allOff.size = 3; allOff.data[0] = 0xB0; allOff.data[1] = 123;
    h.run(0.1, { allOff });
    expect("All Notes Off clears the lamp", !held());

    // Far out of range: knob +24 and note 127 (+67) must clamp, not blow up.
    h.plugin.setParameterValue(kParamPitch, 24.0f);
    const double f = h.run(1.0, { note(true, 127) });
    expect("extreme pitch stays finite", std::isfinite(f));

    std::printf(failures ? "\n%d FAILED\n" : "\nall note input checks passed\n", failures);
    return failures != 0;
}
