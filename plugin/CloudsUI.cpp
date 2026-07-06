// Dear ImGui interface for the Clouds / SuperParasites plugin.
//
// Knob labels follow the active playback mode, mirroring how the hardware
// repurposes its pots per mode.

#include "DistrhoUI.hpp"

#include "CloudsParams.h"

#include "DearImGuiKnobs/imgui-knobs.h"

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

class CloudsUI : public UI
{
public:
    CloudsUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT),
          triggerHeld_(false)
    {
        const double scaleFactor = getScaleFactor();
        if (d_isNotEqual(scaleFactor, 1.0))
            setSize(DISTRHO_UI_DEFAULT_WIDTH * scaleFactor,
                    DISTRHO_UI_DEFAULT_HEIGHT * scaleFactor);
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH * scaleFactor,
                               DISTRHO_UI_DEFAULT_HEIGHT * scaleFactor, true);

        std::memset(values_, 0, sizeof(values_));
        values_[kParamPosition] = 0.5f;
        values_[kParamSize]     = 0.5f;
        values_[kParamDensity]  = 0.5f;
        values_[kParamTexture]  = 0.5f;
        values_[kParamDryWet]   = 0.5f;

        setupStyle();
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
        const float s = width / static_cast<float>(DISTRHO_UI_DEFAULT_WIDTH);

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(width, height));
        ImGui::GetIO().FontGlobalScale = s;

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
            if (active)
                ImGui::PopStyleColor(2);
        }

        ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.55f, 1.0f), "%s", kModeHints[mode]);
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // --------------------------------------------------- main knobs
        const float bigKnob = 78.0f * s;
        knob(kParamPosition, kKnobLabels[mode][0], 0.0f, 1.0f, bigKnob, "%.2f");
        ImGui::SameLine();
        knob(kParamSize, kKnobLabels[mode][1], 0.0f, 1.0f, bigKnob, "%.2f");
        ImGui::SameLine();
        knob(kParamPitch, kKnobLabels[mode][2], -24.0f, 24.0f, bigKnob, "%.2f st",
             /*withInput*/ true); // drag or ctrl+click the field for exact tuning
        ImGui::SameLine();
        knob(kParamDensity, kKnobLabels[mode][3], 0.0f, 1.0f, bigKnob, "%.2f");
        ImGui::SameLine();
        knob(kParamTexture, kKnobLabels[mode][4], 0.0f, 1.0f, bigKnob, "%.2f");

        ImGui::SameLine(0.0f, 30.0f * s);

        // ------------------------------------------------ buttons block
        // FREEZE and REVERSE latch, TRIG is momentary; all three share the
        // same button styling so the panel reads as one control family.
        const ImVec2 buttonSize(108.0f * s, 30.0f * s);
        ImGui::BeginGroup();
        const bool freeze = values_[kParamFreeze] > 0.5f;
        if (toggleButton("FREEZE", freeze, buttonSize))
            setBoolParameter(kParamFreeze, !freeze);

        const bool reverse = values_[kParamReverse] > 0.5f;
        if (toggleButton("REVERSE", reverse, buttonSize))
            setBoolParameter(kParamReverse, !reverse);

        ImGui::Button("TRIG", buttonSize);
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
        knob(kParamDryWet, kKnobLabels[mode][5], 0.0f, 1.0f, smallKnob, "%.2f");
        ImGui::SameLine();
        knob(kParamSpread, kKnobLabels[mode][6], 0.0f, 1.0f, smallKnob, "%.2f");
        ImGui::SameLine();
        knob(kParamFeedback, kKnobLabels[mode][7], 0.0f, 1.0f, smallKnob, "%.2f");
        ImGui::SameLine();
        knob(kParamReverb, kKnobLabels[mode][8], 0.0f, 1.0f, smallKnob, "%.2f");
        ImGui::SameLine(0.0f, 30.0f * s);
        if (mode != 6)
            ImGui::BeginDisabled();
        knob(kParamSlice, "Slice", 0.0f, 1.0f, smallKnob, "%.2f");
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
              bool withInput = false)
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

    static void setupStyle()
    {
        ImGuiStyle& st = ImGui::GetStyle();
        st.WindowRounding = 0.0f;
        st.FrameRounding = 4.0f;
        st.GrabRounding = 4.0f;
        st.WindowPadding = ImVec2(14, 10);
        st.ItemSpacing = ImVec2(10, 8);

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
