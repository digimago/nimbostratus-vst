// DPF plugin wrapping the Mutable Instruments Clouds granular processor.
//
// The DSP is the unmodified firmware code (eurorack/clouds/dsp), which runs
// at 32 kHz on int16 frames in blocks of 32 samples, exactly like the
// hardware. Host audio is resampled to/from 32 kHz with the speexdsp
// resampler, mirroring how the VCV Rack port drives the same code.

#include "DistrhoPlugin.hpp"

#include "supercell/dsp/granular_processor.h"

// Must match the defines resample.c is built with, so the renamed
// (RANDOM_PREFIX'd) symbols line up at link time.
#define OUTSIDE_SPEEX
#define RANDOM_PREFIX cloudsvst
#define EXPORT
#include "speex_resampler.h"

#include <cmath>
#include <cstring>
#include <vector>

#if defined(__SSE2__) || defined(_M_X64)
#include <immintrin.h>
#define CLOUDS_HAVE_SSE 1
#endif

START_NAMESPACE_DISTRHO

static constexpr double   kCloudsRate  = 32000.0;
static constexpr uint32_t kBlock       = clouds::kMaxBlockSize; // 32
static constexpr int      kSrcQuality  = 6;

#include "CloudsParams.h"

class CloudsPlugin : public Plugin
{
public:
    CloudsPlugin()
        : Plugin(kParamCount, 0, 0),
          srIn_(nullptr),
          srOut_(nullptr),
          prevGate_(false)
    {
        std::memset(largeBuffer_, 0, sizeof(largeBuffer_));
        std::memset(smallBuffer_, 0, sizeof(smallBuffer_));

        // The firmware relies on the processor being a zero-initialized
        // global: Init() leaves silence_, freeze_lp_ and parameters_
        // untouched. Zero the object first, as the VCV Rack port does.
        std::memset(static_cast<void*>(&processor_), 0, sizeof(processor_));

        values_[kParamPosition] = 0.5f;
        values_[kParamSize]     = 0.5f;
        values_[kParamPitch]    = 0.0f;
        values_[kParamDensity]  = 0.5f;
        values_[kParamTexture]  = 0.5f;
        values_[kParamDryWet]   = 0.5f;
        values_[kParamSpread]   = 0.0f;
        values_[kParamFeedback] = 0.0f;
        values_[kParamReverb]   = 0.0f;
        values_[kParamFreeze]   = 0.0f;
        values_[kParamTrigger]  = 0.0f;
        values_[kParamMode]     = 0.0f;
        values_[kParamQuality]  = 0.0f;
        values_[kParamReverse]  = 0.0f;
        values_[kParamSlice]    = 0.0f;

        processor_.Init(largeBuffer_, sizeof(largeBuffer_),
                        smallBuffer_, sizeof(smallBuffer_));
        processor_.set_num_channels(2);
        processor_.set_low_fidelity(false);
        processor_.set_playback_mode(clouds::PLAYBACK_MODE_GRANULAR);
        processor_.set_silence(false);
        processor_.set_bypass(false);
        processor_.Prepare();

        createResamplers();
    }

    ~CloudsPlugin() override
    {
        destroyResamplers();
    }

protected:
    const char* getLabel() const override       { return "Nubila"; }
    const char* getDescription() const override
    {
        return "Granular texture synthesizer. 1:1 port of the SuperParasites firmware "
               "(Clouds + Parasites + Kammerl): granular, pitch/stretch, looping delay, "
               "spectral, Oliverb, Resonestor, beat repeat and spectral cloud modes.";
    }
    const char* getMaker() const override       { return "DigiMago / Emilie Gillet (DSP)"; }
    const char* getHomePage() const override    { return "https://github.com/pichenettes/eurorack"; }
    const char* getLicense() const override     { return "MIT"; }
    uint32_t getVersion() const override        { return d_version(1, 2, 0); }
    int64_t getUniqueId() const override        { return d_cconst('d', 'g', 'C', 'l'); }

    void initParameter(uint32_t index, Parameter& parameter) override
    {
        parameter.hints = kParameterIsAutomatable;

        switch (index)
        {
        case kParamPosition:
            parameter.name = "Position";
            parameter.symbol = "position";
            parameter.ranges.def = 0.5f; parameter.ranges.min = 0.0f; parameter.ranges.max = 1.0f;
            break;
        case kParamSize:
            parameter.name = "Size";
            parameter.symbol = "size";
            parameter.ranges.def = 0.5f; parameter.ranges.min = 0.0f; parameter.ranges.max = 1.0f;
            break;
        case kParamPitch:
            parameter.name = "Pitch";
            parameter.symbol = "pitch";
            parameter.unit = "st";
            parameter.ranges.def = 0.0f; parameter.ranges.min = -24.0f; parameter.ranges.max = 24.0f;
            break;
        case kParamDensity:
            parameter.name = "Density";
            parameter.symbol = "density";
            parameter.ranges.def = 0.5f; parameter.ranges.min = 0.0f; parameter.ranges.max = 1.0f;
            break;
        case kParamTexture:
            parameter.name = "Texture";
            parameter.symbol = "texture";
            parameter.ranges.def = 0.5f; parameter.ranges.min = 0.0f; parameter.ranges.max = 1.0f;
            break;
        case kParamDryWet:
            parameter.name = "Dry/Wet";
            parameter.symbol = "dry_wet";
            parameter.ranges.def = 0.5f; parameter.ranges.min = 0.0f; parameter.ranges.max = 1.0f;
            break;
        case kParamSpread:
            parameter.name = "Stereo Spread";
            parameter.symbol = "stereo_spread";
            parameter.ranges.def = 0.0f; parameter.ranges.min = 0.0f; parameter.ranges.max = 1.0f;
            break;
        case kParamFeedback:
            parameter.name = "Feedback";
            parameter.symbol = "feedback";
            parameter.ranges.def = 0.0f; parameter.ranges.min = 0.0f; parameter.ranges.max = 1.0f;
            break;
        case kParamReverb:
            parameter.name = "Reverb";
            parameter.symbol = "reverb";
            parameter.ranges.def = 0.0f; parameter.ranges.min = 0.0f; parameter.ranges.max = 1.0f;
            break;
        case kParamFreeze:
            parameter.hints |= kParameterIsBoolean;
            parameter.name = "Freeze";
            parameter.symbol = "freeze";
            parameter.ranges.def = 0.0f; parameter.ranges.min = 0.0f; parameter.ranges.max = 1.0f;
            break;
        case kParamTrigger:
            parameter.hints |= kParameterIsBoolean;
            parameter.name = "Trigger";
            parameter.symbol = "trigger";
            parameter.ranges.def = 0.0f; parameter.ranges.min = 0.0f; parameter.ranges.max = 1.0f;
            break;
        case kParamMode:
        {
            parameter.hints |= kParameterIsInteger;
            parameter.name = "Mode";
            parameter.symbol = "playback_mode";
            parameter.ranges.def = 0.0f; parameter.ranges.min = 0.0f; parameter.ranges.max = 7.0f;
            parameter.enumValues.count = 8;
            parameter.enumValues.restrictedMode = true;
            ParameterEnumerationValue* const values = new ParameterEnumerationValue[8];
            values[0].value = 0.0f; values[0].label = "Granular";
            values[1].value = 1.0f; values[1].label = "Pitch/Stretch";
            values[2].value = 2.0f; values[2].label = "Looping Delay";
            values[3].value = 3.0f; values[3].label = "Spectral";
            values[4].value = 4.0f; values[4].label = "Oliverb";
            values[5].value = 5.0f; values[5].label = "Resonestor";
            values[6].value = 6.0f; values[6].label = "Beat Repeat";
            values[7].value = 7.0f; values[7].label = "Spectral Cloud";
            parameter.enumValues.values = values;
            break;
        }
        case kParamQuality:
        {
            parameter.hints |= kParameterIsInteger;
            parameter.name = "Quality";
            parameter.symbol = "quality";
            parameter.ranges.def = 0.0f; parameter.ranges.min = 0.0f; parameter.ranges.max = 3.0f;
            parameter.enumValues.count = 4;
            parameter.enumValues.restrictedMode = true;
            ParameterEnumerationValue* const values = new ParameterEnumerationValue[4];
            values[0].value = 0.0f; values[0].label = "16-bit Stereo (1s)";
            values[1].value = 1.0f; values[1].label = "16-bit Mono (2s)";
            values[2].value = 2.0f; values[2].label = "8-bit u-law Stereo (4s)";
            values[3].value = 3.0f; values[3].label = "8-bit u-law Mono (8s)";
            parameter.enumValues.values = values;
            break;
        }
        case kParamReverse:
            parameter.hints |= kParameterIsBoolean;
            parameter.name = "Reverse";
            parameter.symbol = "reverse";
            parameter.ranges.def = 0.0f; parameter.ranges.min = 0.0f; parameter.ranges.max = 1.0f;
            break;
        case kParamSlice:
            parameter.name = "Slice (Beat Repeat)";
            parameter.symbol = "slice";
            parameter.ranges.def = 0.0f; parameter.ranges.min = 0.0f; parameter.ranges.max = 1.0f;
            break;
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        return index < kParamCount ? values_[index] : 0.0f;
    }

    void setParameterValue(uint32_t index, float value) override
    {
        if (index < kParamCount)
            values_[index] = value;
    }

    void activate() override
    {
        fifo32In_.clear();
        fifo32Out_.clear();
        fifoHostOut_.clear();
        prevGate_ = false;

        if (srIn_ != nullptr)
            speex_resampler_reset_mem(srIn_);
        if (srOut_ != nullptr)
            speex_resampler_reset_mem(srOut_);

        // Prime the output queue so block-size mismatch between the host and
        // the 32-frame/32 kHz processing grid never underruns.
        const double ratio = getSampleRate() / kCloudsRate;
        const uint32_t prime = static_cast<uint32_t>(kBlock * ratio) + 16;
        fifoHostOut_.assign(prime * 2, 0.0f);

        uint32_t latency = prime;
        if (srIn_ != nullptr)
            latency += static_cast<uint32_t>(speex_resampler_get_input_latency(srIn_));
        if (srOut_ != nullptr)
            latency += static_cast<uint32_t>(speex_resampler_get_output_latency(srOut_));
        setLatency(latency);
    }

    void sampleRateChanged(double) override
    {
        createResamplers();
        activate();
    }

    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
#ifdef CLOUDS_HAVE_SSE
        const unsigned int oldCsr = _mm_getcsr();
        _mm_setcsr(oldCsr | 0x8040); // flush-to-zero + denormals-are-zero
#endif

        // Interleave host input.
        scratch_.resize(frames * 2);
        for (uint32_t i = 0; i < frames; ++i)
        {
            scratch_[i * 2 + 0] = inputs[0][i];
            scratch_[i * 2 + 1] = inputs[1][i];
        }

        // Host rate -> 32 kHz.
        {
            spx_uint32_t inLen = frames;
            spx_uint32_t outLen = static_cast<spx_uint32_t>(
                std::ceil(frames * kCloudsRate / getSampleRate())) + 16;
            const size_t base = fifo32In_.size();
            fifo32In_.resize(base + outLen * 2);
            speex_resampler_process_interleaved_float(
                srIn_, scratch_.data(), &inLen, fifo32In_.data() + base, &outLen);
            fifo32In_.resize(base + outLen * 2);
        }

        // Run the Clouds engine on complete 32-frame blocks.
        size_t consumed = 0;
        while (fifo32In_.size() - consumed >= kBlock * 2)
        {
            clouds::ShortFrame in[kBlock];
            clouds::ShortFrame out[kBlock];
            const float* src = fifo32In_.data() + consumed;
            for (uint32_t i = 0; i < kBlock; ++i)
            {
                in[i].l = toShort(src[i * 2 + 0]);
                in[i].r = toShort(src[i * 2 + 1]);
            }
            consumed += kBlock * 2;

            applyParameters();
            processor_.Process(in, out, kBlock);
            processor_.Prepare();

            const size_t base = fifo32Out_.size();
            fifo32Out_.resize(base + kBlock * 2);
            for (uint32_t i = 0; i < kBlock; ++i)
            {
                fifo32Out_[base + i * 2 + 0] = out[i].l / 32768.0f;
                fifo32Out_[base + i * 2 + 1] = out[i].r / 32768.0f;
            }
        }
        if (consumed > 0)
            fifo32In_.erase(fifo32In_.begin(), fifo32In_.begin() + consumed);

        // 32 kHz -> host rate.
        if (!fifo32Out_.empty())
        {
            spx_uint32_t inLen = static_cast<spx_uint32_t>(fifo32Out_.size() / 2);
            spx_uint32_t outLen = static_cast<spx_uint32_t>(
                std::ceil(inLen * getSampleRate() / kCloudsRate)) + 16;
            const size_t base = fifoHostOut_.size();
            fifoHostOut_.resize(base + outLen * 2);
            speex_resampler_process_interleaved_float(
                srOut_, fifo32Out_.data(), &inLen, fifoHostOut_.data() + base, &outLen);
            fifoHostOut_.resize(base + outLen * 2);
            fifo32Out_.erase(fifo32Out_.begin(), fifo32Out_.begin() + inLen * 2);
        }

        // Emit to host; pad with silence if the pipeline is still filling.
        const uint32_t avail = static_cast<uint32_t>(fifoHostOut_.size() / 2);
        const uint32_t n = avail < frames ? avail : frames;
        const uint32_t missing = frames - n;
        for (uint32_t i = 0; i < missing; ++i)
        {
            outputs[0][i] = 0.0f;
            outputs[1][i] = 0.0f;
        }
        for (uint32_t i = 0; i < n; ++i)
        {
            outputs[0][missing + i] = fifoHostOut_[i * 2 + 0];
            outputs[1][missing + i] = fifoHostOut_[i * 2 + 1];
        }
        fifoHostOut_.erase(fifoHostOut_.begin(), fifoHostOut_.begin() + n * 2);

#ifdef CLOUDS_HAVE_SSE
        _mm_setcsr(oldCsr);
#endif
    }

private:
    static int16_t toShort(float x)
    {
        float y = x * 32767.0f;
        if (y > 32767.0f) y = 32767.0f;
        if (y < -32768.0f) y = -32768.0f;
        return static_cast<int16_t>(y);
    }

    void applyParameters()
    {
        processor_.set_playback_mode(static_cast<clouds::PlaybackMode>(
            static_cast<int>(values_[kParamMode] + 0.5f)));
        processor_.set_quality(static_cast<int32_t>(values_[kParamQuality] + 0.5f));

        clouds::Parameters* const p = processor_.mutable_parameters();
        p->position      = values_[kParamPosition];
        p->size          = values_[kParamSize];
        p->pitch         = values_[kParamPitch];
        p->density       = values_[kParamDensity];
        p->texture       = values_[kParamTexture];
        p->dry_wet       = values_[kParamDryWet];
        p->stereo_spread = values_[kParamSpread];
        p->feedback      = values_[kParamFeedback];
        p->reverb        = values_[kParamReverb];
        p->freeze        = values_[kParamFreeze] > 0.5f;
        p->granular.reverse = values_[kParamReverse] > 0.5f;

        // Beat Repeat (Kammerl) repurposes the existing knobs the same way
        // the hardware cv_scaler does; slice selection is CV-only there, so
        // it gets a dedicated parameter here.
        p->kammerl.slice_selection  = values_[kParamSlice];
        p->kammerl.slice_modulation = values_[kParamTexture];
        p->kammerl.size_modulation  = values_[kParamDensity];
        p->kammerl.probability      = values_[kParamDryWet];
        p->kammerl.clock_divider    = values_[kParamSpread];
        p->kammerl.pitch_mode       = values_[kParamFeedback];
        p->kammerl.distortion       = values_[kParamReverb];
        float kammerlPitch = values_[kParamPitch] / 48.0f + 0.5f;
        if (kammerlPitch < 0.0f) kammerlPitch = 0.0f;
        if (kammerlPitch > 1.0f) kammerlPitch = 1.0f;
        p->kammerl.pitch = kammerlPitch;

        const bool gate = values_[kParamTrigger] > 0.5f;
        p->capture = gate && !prevGate_;
        p->gate    = gate;
        prevGate_  = gate;
    }

    void createResamplers()
    {
        destroyResamplers();
        int err = 0;
        srIn_ = speex_resampler_init(
            2, static_cast<spx_uint32_t>(getSampleRate()),
            static_cast<spx_uint32_t>(kCloudsRate), kSrcQuality, &err);
        srOut_ = speex_resampler_init(
            2, static_cast<spx_uint32_t>(kCloudsRate),
            static_cast<spx_uint32_t>(getSampleRate()), kSrcQuality, &err);
    }

    void destroyResamplers()
    {
        if (srIn_ != nullptr)  { speex_resampler_destroy(srIn_);  srIn_ = nullptr; }
        if (srOut_ != nullptr) { speex_resampler_destroy(srOut_); srOut_ = nullptr; }
    }

    // Same memory layout the firmware gives the processor: 116 kB SRAM block
    // plus the 64 kB CCM block.
    uint8_t largeBuffer_[118784];
    uint8_t smallBuffer_[65536 - 128];

    clouds::GranularProcessor processor_;

    SpeexResamplerState* srIn_;
    SpeexResamplerState* srOut_;

    std::vector<float> scratch_;
    std::vector<float> fifo32In_;
    std::vector<float> fifo32Out_;
    std::vector<float> fifoHostOut_;

    float values_[kParamCount];
    bool prevGate_;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CloudsPlugin)
};

Plugin* createPlugin()
{
    return new CloudsPlugin();
}

END_NAMESPACE_DISTRHO
