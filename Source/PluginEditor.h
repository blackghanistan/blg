#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/Theme.h"
#include "UI/BackPanel.h"
#include "UI/Widgets.h"
#include "UI/PresetUI.h"

namespace blg::ui
{
/** Everything lives on a fixed 960 x 600 canvas; the editor scales it with a transform. */
class MainPanel : public juce::Component
{
public:
    explicit MainPanel (BlgAudioProcessor& p)
        : sampleRate (p.apvts, ids::sampleRate, "Sample rate", true),
          turbo      (p.apvts, ids::turbo,      "Turbo"),
          tilt       (p.apvts, ids::tilt,       "Tilt"),
          revDry     (p.apvts, ids::revDry,     "Reverb Dry"),
          revSize    (p.apvts, ids::revSize,    "Reverb size"),
          stereo     (p.apvts, ids::stereo,     "Stereo"),
          revWidth   (p.apvts, ids::revWidth,   "Width"),
          revDepth   (p.apvts, ids::revDepth,   "Depth"),
          bits       (p.apvts),
          display    (p.apvts),
          bar        (p.presets),
          browser    (p.presets)
    {
        addAndMakeVisible (back);
        addAndMakeVisible (logo);
        addAndMakeVisible (bar);
        addAndMakeVisible (bits);
        addAndMakeVisible (display);
        addAndMakeVisible (sampleRate);
        addAndMakeVisible (turbo);
        addAndMakeVisible (tilt);
        addAndMakeVisible (revDry);
        addAndMakeVisible (revSize);
        addAndMakeVisible (stereo);
        addAndMakeVisible (revWidth);
        addAndMakeVisible (revDepth);
        addAndMakeVisible (browser);

        setSize (layout::baseWidth, layout::baseHeight);
    }

    void resized() override
    {
        back.setBounds (getLocalBounds());
        logo.setBounds (880, 10, 56, 56);          // small square, top-right corner
        bar.setBounds (24, 14, 610, 48);

        // Vintage plate
        bits.setBounds (44, 128, 146, 48);
        display.setBounds (438, 128, 142, 90);
        sampleRate.setBounds (200, 130, 228, 266);
        turbo.setBounds (52, 246, 120, 156);
        tilt.setBounds (456, 246, 120, 156);

        // Reverb plate
        revDry.setBounds  (632, 132, 96, 140);
        revSize.setBounds (730, 132, 96, 140);
        stereo.setBounds  (828, 132, 96, 140);
        revWidth.setBounds (675, 280, 96, 140);
        revDepth.setBounds (785, 280, 96, 140);

        browser.setBounds (layout::browserArea);
    }

private:
    BackPanel back;
    Logo logo;
    Knob sampleRate, turbo, tilt, revDry, revSize, stereo, revWidth, revDepth;
    BitDepthSelector bits;
    DegradeDisplay display;
    PresetBar bar;
    PresetBrowser browser;
};
} // namespace blg::ui

class BlgAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit BlgAudioProcessorEditor (BlgAudioProcessor&);
    ~BlgAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    BlgAudioProcessor& blgProcessor;
    blg::ui::BlgLookAndFeel lnf;
    blg::ui::MainPanel panel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BlgAudioProcessorEditor)
};
