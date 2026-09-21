#pragma once
#include "Theme.h"

namespace blg::ui
{
/** Static artwork: brushed dark metal, recessed plates, perforated speaker grilles, screws.
    Rendered once into a cached image. */
class BackPanel : public juce::Component
{
public:
    BackPanel()
    {
        setOpaque (true);
        setBufferedToImage (true);
        setInterceptsMouseClicks (false, false);
    }

    void paint (juce::Graphics& g) override
    {
        const float W = (float) layout::baseWidth, H = (float) layout::baseHeight;

        // brushed metal
        g.setGradientFill (juce::ColourGradient (col::metalTop, 0.0f, 0.0f, col::metalBot, 0.0f, H, false));
        g.fillAll();

        juce::Random rng (7);
        for (int y = 0; y < layout::baseHeight; ++y)
        {
            g.setColour ((rng.nextBool() ? juce::Colours::white : juce::Colours::black)
                             .withAlpha (0.012f + 0.03f * rng.nextFloat()));
            g.drawHorizontalLine (y, 0.0f, W);
        }
        for (int i = 0; i < 500; ++i)
        {
            const float x = rng.nextFloat() * W, y = rng.nextFloat() * H;
            const float len = 40.0f + rng.nextFloat() * 260.0f;
            g.setColour (juce::Colours::white.withAlpha (0.02f + 0.04f * rng.nextFloat()));
            g.drawLine (x, y, x + len, y, 1.0f);
        }

        g.setColour (col::edgeLight.withAlpha (0.6f));
        g.drawRect (getLocalBounds(), 1);

        // plates
        drawPlate (g, layout::vintagePlate, "Vintage");
        drawPlate (g, layout::reverbPlate,  "Reverb");

        // speakers + preset screen
        drawSpeaker (g, layout::speakerLeft);
        drawSpeaker (g, layout::speakerRight);
        drawScreen  (g, layout::browserArea);

        // wordmark (left of the logo tile)
        g.setColour (col::text);
        g.setFont (makeFont (13.0f, true));
        g.drawText ("blackghanistan", juce::Rectangle<int> (680, 16, 184, 20), juce::Justification::centredRight);
        g.setColour (col::textDim);
        g.setFont (makeFont (11.0f));
        g.drawText ("vintage sound + reverb", juce::Rectangle<int> (680, 36, 184, 16), juce::Justification::centredRight);

        // screws
        const juce::Point<float> screws[] { { 14.0f, 14.0f }, { W - 14.0f, 14.0f },
                                            { 14.0f, H - 14.0f }, { W - 14.0f, H - 14.0f } };
        const float angles[] { 0.4f, 1.9f, 2.6f, 0.9f };

        for (int i = 0; i < 4; ++i)
            drawScrew (g, screws[i], angles[i]);
    }

private:
    static void drawPlate (juce::Graphics& g, juce::Rectangle<int> rc, const juce::String& title)
    {
        const auto r = rc.toFloat();

        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillRoundedRectangle (r.expanded (2.0f, 0.0f).withTrimmedTop (-0.5f).withTrimmedBottom (-4.0f), 12.0f);

        g.setGradientFill (juce::ColourGradient (col::plateTop, r.getX(), r.getY(),
                                                 col::plateBot, r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 10.0f);
        g.setColour (col::edgeDark);
        g.drawRoundedRectangle (r, 10.0f, 1.5f);
        g.setColour (col::edgeLight.withAlpha (0.55f));
        g.drawRoundedRectangle (r.reduced (2.0f), 8.0f, 1.0f);

        g.setColour (col::white);
        g.setFont (makeFont (17.0f, true));
        g.drawText (title, r.withHeight (34.0f).translated (0.0f, 6.0f), juce::Justification::centred);

        g.setColour (col::orange);
        g.fillRoundedRectangle (r.getCentreX() - 18.0f, r.getY() + 38.0f, 36.0f, 3.0f, 1.5f);
    }

    static void drawSpeaker (juce::Graphics& g, juce::Rectangle<int> rc)
    {
        const auto r = rc.toFloat();

        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillRoundedRectangle (r.expanded (2.0f, 0.0f).withTrimmedBottom (-4.0f), 12.0f);

        g.setGradientFill (juce::ColourGradient (col::grilleTop, r.getX(), r.getY(),
                                                 col::grilleBot, r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 10.0f);
        g.setColour (col::edgeDark);
        g.drawRoundedRectangle (r, 10.0f, 1.5f);

        juce::Graphics::ScopedSaveState ss (g);
        juce::Path clip;
        clip.addRoundedRectangle (r.reduced (6.0f), 6.0f);
        g.reduceClipRegion (clip);

        // staggered perforation; holes glow orange towards the centre (LED behind the mesh)
        const float pitch = 9.0f, rad = 2.4f;
        const auto  centre = r.getCentre();
        const float maxD = r.getWidth() * 0.5f;
        int row = 0;

        for (float y = r.getY() + 9.0f; y < r.getBottom(); y += pitch * 0.866f, ++row)
            for (float x = r.getX() + 9.0f + (row % 2 != 0 ? pitch * 0.5f : 0.0f); x < r.getRight(); x += pitch)
            {
                const float d    = juce::jlimit (0.0f, 1.0f, centre.getDistanceFrom ({ x, y }) / maxD);
                const float glow = std::pow (1.0f - d, 2.2f) * 0.85f;

                g.setColour (juce::Colour (0xff0b0c0e).interpolatedWith (col::orange, glow));
                g.fillEllipse (x - rad, y - rad, rad * 2.0f, rad * 2.0f);
                g.setColour (juce::Colours::white.withAlpha (0.10f));
                g.drawEllipse (x - rad, y - rad + 0.6f, rad * 2.0f, rad * 2.0f, 0.8f);
            }

        // cone rings
        g.setColour (juce::Colours::white.withAlpha (0.22f));
        g.drawEllipse (centre.x - 58.0f, centre.y - 58.0f, 116.0f, 116.0f, 1.5f);
        g.drawEllipse (centre.x - 14.0f, centre.y - 14.0f, 28.0f, 28.0f, 1.5f);
    }

    static void drawScreen (juce::Graphics& g, juce::Rectangle<int> rc)
    {
        const auto r = rc.toFloat();
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillRoundedRectangle (r.expanded (2.0f, 0.0f).withTrimmedBottom (-4.0f), 12.0f);
        g.setColour (col::screen);
        g.fillRoundedRectangle (r, 10.0f);
        g.setColour (col::edgeLight);
        g.drawRoundedRectangle (r.reduced (0.5f), 10.0f, 1.0f);
    }

    static void drawScrew (juce::Graphics& g, juce::Point<float> c, float angle)
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff9a9da5), c.x - 2.0f, c.y - 2.5f,
                                                 juce::Colour (0xff30323a), c.x + 4.0f, c.y + 4.0f, true));
        g.fillEllipse (c.x - 5.5f, c.y - 5.5f, 11.0f, 11.0f);
        g.setColour (col::edgeDark);
        g.drawEllipse (c.x - 5.5f, c.y - 5.5f, 11.0f, 11.0f, 1.0f);

        juce::Path slot;
        slot.startNewSubPath (-3.5f, 0.0f);
        slot.lineTo (3.5f, 0.0f);
        g.setColour (juce::Colour (0xff17181b));
        g.strokePath (slot, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded),
                      juce::AffineTransform::rotation (angle).translated (c.x, c.y));
    }
};
} // namespace blg::ui
