#pragma once
#include <JuceHeader.h>
#include <vector>

namespace blg::dsp
{
/**
    Reverb section.

      size  -> room size of the reverb core
      width -> stereo spread of the reverb tail (juce::Reverb width)
      depth -> perceived distance: pre-delay (0..80 ms) and darker damping
      dry   -> equal-power crossfade, 1 = dry only, 0 = reverb only

    All buffers are allocated in prepare(); process() is allocation-free.
*/
class ReverbEngine
{
public:
    struct Settings
    {
        float dry   = 1.0f;
        float size  = 0.5f;
        float width = 1.0f;
        float depth = 0.3f;
    };

    void prepare (double newSampleRate, int maxBlockSize)
    {
        sampleRate = newSampleRate;
        reverb.setSampleRate (newSampleRate);

        maxDelay = (int) std::ceil (maxPreDelaySeconds * newSampleRate) + 4;
        for (auto& d : delayLine)
            d.assign ((size_t) maxDelay, 0.0f);

        for (auto& w : wet)
            w.assign ((size_t) juce::jmax (1, maxBlockSize), 0.0f);

        preDelaySm.reset (newSampleRate, 0.08);
        drySm.reset (newSampleRate, 0.03);

        reset();
    }

    void reset()
    {
        reverb.reset();
        for (auto& d : delayLine)
            std::fill (d.begin(), d.end(), 0.0f);
        writePos  = 0;
        needsSnap = true;
    }

    void process (juce::AudioBuffer<float>& buffer, const Settings& s) noexcept
    {
        const int numChannels = buffer.getNumChannels();
        const int total       = buffer.getNumSamples();
        if (numChannels == 0 || total == 0 || maxDelay == 0 || wet[0].empty())
            return;

        const float dry   = juce::jlimit (0.0f, 1.0f, s.dry);
        const float size  = juce::jlimit (0.0f, 1.0f, s.size);
        const float width = juce::jlimit (0.0f, 1.0f, s.width);
        const float depth = juce::jlimit (0.0f, 1.0f, s.depth);

        juce::Reverb::Parameters p;
        p.roomSize   = 0.25f + 0.73f * size;
        p.damping    = 0.25f + 0.60f * depth;
        p.wetLevel   = 0.40f;   // unity-ish level; the mix is done below
        p.dryLevel   = 0.0f;
        p.width      = width;
        p.freezeMode = 0.0f;
        reverb.setParameters (p);

        const float preDelayTarget = depth * maxPreDelaySeconds * (float) sampleRate;

        if (needsSnap)
        {
            preDelaySm.setCurrentAndTargetValue (preDelayTarget);
            drySm.setCurrentAndTargetValue (dry);
            needsSnap = false;
        }
        else
        {
            preDelaySm.setTargetValue (preDelayTarget);
            drySm.setTargetValue (dry);
        }

        float* l = buffer.getWritePointer (0);
        float* r = numChannels > 1 ? buffer.getWritePointer (1) : nullptr;

        auto& dl = delayLine[0];
        auto& dr = delayLine[1];

        int pos = 0;
        while (pos < total)
        {
            const int n = juce::jmin (total - pos, (int) wet[0].size());
            float* wl = wet[0].data();
            float* wr = wet[1].data();

            // pre-delay feeds only the wet path
            for (int i = 0; i < n; ++i)
            {
                const float d   = preDelaySm.getNextValue();
                const float inL = l[pos + i];
                const float inR = r != nullptr ? r[pos + i] : inL;

                dl[(size_t) writePos] = inL;
                dr[(size_t) writePos] = inR;

                wl[i] = readInterpolated (dl, writePos, d);
                wr[i] = readInterpolated (dr, writePos, d);

                if (++writePos >= maxDelay)
                    writePos = 0;
            }

            reverb.processStereo (wl, wr, n);

            for (int i = 0; i < n; ++i)
            {
                const float dv      = drySm.getNextValue();
                const float dryGain = std::sqrt (dv);
                const float wetGain = std::sqrt (juce::jmax (0.0f, 1.0f - dv));

                if (r != nullptr)
                {
                    l[pos + i] = l[pos + i] * dryGain + wl[i] * wetGain;
                    r[pos + i] = r[pos + i] * dryGain + wr[i] * wetGain;
                }
                else
                {
                    l[pos + i] = l[pos + i] * dryGain + 0.5f * (wl[i] + wr[i]) * wetGain;
                }
            }

            pos += n;
        }
    }

private:
    static constexpr float maxPreDelaySeconds = 0.08f;

    static float readInterpolated (const std::vector<float>& buf, int writeIdx, float delay) noexcept
    {
        const int size = (int) buf.size();
        float rp = (float) writeIdx - delay;
        if (rp < 0.0f)
            rp += (float) size;

        const int   i0   = juce::jlimit (0, size - 1, (int) rp);
        const int   i1   = i0 + 1 >= size ? 0 : i0 + 1;
        const float frac = rp - (float) i0;
        return buf[(size_t) i0] + frac * (buf[(size_t) i1] - buf[(size_t) i0]);
    }

    double sampleRate = 44100.0;
    int    maxDelay   = 0;
    int    writePos   = 0;
    bool   needsSnap  = true;

    juce::Reverb reverb;
    std::vector<float> delayLine[2];
    std::vector<float> wet[2];

    juce::SmoothedValue<float> preDelaySm, drySm;
};
} // namespace blg::dsp
