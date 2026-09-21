#pragma once
#include "Theme.h"
#include "../Parameters.h"

namespace blg::ui
{
//==============================================================================
/** White rotary knob with value text above and name below (like the reference plugin). */
class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID,
          const juce::String& title, bool isLarge = false)
        : param (*apvts.getParameter (paramID)), large (isLarge)
    {
        slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                    juce::MathConstants<float>::pi * 2.75f, true);
        slider.setMouseDragSensitivity (220);
        slider.textFromValueFunction = [this] (double v)
        {
            return param.getText (param.convertTo0to1 ((float) v), 0);
        };
        addAndMakeVisible (slider);

        nameLabel.setText (title, juce::dontSendNotification);
        nameLabel.setJustificationType (juce::Justification::centred);
        nameLabel.setFont (makeFont (large ? 18.0f : 15.0f, true));
        nameLabel.setColour (juce::Label::textColourId, col::white);
        nameLabel.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (nameLabel);

        valueLabel.setJustificationType (juce::Justification::centred);
        valueLabel.setFont (makeFont (13.0f));
        valueLabel.setColour (juce::Label::textColourId, col::textDim);
        valueLabel.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (valueLabel);

        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, paramID, slider);
        slider.setDoubleClickReturnValue (true, param.convertFrom0to1 (param.getDefaultValue()));
        slider.onValueChange = [this] { updateValueText(); };
        updateValueText();
    }

    void resized() override
    {
        auto r = getLocalBounds();
        nameLabel.setBounds  (r.removeFromBottom (large ? 26 : 22));
        valueLabel.setBounds (r.removeFromBottom (18));
        slider.setBounds (r);
    }

private:
    void updateValueText()
    {
        valueLabel.setText (slider.getTextFromValue (slider.getValue()), juce::dontSendNotification);
    }

    juce::RangedAudioParameter& param;
    bool large;
    juce::Slider slider;
    juce::Label nameLabel, valueLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

//==============================================================================
/** Segmented selector for the bit-depth choice parameter (8 / 12 / 16 / 24). */
class BitDepthSelector : public juce::Component
{
public:
    explicit BitDepthSelector (juce::AudioProcessorValueTreeState& apvts)
        : attachment (*apvts.getParameter (ids::bits),
                      [this] (float v) { selected = juce::roundToInt (v); repaint(); })
    {
        attachment.sendInitialUpdate();
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (col::textDim);
        g.setFont (makeFont (12.0f, true));
        g.drawText ("Bit depth", r.removeFromTop (16.0f), juce::Justification::centredLeft);
        r.removeFromTop (2.0f);

        g.setColour (col::screen);
        g.fillRoundedRectangle (r, 6.0f);

        const float w = r.getWidth() / 4.0f;
        for (int i = 0; i < 4; ++i)
        {
            const auto seg = juce::Rectangle<float> (r.getX() + w * (float) i, r.getY(), w, r.getHeight()).reduced (2.0f);
            const bool on = i == selected;
            if (on)
            {
                g.setColour (col::orange);
                g.fillRoundedRectangle (seg, 4.0f);
            }
            g.setColour (on ? col::screen : col::text);
            g.setFont (makeFont (13.0f, true));
            g.drawText (juce::String (bitDepthTable[i]), seg, juce::Justification::centred);
        }

        g.setColour (col::edgeLight);
        g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const int idx = juce::jlimit (0, 3, (int) (e.position.x / ((float) getWidth() / 4.0f)));
        attachment.setValueAsCompleteGesture ((float) idx);
    }

private:
    int selected = 2;
    juce::ParameterAttachment attachment;
};

//==============================================================================
/** Small scope that previews what Turbo, sample rate and bit depth do to a sine wave.
    UI-thread only, computed from parameter values (never touches audio). */
class DegradeDisplay : public juce::Component, private juce::Timer
{
public:
    explicit DegradeDisplay (juce::AudioProcessorValueTreeState& apvts)
        : pSampleRate (apvts.getRawParameterValue (ids::sampleRate)),
          pBits       (apvts.getRawParameterValue (ids::bits)),
          pTurbo      (apvts.getRawParameterValue (ids::turbo))
    {
        startTimerHz (24);
    }

    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat();
        g.setColour (col::screen);
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (col::edgeLight);
        g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.0f);

        const auto a = r.reduced (8.0f, 10.0f);

        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.drawHorizontalLine ((int) a.getCentreY(), a.getX(), a.getRight());
        for (int i = 1; i < 3; ++i)
            g.drawVerticalLine ((int) (a.getX() + a.getWidth() * (float) i / 3.0f), a.getY(), a.getBottom());

        const float sr    = pSampleRate->load();
        const float turbo = pTurbo->load();
        const int   bits  = bitDepthTable[juce::jlimit (0, 3, juce::roundToInt (pBits->load()))];

        constexpr int   N = 160;
        constexpr float period = 40.0f;
        const float ratio = juce::jmin (1.0f, sr / 48000.0f);
        const float drive = 1.0f + 14.0f * turbo;
        const float mix   = juce::jmin (1.0f, 8.0f * turbo);
        const float bias  = 0.12f * turbo;
        // bit steps are exaggerated (bits / 2) so they stay visible at this size
        const float levels = bits >= 24 ? 0.0f : (float) (1 << juce::jmax (1, bits / 2 - 1));

        auto yOf = [&] (float v) { return a.getCentreY() - v * a.getHeight() * 0.45f; };

        juce::Path original, processed;
        float phase = 0.0f, hold = 0.0f, prevY = 0.0f;

        for (int i = 0; i < N; ++i)
        {
            const float x  = a.getX() + a.getWidth() * (float) i / (float) (N - 1);
            const float in = std::sin (juce::MathConstants<float>::twoPi * (float) i / period);

            float v = in;
            if (turbo > 0.0f)
            {
                const float sat = (std::tanh (drive * in + bias) - std::tanh (bias)) / std::sqrt (drive);
                v = in + mix * (sat - in);
            }

            phase += ratio;
            if (phase >= 1.0f)
            {
                phase -= 1.0f;
                hold = levels > 0.0f ? std::round (v * levels) / levels : v;
            }

            const float y = yOf (hold);
            if (i == 0)
            {
                original.startNewSubPath (x, yOf (in));
                processed.startNewSubPath (x, y);
            }
            else
            {
                original.lineTo (x, yOf (in));
                if (y != prevY)
                    processed.lineTo (x, prevY);
                processed.lineTo (x, y);
            }
            prevY = y;
        }

        g.setColour (juce::Colours::white.withAlpha (0.28f));
        g.strokePath (original, juce::PathStrokeType (1.0f));
        g.setColour (col::orange);
        g.strokePath (processed, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved));
    }

private:
    void timerCallback() override
    {
        const float s = pSampleRate->load(), b = pBits->load(), t = pTurbo->load();
        if (s != lastS || b != lastB || t != lastT)
        {
            lastS = s; lastB = b; lastT = t;
            repaint();
        }
    }

    std::atomic<float>* pSampleRate;
    std::atomic<float>* pBits;
    std::atomic<float>* pTurbo;
    float lastS = -1.0f, lastB = -1.0f, lastT = -1.0f;
};

//==============================================================================
/** "blg" logo tile: orange square, geometric letters whose bowls are tiny knobs. */
class Logo : public juce::Component
{
public:
    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat();
        const float s = juce::jmin (r.getWidth(), r.getHeight());
        const float corner = s * 0.16f;

        g.setGradientFill (juce::ColourGradient (col::orangeHi, r.getX(), r.getY(),
                                                 juce::Colour (0xfff05c00), r.getRight(), r.getBottom(), false));
        g.fillRoundedRectangle (r, corner);
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.drawRoundedRectangle (r.reduced (0.5f), corner, 1.0f);

        // glyphs are drawn in a 64 x 64 unit box
        const auto tf = juce::AffineTransform::scale (s / 64.0f).translated (r.getX(), r.getY());

        juce::Path ink;
        ink.startNewSubPath (8.5f, 12.0f);   ink.lineTo (8.5f, 46.5f);          // b: stem
        ink.addEllipse (8.5f, 29.5f, 17.0f, 17.0f);                            //    bowl
        ink.startNewSubPath (32.0f, 12.0f);  ink.lineTo (32.0f, 46.5f);         // l
        ink.addEllipse (38.5f, 29.5f, 17.0f, 17.0f);                           // g: bowl
        ink.startNewSubPath (55.5f, 38.0f);  ink.lineTo (55.5f, 49.0f);         //    descender
        ink.quadraticTo (55.5f, 57.0f, 47.0f, 57.0f);
        ink.quadraticTo (42.5f, 57.0f, 40.5f, 54.0f);

        g.setColour (juce::Colour (0xff1b1c1f));
        g.strokePath (ink, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded), tf);

        juce::Path ticks;                                                       // knob pointers inside the bowls
        ticks.startNewSubPath (17.0f, 38.0f); ticks.lineTo (19.8f, 35.2f);
        ticks.startNewSubPath (47.0f, 38.0f); ticks.lineTo (44.2f, 35.2f);
        g.setColour (juce::Colours::white);
        g.strokePath (ticks, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded), tf);
    }
};
} // namespace blg::ui
