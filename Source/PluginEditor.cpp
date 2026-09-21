#include "PluginEditor.h"

using namespace blg::ui;

namespace
{
constexpr int minWidth = 640;
constexpr int maxWidth = 1920;
}

BlgAudioProcessorEditor::BlgAudioProcessorEditor (BlgAudioProcessor& p)
    : AudioProcessorEditor (&p), blgProcessor (p), panel (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (panel);

    setResizable (true, true);
    setResizeLimits (minWidth, (int) std::round (minWidth / layout::aspect),
                     maxWidth, (int) std::round (maxWidth / layout::aspect));
    getConstrainer()->setFixedAspectRatio (layout::aspect);

    const int savedWidth = juce::jlimit (minWidth, maxWidth,
                                         (int) blgProcessor.apvts.state.getProperty ("uiWidth", layout::baseWidth));
    setSize (savedWidth, (int) std::round (savedWidth / layout::aspect));
}

BlgAudioProcessorEditor::~BlgAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void BlgAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (col::metalBot);
}

void BlgAudioProcessorEditor::resized()
{
    const float scale = (float) getWidth() / (float) layout::baseWidth;

    panel.setBounds (0, 0, layout::baseWidth, layout::baseHeight);
    panel.setTransform (juce::AffineTransform::scale (scale));

    blgProcessor.apvts.state.setProperty ("uiWidth", getWidth(), nullptr);
}
