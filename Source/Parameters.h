#pragma once
#include <JuceHeader.h>

namespace blg
{
namespace ids
{
    inline constexpr const char* sampleRate = "sampleRate";  // Hz, 1 kHz .. 48 kHz
    inline constexpr const char* bits       = "bits";        // choice: 8 / 12 / 16 / 24
    inline constexpr const char* turbo      = "turbo";       // 0 .. 1
    inline constexpr const char* tilt       = "tilt";        // -12 .. +12 dB
    inline constexpr const char* revDry     = "revDry";      // 0 .. 1  (1 = fully dry)
    inline constexpr const char* revSize    = "revSize";     // 0 .. 1
    inline constexpr const char* stereo     = "stereo";      // 0 .. 2  (output M/S width, 1 = neutral)
    inline constexpr const char* revWidth   = "revWidth";    // 0 .. 1  (stereo spread of the reverb tail)
    inline constexpr const char* revDepth   = "revDepth";    // 0 .. 1  (pre-delay + damping = "distance")
}

inline constexpr int bitDepthTable[] = { 8, 12, 16, 24 };

inline juce::String percentText (float v, float scale = 100.0f)
{
    return juce::String (juce::roundToInt (v * scale)) + " %";
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;
    using Float = AudioParameterFloat;
    using Attr  = AudioParameterFloatAttributes;

    AudioProcessorValueTreeState::ParameterLayout layout;

    // Sample rate: 1 kHz .. 48 kHz, skewed so the low end gets more knob travel
    NormalisableRange<float> srRange (1000.0f, 48000.0f, 1.0f);
    srRange.setSkewForCentre (8000.0f);

    layout.add (std::make_unique<Float> (
        ParameterID { ids::sampleRate, 1 }, "Sample rate", srRange, 22050.0f,
        Attr()
            .withStringFromValueFunction ([] (float v, int)
            {
                return v >= 1000.0f ? String (v / 1000.0f, 1) + " kHz"
                                    : String (roundToInt (v)) + " Hz";
            })
            .withValueFromStringFunction ([] (const String& t)
            {
                const float v = t.getFloatValue();
                return t.containsIgnoreCase ("k") ? v * 1000.0f : v;
            })));

    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { ids::bits, 1 }, "Bit depth",
        StringArray { "8 bit", "12 bit", "16 bit", "24 bit" }, 2));

    auto percent = [] { return Attr()
        .withStringFromValueFunction ([] (float v, int) { return percentText (v); })
        .withValueFromStringFunction ([] (const String& t) { return t.getFloatValue() / 100.0f; }); };

    layout.add (std::make_unique<Float> (ParameterID { ids::turbo, 1 }, "Turbo",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.2f, percent()));

    layout.add (std::make_unique<Float> (ParameterID { ids::tilt, 1 }, "Tilt",
        NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f,
        Attr().withStringFromValueFunction ([] (float v, int)
        {
            return (v > 0.05f ? "+" : "") + String (v, 1) + " dB";
        })));

    layout.add (std::make_unique<Float> (ParameterID { ids::revDry, 1 }, "Reverb Dry",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.8f, percent()));

    layout.add (std::make_unique<Float> (ParameterID { ids::revSize, 1 }, "Reverb size",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f, percent()));

    layout.add (std::make_unique<Float> (ParameterID { ids::stereo, 1 }, "Stereo",
        NormalisableRange<float> (0.0f, 2.0f, 0.005f), 1.0f, percent()));

    layout.add (std::make_unique<Float> (ParameterID { ids::revWidth, 1 }, "Reverb width",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f, percent()));

    layout.add (std::make_unique<Float> (ParameterID { ids::revDepth, 1 }, "Reverb depth",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f, percent()));

    return layout;
}
} // namespace blg
