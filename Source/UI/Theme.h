#pragma once
#include <JuceHeader.h>

namespace blg::ui
{
namespace col
{
    inline const juce::Colour metalTop  { 0xff3a3c41 };
    inline const juce::Colour metalBot  { 0xff1f2024 };
    inline const juce::Colour plateTop  { 0xff2d2f34 };
    inline const juce::Colour plateBot  { 0xff25272b };
    inline const juce::Colour grilleTop { 0xff45484f };
    inline const juce::Colour grilleBot { 0xff2b2d32 };
    inline const juce::Colour edgeLight { 0xff4a4d54 };
    inline const juce::Colour edgeDark  { 0xff121315 };
    inline const juce::Colour screen    { 0xff141518 };
    inline const juce::Colour white     { 0xfff5f2ec };
    inline const juce::Colour text      { 0xffc9cbd0 };
    inline const juce::Colour textDim   { 0xff858990 };
    inline const juce::Colour orange    { 0xffff6a13 };
    inline const juce::Colour orangeHi  { 0xffffa15c };
    inline const juce::Colour orangeLo  { 0xffb54400 };
}

// The whole UI is designed on a fixed 960 x 600 canvas and scaled with a component transform.
namespace layout
{
    inline constexpr int    baseWidth  = 960;
    inline constexpr int    baseHeight = 600;
    inline constexpr double aspect     = 960.0 / 600.0;

    inline const juce::Rectangle<int> vintagePlate { 24, 84, 580, 344 };
    inline const juce::Rectangle<int> reverbPlate  { 620, 84, 316, 344 };
    inline const juce::Rectangle<int> speakerLeft  { 24, 444, 248, 140 };
    inline const juce::Rectangle<int> speakerRight { 688, 444, 248, 140 };
    inline const juce::Rectangle<int> browserArea  { 288, 444, 384, 140 };
}

inline juce::Font makeFont (float height, bool bold = false)
{
    return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain));
}

class BlgLookAndFeel : public juce::LookAndFeel_V4
{
public:
    BlgLookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, col::metalBot);
        setColour (juce::Label::textColourId, col::text);
        setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
        setColour (juce::ScrollBar::thumbColourId, col::orange.withAlpha (0.75f));
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider&) override
    {
        const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (2.0f);
        const float diameter = juce::jmin (bounds.getWidth(), bounds.getHeight());
        const auto area = juce::Rectangle<float> (diameter, diameter).withCentre (bounds.getCentre());
        const float R = diameter * 0.5f;
        const auto  c = area.getCentre();
        const float angle = startAngle + sliderPos * (endAngle - startAngle);

        // value ring
        const float ringR = R - 2.5f;
        const float ringW = juce::jmax (3.0f, R * 0.07f);
        const juce::PathStrokeType stroke (ringW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
        const juce::PathStrokeType glow   (ringW * 2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

        juce::Path track;
        track.addCentredArc (c.x, c.y, ringR, ringR, 0.0f, startAngle, endAngle, true);
        g.setColour (col::edgeDark);
        g.strokePath (track, stroke);

        juce::Path value;
        value.addCentredArc (c.x, c.y, ringR, ringR, 0.0f, startAngle, angle, true);
        g.setColour (col::orange.withAlpha (0.25f));
        g.strokePath (value, glow);
        g.setColour (col::orange);
        g.strokePath (value, stroke);

        // white knob body
        const auto body = area.reduced (ringW + 5.0f);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillEllipse (body.translated (0.0f, R * 0.05f).expanded (1.5f));

        g.setGradientFill (juce::ColourGradient (juce::Colours::white,
                                                 body.getX() + body.getWidth() * 0.3f,
                                                 body.getY() + body.getHeight() * 0.25f,
                                                 juce::Colour (0xffcfcac1),
                                                 body.getRight(), body.getBottom(), true));
        g.fillEllipse (body);
        g.setColour (col::edgeDark);
        g.drawEllipse (body, juce::jmax (1.5f, R * 0.03f));

        // pointer
        const float rb = body.getWidth() * 0.5f;
        juce::Path pointer;
        pointer.startNewSubPath (0.0f, -rb * 0.30f);
        pointer.lineTo (0.0f, -rb * 0.86f);
        g.setColour (col::orange);
        g.strokePath (pointer,
                      juce::PathStrokeType (juce::jmax (2.5f, R * 0.055f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded),
                      juce::AffineTransform::rotation (angle).translated (c.x, c.y));
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&,
                               bool highlighted, bool down) override
    {
        const auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (down ? col::orangeLo : (highlighted ? juce::Colour (0xff3c3f46) : juce::Colour (0xff2f3238)));
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (down ? col::orange : col::edgeLight);
        g.drawRoundedRectangle (r, 6.0f, 1.0f);
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool) override
    {
        g.setFont (makeFont (13.0f, true));
        g.setColour (col::white);
        g.drawFittedText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred, 1);
    }
};
} // namespace blg::ui
