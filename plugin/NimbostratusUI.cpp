// Dear ImGui interface for the Nimbostratus plugin.
//
// Knob labels follow the active playback mode, mirroring how the hardware
// repurposes its pots per mode.

#include "DistrhoUI.hpp"

#include "NimbostratusParams.h"

#include "DearImGuiKnobs/imgui-knobs.h"

#if defined(DISTRHO_OS_MAC) || defined(DISTRHO_OS_WINDOWS)
# define NIMBO_KEY_PASSTHROUGH 1
# include "NimbostratusKeyboard.h"
#endif

#include <cstdio>
#include <cstring>

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------

static const char* const kModeNames[8] = {
    "Granular", "Stretch", "Loop Delay", "Spectral",
    "Oliverb", "Resonestor", "Beat Repeat", "Spct Cloud"
};

static const char* const kModeHints[8] = {
    "Classic granular texture synthesis",
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
static const char* const kInGainTip =
    "Trim before the engine. Pushing it drives the buffer and the feedback "
    "path harder";
static const char* const kOutGainTip =
    "Trim after the engine. +9 dB matches bypass at 100% dry";
static const char* const kQualityTip =
    "Buffer quality vs. length: mono and 8-bit u-law extend recording time and add vintage grit";

class NimbostratusUI : public UI
{
public:
    NimbostratusUI()
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
        std::memset(editing_, 0, sizeof(editing_));
        values_[kParamPosition] = 0.5f;
        values_[kParamSize]     = 0.5f;
        values_[kParamDensity]  = 0.5f;
        values_[kParamTexture]  = 0.5f;
        values_[kParamDryWet]   = 0.5f;

        setupStyle(static_cast<float>(scaleFactor));
    }

   #ifdef NIMBO_KEY_PASSTHROUGH
    ~NimbostratusUI() override
    {
        // The window outlives the widget, so its handle is still good here.
        nimboReleaseKeyboard(getWindow().getNativeWindowHandle());
    }
   #endif

protected:
    // Keys can reach a plugin two ways: through the host's plugin API, or
    // straight to the native window. This is the plugin-API route, and what
    // it returns tells the host whether the key was used - a host that is
    // told yes keeps its own shortcuts to itself.
    //
    // The base implementation answers with io.WantCaptureKeyboard, which is
    // broader than this UI needs. Feed the key to Dear ImGui either way, but
    // only claim it while a value is actually being typed, matching what the
    // native hook does on the other route.
    bool onKeyboard(const KeyboardEvent& ev) override
    {
        const bool used = UI::onKeyboard(ev);

        return used && ImGui::GetIO().WantTextInput;
    }

    void parameterChanged(uint32_t index, float value) override
    {
        // A host echoes back what it was just sent. Taking that while the
        // knob or its numeric field is held would have the echo fight the
        // gesture in progress.
        if (index < kParamCount && !editing_[index])
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

        ImGui::Begin("Nimbostratus", nullptr,
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
        tooltip(kQualityTip);

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
            tooltip(kModeHints[m]);
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
        tooltip(kFreezeTip);

        const bool reverse = values_[kParamReverse] > 0.5f;
        if (toggleButton("REVERSE", reverse, buttonSize))
            setBoolParameter(kParamReverse, !reverse);
        tooltip(kReverseTip);

        if (toggleButton("SYNC", sync, buttonSize))
            setBoolParameter(kParamSync, !sync);
        tooltip(kSyncTip);

        // Lit while held, and flashes on every trigger fired by the engine
        // (reported through the Trigger Activity output parameter).
        const bool trigLit = triggerHeld_ || values_[kParamActivity] > 0.5f;
        toggleButton("TRIG", trigLit, buttonSize);
        tooltip(kTrigTip);
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

        ImGui::SameLine(0.0f, 30.0f * s);
        knob(kParamInputGain, "In Gain", -24.0f, 24.0f, smallKnob, "%+.1f dB",
             /*withInput*/ true, kInGainTip);
        ImGui::SameLine();
        knob(kParamOutputGain, "Out Gain", -24.0f, 24.0f, smallKnob, "%+.1f dB",
             /*withInput*/ true, kOutGainTip);

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.40f, 0.40f, 0.40f, 1.0f),
            "Based on the excellent open-source work of Emilie Gillet - "
            "with community extensions by Matthias Puech & Julian Kammerl");

        // Version, right-aligned on the credit line. That row is the last
        // thing in the window and the height has to leave room for it - it
        // used to start below the bottom edge, which hid the credits too.
        char version[24];
        std::snprintf(version, sizeof(version), "v%d.%d.%d",
                      NIMBO_VERSION_MAJOR, NIMBO_VERSION_MINOR,
                      NIMBO_VERSION_PATCH);
        ImGui::SameLine(width - ImGui::CalcTextSize(version).x
                              - ImGui::GetStyle().WindowPadding.x);
        ImGui::TextColored(ImVec4(0.40f, 0.40f, 0.40f, 1.0f), "%s", version);

        ImGui::End();

       #ifdef NIMBO_KEY_PASSTHROUGH
        // Keep the host's key commands working while the window is open: only
        // hold on to the keyboard while a value is actually being typed into
        // the UI, otherwise the spacebar would never reach the DAW's transport.
        const bool capturing = ImGui::GetIO().WantTextInput;
        nimboSetKeyboardCapture(getWindow().getNativeWindowHandle(), capturing);

        // Key presses only reach Dear ImGui while that capture is on, so the
        // moment it ends anything still held is a release that went somewhere
        // else. Both shims now keep a release with its press, but neither can
        // see one that never arrives at all - a key let go after the host has
        // taken the window's focus, say. A key stuck down is not a harmless
        // leftover: Enter and Escape auto-repeat inside Dear ImGui, and either
        // one closes a numeric field the instant it opens.
        if (wasCapturing_ && ! capturing)
            releaseStuckKeys();
        wasCapturing_ = capturing;
       #endif
    }

private:
    // ImGui lays a tooltip out as a single unwrapped line, so the longer tips
    // ran far wider than the 820px window. Wrap them at a readable measure.
    void tooltip(const char* text)
    {
        if (text == nullptr || !ImGui::BeginItemTooltip())
            return;
        ImGui::PushTextWrapPos(300.0f * static_cast<float>(getScaleFactor()));
        ImGui::TextUnformatted(text);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }

   #ifdef NIMBO_KEY_PASSTHROUGH
    // Text keys only, and only on the edge where capture ends. Modifiers are
    // left alone: they are legitimately held across that edge (a ctrl-click
    // elsewhere is one way to end text entry) and every later event carries
    // fresh modifier state anyway. Mouse buttons share the key enum and must
    // not be touched at all.
    static void releaseStuckKeys()
    {
        ImGuiIO& io = ImGui::GetIO();

        for (int k = ImGuiKey_NamedKey_BEGIN; k < ImGuiKey_NamedKey_END; ++k)
        {
            const ImGuiKey key = static_cast<ImGuiKey>(k);

            if (key >= ImGuiKey_MouseLeft && key <= ImGuiKey_MouseWheelY)
                continue;
            if (key >= ImGuiKey_LeftCtrl && key <= ImGuiKey_RightSuper)
                continue;
            if (ImGui::IsKeyDown(key))
                io.AddKeyEvent(key, false);
        }
    }
   #endif

    // A begin/end edit pair has to reach the host balanced. An end with no
    // begin leaves Ableton Live holding a gesture that was never opened, and
    // the next click on the UI lands in that stale state -- which is what made
    // the gain fields feel unreliable after a value had been typed or dragged.
    // Track the pair here instead of inferring it from the widget's return
    // value, so both calls happen exactly once whatever ImGui reports.
    void beginGesture(uint32_t index)
    {
        if (editing_[index])
            return;
        editing_[index] = true;
        editParameter(index, true);
    }

    void endGesture(uint32_t index)
    {
        if (!editing_[index])
            return;
        editing_[index] = false;
        editParameter(index, false);
    }

    void setValue(uint32_t index, float value)
    {
        beginGesture(index);
        values_[index] = value;
        setParameterValue(index, value);
    }

    void knob(uint32_t index, const char* label,
              float vmin, float vmax, float size, const char* fmt,
              bool withInput = false, const char* tipText = nullptr)
    {
        float value = values_[index];
        // The widget draws the label verbatim, so keep it clean and use an
        // id scope for uniqueness instead of "##" suffixes.
        ImGui::PushID(static_cast<int>(index));
        const ImGuiKnobFlags flags = withInput ? 0 : ImGuiKnobFlags_NoInput;
        const bool changed = ImGuiKnobs::Knob(label, &value, vmin, vmax,
                                              (vmax - vmin) / 254.0f, fmt,
                                              ImGuiKnobVariant_WiperOnly,
                                              size, flags);
        // Read the item state while it still describes this knob, before the
        // tooltip pushes a window of its own.
        const bool activated = ImGui::IsItemActivated();
        const bool active    = ImGui::IsItemActive();
        const bool hovered   = ImGui::IsItemHovered();
        const ImVec2 rectMin = ImGui::GetItemRectMin();
        ImVec2 resetMax      = ImGui::GetItemRectMax();

        // Open the gesture on mouse-down, not on the first value change: the
        // widget reports activation and movement on different frames, so the
        // two cannot be driven from one test.
        if (activated)
            beginGesture(index);
        if (changed)
            setValue(index, value);
        tooltip(tipText);
        // Closing on "no longer active" rather than on IsItemDeactivated()
        // also covers the frame a typed value is committed, where the field
        // deactivates and reports its change at once.
        if (!active)
            endGesture(index);
        // Double-click resets to default. ImGuiKnobs::Knob wraps title, knob
        // and (when withInput) the numeric field in a group, so the item rect
        // here covers all three -- testing it directly would make a
        // double-click inside the text field, the ordinary way to select what
        // you typed, reset the parameter instead. Trim the field off the
        // bottom and ignore the gesture outright while text entry is live.
        if (withInput)
            resetMax.y -= ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y;
        if (!ImGui::GetIO().WantTextInput
            && hovered
            && ImGui::IsMouseHoveringRect(rectMin, resetMax)
            && ImGui::IsMouseDoubleClicked(0))
        {
            const float def =
                index == kParamPosition || index == kParamSize ||
                index == kParamDensity || index == kParamTexture ||
                index == kParamDryWet ? 0.5f : 0.0f;
            setValue(index, def);
            endGesture(index);
        }
        ImGui::PopID();
    }

    // Density while tempo-synced: a stepped knob that clicks through the
    // clock divisions directly instead of sweeping a continuous range.
    void steppedRateKnob(const char* label, float size)
    {
        int idx = syncDivIndex(values_[kParamDensity]);
        ImGui::PushID(static_cast<int>(kParamDensity));
        const bool changed = ImGuiKnobs::KnobInt(label, &idx, 0,
                                                 kNumSyncDivisions - 1,
                                                 0.1f, "", ImGuiKnobVariant_Stepped,
                                                 size, ImGuiKnobFlags_NoInput,
                                                 kNumSyncDivisions);
        const bool activated = ImGui::IsItemActivated();
        const bool active    = ImGui::IsItemActive();
        if (activated)
            beginGesture(kParamDensity);
        if (changed)
            setValue(kParamDensity, static_cast<float>(idx)
                                  / static_cast<float>(kNumSyncDivisions - 1));
        tooltip(kSyncedDensityTip);
        if (!active)
            endGesture(kParamDensity);
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
    bool editing_[kParamCount];
    bool triggerHeld_;
   #ifdef NIMBO_KEY_PASSTHROUGH
    bool wasCapturing_ = false;
   #endif

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NimbostratusUI)
};

UI* createUI()
{
    return new NimbostratusUI();
}

END_NAMESPACE_DISTRHO
