#pragma once
#include "PluginProcessor.h"

namespace mello
{
    // 0 until drive passes ~15%, then rises to 1. Drives the green -> red shift.
    inline float tint (float drive)
    {
        return std::pow (juce::jlimit (0.0f, 1.0f, (drive - 0.15f) / 0.85f), 0.85f);
    }

    inline juce::Colour accent (float t)
    {
        return juce::Colour (25, 224, 79).interpolatedWith (juce::Colour (255, 42, 26), t);
    }

   #if JUCE_MAJOR_VERSION >= 8
    inline juce::Font display (float h) { return juce::Font (juce::FontOptions (h, juce::Font::bold)); }
    inline juce::Font mono (float h)    { return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), h, juce::Font::plain)); }
   #else
    inline juce::Font display (float h) { return juce::Font (h, juce::Font::bold); }
    inline juce::Font mono (float h)    { return juce::Font (juce::Font::getDefaultMonospacedFontName(), h, juce::Font::plain); }
   #endif
}

//==============================================================================
/** Draws both knobs. The DRIVE knob is the face: it tilts with the value. */
class MelluLookAndFeel : public juce::LookAndFeel_V4
{
public:
    explicit MelluLookAndFeel (const float& driveLevel);

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                           float pos, float startAngle, float endAngle, juce::Slider&) override;
private:
    const float& level;
    juce::Image face;
};

/** Circular hit area instead of the full square. */
struct RoundSlider : public juce::Slider
{
    bool hitTest (int x, int y) override
    {
        const auto c = getLocalBounds().toFloat().getCentre();
        return c.getDistanceFrom ({ (float) x, (float) y }) <= (float) juce::jmin (getWidth(), getHeight()) * 0.5f;
    }
};

//==============================================================================
/** Everything the user sees, drawn normally. The overlay pixelates a snapshot of this. */
class MelluContent : public juce::Component
{
public:
    MelluContent (MelluratorProcessor&, MelluLookAndFeel&, const float& level);
    ~MelluContent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    const float& level;
    RoundSlider drive, bass;
    juce::AudioProcessorValueTreeState::SliderAttachment driveAttach, bassAttach;
};

//==============================================================================
/** Sits on top, lets mouse events through, and draws a block-averaged copy of the content. */
class PixelOverlay : public juce::Component
{
public:
    PixelOverlay (juce::Component& target, const float& level);

    void paint (juce::Graphics&) override;
    void rerollGlitch();

private:
    struct Band { int y, h, dx; juce::Array<juce::Rectangle<int>> blocks; };

    juce::Component& target;
    const float& level;
    juce::Array<Band> bands;
    int bandBlock = 0;
    juce::Random rng;
};

//==============================================================================
class MelluratorEditor : public juce::AudioProcessorEditor,
                         private juce::Timer
{
public:
    explicit MelluratorEditor (MelluratorProcessor&);
    ~MelluratorEditor() override;

    void resized() override;

private:
    void timerCallback() override;

    MelluratorProcessor& proc;
    float level = 0.0f;           // must be declared before the members that reference it
    MelluLookAndFeel laf;
    MelluContent content;
    PixelOverlay overlay;
    int ticks = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MelluratorEditor)
};
