#pragma once
#include <JuceHeader.h>
#include <array>

namespace blg::dsp
{
/**
    Vintage section. Signal path per sample:

        Turbo (tanh drive + slight bias, auto-level, DC blocker)
     -> sample-rate reduction (sample & hold) + bit-depth quantiser
     -> reconstruction low-pass (only while the rate is reduced)
     -> Tilt (one-pole crossover, +/- tilt/2 dB around 650 Hz)

    No look-ahead and no oversampling -> zero reported latency.
    Nothing allocates in process(); all parameter changes are smoothed.
*/
class DegradeEngine
{
public:
    struct Settings
    {
        float sampleRateHz = 44100.0f;
        float turbo        = 0.0f;   // 0 .. 1
        float tiltDb       = 0.0f;   // -12 .. +12
        int   bits         = 16;     // 8 / 12 / 16 / 24 (24 = quantiser off)
    };

    void prepare (double newSampleRate, int maxBlockSize)
    {
        sampleRate = newSampleRate;

        juce::dsp::ProcessSpec spec { newSampleRate,
                                      (juce::uint32) juce::jmax (1, maxBlockSize),
                                      (juce::uint32) maxChannels };
        recon.prepare (spec);
        recon.setType (juce::dsp::StateVariableTPTFilterType::lowpass);

        tiltCoeff = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * tiltPivotHz / (float) newSampleRate);
        dcCoeff   = 1.0f - juce::MathConstants<float>::twoPi * 8.0f / (float) newSampleRate;

        turboSm.reset (newSampleRate, 0.03);
        gHighSm.reset (newSampleRate, 0.03);
        gLowSm.reset  (newSampleRate, 0.03);

        reset();
    }

    void reset()
    {
        recon.reset();
        for (auto& c : chan) c = ChannelState {};
        phase = 0.0f;
        needsSnap = true;
    }

    void process (juce::AudioBuffer<float>& buffer, const Settings& s) noexcept
    {
        const int numChannels = juce::jmin (buffer.getNumChannels(), maxChannels);
        const int numSamples  = buffer.getNumSamples();
        if (numChannels == 0 || numSamples == 0)
            return;

        const float turbo = juce::jlimit (0.0f, 1.0f, s.turbo);
        const float tilt  = juce::jlimit (-24.0f, 24.0f, s.tiltDb);

        const float ratio    = juce::jlimit (0.001f, 1.0f, s.sampleRateHz / (float) sampleRate);
        const bool  reduce   = ratio < 0.9999f;
        const float stepRate = reduce ? ratio : 1.0f;

        const bool  quantise  = s.bits < 24;
        const float levels    = (float) (1 << (juce::jlimit (2, 24, s.bits) - 1));
        const float invLevels = 1.0f / levels;

        if (reduce)
            recon.setCutoffFrequency (juce::jlimit (200.0f, 0.45f * (float) sampleRate, 0.45f * s.sampleRateHz));

        const float gHighTarget = juce::Decibels::decibelsToGain (+0.5f * tilt);
        const float gLowTarget  = juce::Decibels::decibelsToGain (-0.5f * tilt);

        if (needsSnap)
        {
            turboSm.setCurrentAndTargetValue (turbo);
            gHighSm.setCurrentAndTargetValue (gHighTarget);
            gLowSm.setCurrentAndTargetValue  (gLowTarget);
            needsSnap = false;
        }
        else
        {
            turboSm.setTargetValue (turbo);
            gHighSm.setTargetValue (gHighTarget);
            gLowSm.setTargetValue  (gLowTarget);
        }

        float* data[maxChannels] = {};
        for (int c = 0; c < numChannels; ++c)
            data[c] = buffer.getWritePointer (c);

        for (int i = 0; i < numSamples; ++i)
        {
            const float t     = turboSm.getNextValue();
            const float gHigh = gHighSm.getNextValue();
            const float gDiff = gLowSm.getNextValue() - gHigh;

            const bool  turboOn = t > 1.0e-4f;
            const float drive   = 1.0f + 14.0f * t;
            const float bias    = 0.12f * t;
            const float biasOff = turboOn ? std::tanh (bias) : 0.0f;
            const float makeup  = 1.0f / std::sqrt (drive);
            const float mix     = juce::jmin (1.0f, 8.0f * t);

            phase += stepRate;
            const bool tick = phase >= 1.0f;
            if (tick)
                phase -= 1.0f;

            for (int c = 0; c < numChannels; ++c)
            {
                auto& st = chan[(size_t) c];
                float x = data[c][i];

                // 1) Turbo
                if (turboOn)
                {
                    const float sat   = (std::tanh (drive * x + bias) - biasOff) * makeup;
                    const float dcOut = sat - st.dcX + dcCoeff * st.dcY;
                    st.dcX = sat;
                    st.dcY = dcOut;
                    x += mix * (dcOut - x);
                }
                else
                {
                    st.dcX = st.dcY = 0.0f;
                }

                // 2) sample-rate + bit-depth reduction
                if (tick)
                    st.hold = quantise ? std::round (x * levels) * invLevels : x;

                x = st.hold;

                if (reduce)
                    x = recon.processSample (c, x);

                // 3) Tilt
                st.tiltLp += tiltCoeff * (x - st.tiltLp);
                x = gHigh * x + gDiff * st.tiltLp;

                data[c][i] = x;
            }
        }
    }

private:
    static constexpr int   maxChannels  = 2;
    static constexpr float tiltPivotHz  = 650.0f;

    struct ChannelState
    {
        float hold   = 0.0f;
        float tiltLp = 0.0f;
        float dcX    = 0.0f;
        float dcY    = 0.0f;
    };

    double sampleRate = 44100.0;
    float  tiltCoeff  = 0.1f;
    float  dcCoeff    = 0.999f;
    float  phase      = 0.0f;
    bool   needsSnap  = true;

    std::array<ChannelState, (size_t) maxChannels> chan {};
    juce::dsp::StateVariableTPTFilter<float> recon;

    juce::SmoothedValue<float> turboSm, gHighSm, gLowSm;
};
} // namespace blg::dsp
