// Dear ImGui interface for the Clouds / SuperParasites plugin.
//
// Knob labels follow the active playback mode, mirroring how the hardware
// repurposes its pots per mode.

#include "DistrhoUI.hpp"

#include "CloudsParams.h"

#include "DearImGuiKnobs/imgui-knobs.h"

#include <cstdio>
#include <cstring>

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------

static const char* const kModeNames[8] = {
    "Granular", "Stretch", "Loop Delay", "Spectral",
    "Oliverb", "Resonestor", "Beat Repeat", "Spct Cloud"
};

static const char* const kModeHints[8] = {
    "Classic Clouds granular texture synthesis",
    "WSOLA pitch shifter / time stretcher",
    "Looping delay with overdub",
    "Spectral madness: FFT magnitude warping",
    "Parasites lush modulated reverb",
    "Parasites polyphonic resonator (send triggers!)",
    "Kammerl beat repeat: slice looper (needs regular triggers)",
    "Spectral cloud: band-gated spectral freeze/blur"
};

// Per-mode labels for the 9 continuous knobs, in ParamId order
// Position, Size, Pitch, Density, Texture, DryWet, Spread, Feedback, Reverb.
static const char* const kKnobLabels[8][9] = {
    {"Position", "Size", "Pitch", "Density", "Texture",
     "Dry/Wet", "Spread", "Feedback", "Reverb"},
    {"Position", "Size", "Pitch", "Diffusion", "Filter",
     "Dry/Wet", "Spread", "Feedback", "Reverb"},
    {"Position", "Loop Size", "Pitch", "Granulation", "Filter",
     "Dry/Wet", "Spread", "Feedback", "Reverb"},
    {"Buffer", "Warp", "Pitch", "Refresh", "Quantize",
     "Dry/Wet", "Spread", "Feedback", "Reverb"},
    {"Pre-Delay", "Size", "Shift", "Decay", "Filter",
     "Dry/Wet", "Diffusion", "Mod Rate", "Mod Amt"},
    {"Burst", "Chord", "Pitch", "Decay", "Filter",
     "Distortion", "Stereo/Sep", "Harmonics", "Spread"},
    {"Position", "Slice Size", "Pitch", "Size Mod", "Slice Mod",
     "Probability", "Clock Div", "Pitch Mode", "Distortion"},
    {"Band Gate", "Warp", "Pitch", "Smooth", "Texture",
     "Dry/Wet", "Spread", "Distortion", "Reverb"}
};

static const char* const kQualityNames[4] = {
    "16-bit Stereo (1s)", "16-bit Mono (2s)",
    "8-bit u-law Stereo (4s)", "8-bit u-law Mono (8s)"
};

// Per-mode tooltips for the 9 continuous knobs, same order as kKnobLabels.
static const char* const kKnobTips[8][9] = {
    { // Granular
      "Playback position in the recording buffer: left = now, right = further back in time",
      "Grain size, from tiny shards to about one second",
      "Grain transposition in semitones",
      "How grains fire: center = silence, right = increasingly dense random grains, left = evenly clocked grains",
      "Grain window shape: percussive to smooth; fully right adds diffusion",
      "Dry/wet balance",
      "Random stereo panning of individual grains",
      "Feeds output back into the buffer; self-oscillates near max",
      "Reverb amount" },
    { // Pitch/Stretch
      "Playback position in the buffer",
      "Time-stretch window size",
      "Transposition in semitones, time-independent",
      "Diffuser amount, smears transients",
      "Filter tilt: low-pass left, high-pass right",
      "Dry/wet balance",
      "Stereo image width",
      "Feedback into the buffer",
      "Reverb amount" },
    { // Looping Delay
      "Delay tap position / loop start",
      "Loop length",
      "Playback transposition in semitones",
      "Granulation of the loop content",
      "Filter tilt: low-pass left, high-pass right",
      "Dry/wet balance",
      "Stereo spread",
      "Delay regeneration; runaway near max",
      "Reverb amount" },
    { // Spectral
      "Position in the spectral buffer",
      "Spectral warp: shifts the magnitude spectrum non-linearly",
      "Transposition in semitones",
      "Spectrum refresh rate: low values smear and freeze",
      "Quantizes spectral magnitudes for robotic, metallic textures",
      "Dry/wet balance",
      "Stereo spread",
      "Feedback amount",
      "Reverb amount" },
    { // Oliverb
      "Pre-delay before the reverb onset",
      "Room size",
      "Pitch shift inside the reverb tail (shimmer)",
      "Reverb decay time",
      "Damping filter: low-pass left, high-pass right",
      "Dry/wet balance",
      "Diffusion of the early reflections",
      "Modulation rate of the reverb lines",
      "Modulation depth of the reverb lines" },
    { // Resonestor
      "Excitation burst character: damping, comb and duration",
      "Chord selection for the resonator voices",
      "Root pitch of the resonators in semitones",
      "Resonator decay time",
      "Damping/narrowness of the resonator filter",
      "Distortion of the resonator output",
      "Left = voice separation, right = stereo width",
      "Harmonicity: pure partials left, detuned right",
      "Spread amount across voices" },
    { // Beat Repeat
      "Position within the recording buffer",
      "Length of the repeated slice",
      "Slice playback pitch",
      "Modulates repeat length from cycle to cycle",
      "How the repeated slice steps between cycles; max = random jumps",
      "Probability that a repeat engages on each trigger",
      "Divides the incoming trigger clock",
      "Pitch behavior of repeats: constant, ramp down, stutter...",
      "Distortion amount on the repeats" },
    { // Spectral Cloud
      "Band gate threshold: left mutes more quiet spectral bands",
      "Spectral warp of the magnitude spectrum",
      "Transposition in semitones",
      "Smooths spectral changes over time; high = frozen wash",
      "Spectral texture amount",
      "Dry/wet balance",
      "Stereo spread",
      "Warm distortion amount",
      "Reverb amount" }
};

static const char* const kSyncedDensityTip =
    "Trigger clock divider (tempo-synced). Steps through 4 bars .. 1/32";
static const char* const kSliceTip =
    "Selects which of the 8 recorded slices gets repeated. Active in Beat Repeat mode only";
static const char* const kFreezeTip =
    "Stops recording and loops the current buffer contents";
static const char* const kReverseTip =
    "Plays grains and loops backwards (granular / looping modes)";
static const char* const kSyncTip =
    "Locks the trigger clock to the host tempo; Density becomes the clock divider";
static const char* const kTrigTip =
    "Manual trigger: fires a grain, excitation or repeat. Flashes on every trigger, including synced ones";
static const char* const kQualityTip =
    "Buffer quality vs. length: mono and 8-bit u-law extend recording time and add vintage grit";

class CloudsUI : public UI
{
public:
    CloudsUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT),
          triggerHeld_(false)
    {
        // Fixed-size window; hi-DPI comes only from the host/OS scale factor.
        // The ImGuiWidget wrapper already scales fonts and style metrics by
        // getScaleFactor(), so layout code multiplies its own pixel sizes by
        // the same factor and must not add any scaling on top.
        const double scaleFactor = getScaleFactor();
        if (d_isNotEqual(scaleFactor, 1.0))
            setSize(DISTRHO_UI_DEFAULT_WIDTH * scaleFactor,
                    DISTRHO_UI_DEFAULT_HEIGHT * scaleFactor);

        std::memset(values_, 0, sizeof(values_));
        values_[kParamPosition] = 0.5f;
        values_[kParamSize]     = 0.5f;
        values_[kParamDensity]  = 0.5f;
        values_[kParamTexture]  = 0.5f;
        values_[kParamDryWet]   = 0.5f;

        setupStyle(static_cast<float>(scaleFactor));
    }

protected:
    void parameterChanged(uint32_t index, float value) override
    {
        if (index < kParamCount)
            values_[index] = value;
        repaint();
    }

    void onImGuiDisplay() override
    {
        const float width  = getWidth();
        const float height = getHeight();
        const float s = static_cast<float>(getScaleFactor());

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(width, height));

        ImGui::Begin("Clouds", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoScrollWithMouse);

        const int mode = static_cast<int>(values_[kParamMode] + 0.5f);

        // ------------------------------------------------------ header
        ImGui::PushFont(nullptr);
        ImGui::TextColored(ImVec4(0.95f, 0.93f, 0.88f, 1.0f), "N I M B O S T R A T U S");
        ImGui::PopFont();
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.45f, 0.75f, 0.78f, 1.0f), " granular texture processor");
        ImGui::SameLine(width - 250.0f * s);
        ImGui::SetNextItemWidth(240.0f * s);
        int quality = static_cast<int>(values_[kParamQuality] + 0.5f);
        if (ImGui::Combo("##quality", &quality, kQualityNames, 4))
            setIntParameter(kParamQuality, quality);
        ImGui::SetItemTooltip("%s", kQualityTip);

        ImGui::Spacing();

        // ------------------------------------------------- mode buttons
        for (int m = 0; m < 8; ++m)
        {
            if (m > 0)
                ImGui::SameLine();
            const bool active = m == mode;
            if (active)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.45f, 0.48f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.52f, 0.55f, 1.0f));
            }
            if (ImGui::Button(kModeNames[m], ImVec2(86.0f * s, 26.0f * s)))
                setIntParameter(kParamMode, m);
            ImGui::SetItemTooltip("%s", kModeHints[m]);
            if (active)
                ImGui::PopStyleColor(2);
        }

        ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.55f, 1.0f), "%s", kModeHints[mode]);
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // --------------------------------------------------- main knobs
        const float bigKnob = 78.0f * s;
        knob(kParamPosition, kKnobLabels[mode][0], 0.0f, 1.0f, bigKnob, "%.2f",
             false, kKnobTips[mode][0]);
        ImGui::SameLine();
        knob(kParamSize, kKnobLabels[mode][1], 0.0f, 1.0f, bigKnob, "%.2f",
             false, kKnobTips[mode][1]);
        ImGui::SameLine();
        knob(kParamPitch, kKnobLabels[mode][2], -24.0f, 24.0f, bigKnob, "%.2f st",
             /*withInput*/ true, kKnobTips[mode][2]);
        ImGui::SameLine();
        // While synced, Density is the trigger clock divider: a stepped knob
        // that clicks through the divisions, labeled with the active one.
        const bool sync = values_[kParamSync] > 0.5f;
        if (sync)
        {
            char densityLabel[24];
            std::snprintf(densityLabel, sizeof(densityLabel), "Rate %s",
                          kSyncDivLabels[syncDivIndex(values_[kParamDensity])]);
            steppedRateKnob(densityLabel, bigKnob);
        }
        else
        {
            knob(kParamDensity, kKnobLabels[mode][3], 0.0f, 1.0f, bigKnob,
                 "%.2f", false, kKnobTips[mode][3]);
        }
        ImGui::SameLine();
        knob(kParamTexture, kKnobLabels[mode][4], 0.0f, 1.0f, bigKnob, "%.2f",
             false, kKnobTips[mode][4]);

        ImGui::SameLine(0.0f, 30.0f * s);

        // ------------------------------------------------ buttons block
        // FREEZE and REVERSE latch, TRIG is momentary; all three share the
        // same button styling so the panel reads as one control family.
        const ImVec2 buttonSize(108.0f * s, 30.0f * s);
        ImGui::BeginGroup();
        const bool freeze = values_[kParamFreeze] > 0.5f;
        if (toggleButton("FREEZE", freeze, buttonSize))
            setBoolParameter(kParamFreeze, !freeze);
        ImGui::SetItemTooltip("%s", kFreezeTip);

        const bool reverse = values_[kParamReverse] > 0.5f;
        if (toggleButton("REVERSE", reverse, buttonSize))
            setBoolParameter(kParamReverse, !reverse);
        ImGui::SetItemTooltip("%s", kReverseTip);

        if (toggleButton("SYNC", sync, buttonSize))
            setBoolParameter(kParamSync, !sync);
        ImGui::SetItemTooltip("%s", kSyncTip);

        // Lit while held, and flashes on every trigger fired by the engine
        // (reported through the Trigger Activity output parameter).
        const bool trigLit = triggerHeld_ || values_[kParamActivity] > 0.5f;
        toggleButton("TRIG", trigLit, buttonSize);
        ImGui::SetItemTooltip("%s", kTrigTip);
        const bool trigNow = ImGui::IsItemActive();
        if (trigNow != triggerHeld_)
        {
            triggerHeld_ = trigNow;
            editParameter(kParamTrigger, true);
            setParameterValue(kParamTrigger, trigNow ? 1.0f : 0.0f);
            editParameter(kParamTrigger, false);
            values_[kParamTrigger] = trigNow ? 1.0f : 0.0f;
        }
        ImGui::EndGroup();

        ImGui::Spacing();

        // -------------------------------------------------- blend knobs
        const float smallKnob = 58.0f * s;
        knob(kParamDryWet, kKnobLabels[mode][5], 0.0f, 1.0f, smallKnob, "%.2f",
             false, kKnobTips[mode][5]);
        ImGui::SameLine();
        knob(kParamSpread, kKnobLabels[mode][6], 0.0f, 1.0f, smallKnob, "%.2f",
             false, kKnobTips[mode][6]);
        ImGui::SameLine();
        knob(kParamFeedback, kKnobLabels[mode][7], 0.0f, 1.0f, smallKnob, "%.2f",
             false, kKnobTips[mode][7]);
        ImGui::SameLine();
        knob(kParamReverb, kKnobLabels[mode][8], 0.0f, 1.0f, smallKnob, "%.2f",
             false, kKnobTips[mode][8]);
        ImGui::SameLine(0.0f, 30.0f * s);
        if (mode != 6)
            ImGui::BeginDisabled();
        knob(kParamSlice, "Slice", 0.0f, 1.0f, smallKnob, "%.2f",
             false, kSliceTip);
        if (mode != 6)
            ImGui::EndDisabled();

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.40f, 0.40f, 0.40f, 1.0f),
            "Based on the excellent open-source work of Emilie Gillet - "
            "with community extensions by Matthias Puech & Julian Kammerl");

        ImGui::End();
    }

private:
    void knob(uint32_t index, const char* label,
              float vmin, float vmax, float size, const char* fmt,
              bool withInput = false, const char* tipText = nullptr)
    {
        float value = values_[index];
        // The widget draws the label verbatim, so keep it clean and use an
        // id scope for uniqueness instead of "##" suffixes.
        ImGui::PushID(static_cast<int>(index));
        const ImGuiKnobFlags flags = withInput ? 0 : ImGuiKnobFlags_NoInput;
        if (ImGuiKnobs::Knob(label, &value, vmin, vmax,
                             (vmax - vmin) / 254.0f, fmt,
                             ImGuiKnobVariant_WiperOnly, size, flags))
        {
            if (ImGui::IsItemActivated())
                editParameter(index, true);
            values_[index] = value;
            setParameterValue(index, value);
        }
        if (tipText != nullptr)
            ImGui::SetItemTooltip("%s", tipText);
        if (ImGui::IsItemDeactivated())
            editParameter(index, false);
        // Double-click resets to default.
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
        {
            const float def =
                index == kParamPosition || index == kParamSize ||
                index == kParamDensity || index == kParamTexture ||
                index == kParamDryWet ? 0.5f : 0.0f;
            editParameter(index, true);
            values_[index] = def;
            setParameterValue(index, def);
            editParameter(index, false);
        }
        ImGui::PopID();
    }

    // Density while tempo-synced: a stepped knob that clicks through the
    // clock divisions directly instead of sweeping a continuous range.
    void steppedRateKnob(const char* label, float size)
    {
        int idx = syncDivIndex(values_[kParamDensity]);
        ImGui::PushID(static_cast<int>(kParamDensity));
        if (ImGuiKnobs::KnobInt(label, &idx, 0, kNumSyncDivisions - 1,
                                0.1f, "", ImGuiKnobVariant_Stepped, size,
                                ImGuiKnobFlags_NoInput, kNumSyncDivisions))
        {
            if (ImGui::IsItemActivated())
                editParameter(kParamDensity, true);
            const float v = static_cast<float>(idx)
                          / static_cast<float>(kNumSyncDivisions - 1);
            values_[kParamDensity] = v;
            setParameterValue(kParamDensity, v);
        }
        ImGui::SetItemTooltip("%s", kSyncedDensityTip);
        if (ImGui::IsItemDeactivated())
            editParameter(kParamDensity, false);
        ImGui::PopID();
    }

    // Uniform latching button: same look as the momentary TRIG button,
    // accent-colored while engaged.
    bool toggleButton(const char* label, bool active, const ImVec2& size)
    {
        if (active)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.45f, 0.48f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.52f, 0.55f, 1.0f));
        }
        const bool clicked = ImGui::Button(label, size);
        if (active)
            ImGui::PopStyleColor(2);
        return clicked;
    }

    void setIntParameter(uint32_t index, int value)
    {
        editParameter(index, true);
        values_[index] = static_cast<float>(value);
        setParameterValue(index, values_[index]);
        editParameter(index, false);
    }

    void setBoolParameter(uint32_t index, bool value)
    {
        editParameter(index, true);
        values_[index] = value ? 1.0f : 0.0f;
        setParameterValue(index, values_[index]);
        editParameter(index, false);
    }

    static void setupStyle(const float s)
    {
        ImGuiStyle& st = ImGui::GetStyle();
        st.WindowRounding = 0.0f;
        st.FrameRounding = 4.0f * s;
        st.GrabRounding = 4.0f * s;
        st.WindowPadding = ImVec2(14 * s, 10 * s);
        st.ItemSpacing = ImVec2(10 * s, 8 * s);

        ImVec4* c = st.Colors;
        c[ImGuiCol_WindowBg]      = ImVec4(0.09f, 0.10f, 0.11f, 1.00f);
        c[ImGuiCol_Text]          = ImVec4(0.86f, 0.86f, 0.84f, 1.00f);
        c[ImGuiCol_Button]        = ImVec4(0.16f, 0.18f, 0.20f, 1.00f);
        c[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.27f, 0.29f, 1.00f);
        c[ImGuiCol_ButtonActive]  = ImVec4(0.16f, 0.45f, 0.48f, 1.00f);
        c[ImGuiCol_FrameBg]       = ImVec4(0.15f, 0.16f, 0.18f, 1.00f);
        c[ImGuiCol_FrameBgHovered]= ImVec4(0.22f, 0.24f, 0.26f, 1.00f);
        c[ImGuiCol_FrameBgActive] = ImVec4(0.16f, 0.45f, 0.48f, 1.00f);
        c[ImGuiCol_SliderGrab]    = ImVec4(0.45f, 0.75f, 0.78f, 1.00f);
        c[ImGuiCol_SliderGrabActive] = ImVec4(0.55f, 0.85f, 0.88f, 1.00f);
        c[ImGuiCol_Header]        = ImVec4(0.16f, 0.45f, 0.48f, 1.00f);
        c[ImGuiCol_HeaderHovered] = ImVec4(0.20f, 0.52f, 0.55f, 1.00f);
        c[ImGuiCol_PopupBg]       = ImVec4(0.11f, 0.12f, 0.13f, 0.98f);
        c[ImGuiCol_Separator]     = ImVec4(0.25f, 0.27f, 0.29f, 1.00f);
    }

    float values_[kParamCount];
    bool triggerHeld_;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CloudsUI)
};

UI* createUI()
{
    return new CloudsUI();
}

END_NAMESPACE_DISTRHO
