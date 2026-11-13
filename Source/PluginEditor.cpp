#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
    constexpr float kW = 640.0f, kH = 420.0f;
    constexpr float kFaceCx = 320.0f, kFaceCy = 224.0f, kFaceOuter = 176.0f;
    constexpr float kBassCx = 566.0f, kBassCy = 322.0f, kBassOuter = 44.0f;
    constexpr float kFaceTilt = juce::degreesToRadians (50.0f);
    constexpr float kRingStart = juce::degreesToRadians (-135.0f);
    constexpr float kRingSweep = juce::degreesToRadians (270.0f);

    juce::Point<float> onCircle (juce::Point<float> c, float r, float angleFromTop)
    {
        return { c.x + r * std::sin (angleFromTop), c.y - r * std::cos (angleFromTop) };
    }

    float textWidth (const juce::Font& f, const juce::String& s)
    {
        juce::GlyphArrangement ga;
        ga.addLineOfText (f, s, 0.0f, 0.0f);
        return ga.getBoundingBox (0, -1, true).getWidth();
    }
}

//==============================================================================
MelluLookAndFeel::MelluLookAndFeel (const float& driveLevel) : level (driveLevel)
{
    face = juce::ImageFileFormat::loadFrom (BinaryData::face_jpg, (size_t) BinaryData::face_jpgSize);
}

void MelluLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                         float pos, float startAngle, float endAngle, juce::Slider& s)
{
    const auto b      = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const auto c      = b.getCentre();
    const float outer = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const float t     = mello::tint (level);
    const auto acc    = mello::accent (t);
    const auto track  = juce::Colour (30, 62, 40).interpolatedWith (juce::Colour (64, 24, 20), t);

    if (s.getComponentID() == "drive")
    {
        // ring ticks, lit up to the current value
        const int n = 41;
        for (int i = 0; i < n; ++i)
        {
            const float p = (float) i / (float) (n - 1);
            const bool major = (i % 5 == 0);
            const float a  = kRingStart + kRingSweep * p;
            const float r1 = outer * 0.875f;
            const float r2 = r1 + outer * (major ? 0.085f : 0.051f);
            g.setColour (p <= pos + 1.0e-6f ? acc : track);
            g.drawLine ({ onCircle (c, r1, a), onCircle (c, r2, a) }, major ? 3.0f : 2.2f);
        }

        const float faceR = outer * 0.773f;
        const float shake = (level > 0.3f) ? std::pow (level, 2.2f) * 7.0f * (outer / kFaceOuter) : 0.0f;
        auto& rnd = juce::Random::getSystemRandom();
        const float jx = (rnd.nextFloat() - 0.5f) * shake, jy = (rnd.nextFloat() - 0.5f) * shake;

        g.setColour (acc);
        g.fillEllipse (c.x - faceR - 5.0f, c.y - faceR - 5.0f, (faceR + 5.0f) * 2.0f, (faceR + 5.0f) * 2.0f);

        g.saveState();
        juce::Path clip;
        clip.addEllipse (c.x - faceR, c.y - faceR, faceR * 2.0f, faceR * 2.0f);
        g.reduceClipRegion (clip);

        g.setColour (juce::Colour (10, 70, 28).interpolatedWith (juce::Colour (80, 8, 6), t));
        g.fillPath (clip);

        const float angle = juce::jmap (pos, startAngle, endAngle);   // the knob's rotary range is +-50 degrees
        g.addTransform (juce::AffineTransform::rotation (angle, c.x, c.y).translated (jx, jy));
        if (face.isValid())
        {
            const float sz = faceR * 2.0f * 1.1f * (1.0f + 0.07f * level);
            g.drawImage (face, juce::Rectangle<float> (c.x - sz * 0.5f, c.y - sz * 0.5f + 4.0f, sz, sz),
                         juce::RectanglePlacement::stretchToFit);
        }
        // skin goes red as drive rises (JUCE has no multiply blend, so a translucent fill stands in)
        g.setColour (juce::Colour (255, 20, 10).withAlpha (0.45f * t));
        g.fillRect (c.x - outer, c.y - outer, outer * 2.0f, outer * 2.0f);
        g.restoreState();
    }
    else
    {
        // BASS: small plain knob with an arc around it
        const float trackR = outer * 0.864f, knobR = outer * 0.568f;
        juce::Path arc;
        arc.addCentredArc (c.x, c.y, trackR, trackR, 0.0f, kRingStart, kRingStart + kRingSweep, true);
        g.setColour (track);
        g.strokePath (arc, juce::PathStrokeType (4.0f));

        const float a = juce::jmap (pos, startAngle, endAngle);
        if (pos > 0.002f)
        {
            juce::Path val;
            val.addCentredArc (c.x, c.y, trackR, trackR, 0.0f, startAngle, a, true);
            g.setColour (acc);
            g.strokePath (val, juce::PathStrokeType (4.0f));
        }

        g.setColour (juce::Colour (12, 24, 15).interpolatedWith (juce::Colour (30, 10, 9), t));
        g.fillEllipse (c.x - knobR, c.y - knobR, knobR * 2.0f, knobR * 2.0f);
        g.setColour (acc);
        g.drawEllipse (c.x - knobR, c.y - knobR, knobR * 2.0f, knobR * 2.0f, 2.0f);
        g.drawLine ({ onCircle (c, 9.0f, a), onCircle (c, knobR - 3.0f, a) }, 3.5f);
    }
}

//==============================================================================
MelluContent::MelluContent (MelluratorProcessor& p, MelluLookAndFeel& laf, const float& lvl)
    : level (lvl),
      driveAttach (p.apvts, "drive", drive),
      bassAttach  (p.apvts, "bass",  bass)
{
    auto setup = [&] (juce::Slider& s, const juce::String& id, float lo, float hi, double def)
    {
        s.setComponentID (id);
        s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setRotaryParameters (lo, hi, true);
        s.setMouseDragSensitivity (230);
        s.setDoubleClickReturnValue (true, def);
        s.setLookAndFeel (&laf);
        addAndMakeVisible (s);
    };
    setup (drive, "drive", -kFaceTilt, kFaceTilt, 0.30);
    setup (bass,  "bass",  kRingStart, kRingStart + kRingSweep, 0.35);
}

MelluContent::~MelluContent()
{
    drive.setLookAndFeel (nullptr);
    bass.setLookAndFeel (nullptr);
}

void MelluContent::resized()
{
    drive.setBounds (juce::Rectangle<float> (kFaceOuter * 2, kFaceOuter * 2)
                         .withCentre ({ kFaceCx, kFaceCy }).toNearestInt());
    bass.setBounds (juce::Rectangle<float> (kBassOuter * 2, kBassOuter * 2)
                        .withCentre ({ kBassCx, kBassCy }).toNearestInt());
}

void MelluContent::paint (juce::Graphics& g)
{
    const float t  = mello::tint (level);
    const auto acc = mello::accent (t);
    const auto dim = juce::Colour (111, 143, 120).interpolatedWith (juce::Colour (190, 100, 92), t);

    g.fillAll (juce::Colour (6, 10, 7).interpolatedWith (juce::Colour (22, 5, 5), t));

    juce::ColourGradient glow (acc.withAlpha (0.38f), kFaceCx, kFaceCy,
                               acc.withAlpha (0.0f),  kFaceCx + 310.0f, kFaceCy, true);
    glow.addColour (0.55, acc.withAlpha (0.11f));
    g.setGradientFill (glow);
    g.fillRect (getLocalBounds());

    g.setColour (juce::Colour (26, 54, 34).interpolatedWith (juce::Colour (96, 22, 18), t));
    g.drawRect (getLocalBounds(), 1);

    // header
    const auto logo = mello::display (22.0f);
    g.setFont (logo);
    g.setColour (acc);
    g.drawText ("MELL", 26, 16, 100, 28, juce::Justification::centredLeft);
    g.setColour (juce::Colour (233, 255, 240));
    g.drawText ("URATOR", 26 + (int) textWidth (logo, "MELL") + 2, 16, 160, 28, juce::Justification::centredLeft);
    g.setFont (mello::mono (11.0f).withExtraKerningFactor (0.15f));
    g.setColour (dim);
    g.drawText ("VST3 · FL STUDIO", (int) kW - 26 - 200, 16, 200, 28, juce::Justification::centredRight);

    // readouts
    g.setFont (mello::mono (11.0f).withExtraKerningFactor (0.2f));
    g.setColour (dim);
    g.drawText ("DRIVE", 200, 394, 112, 20, juce::Justification::centredRight);
    g.drawText ("BASS", (int) kBassCx - 50, 372, 100, 16, juce::Justification::centred);
    g.drawText ("SATURATOR + LOW BOOST", 26, 394, 220, 20, juce::Justification::centredLeft);

    g.setColour (acc);
    g.setFont (mello::display (19.0f));
    g.drawText (juce::String (juce::roundToInt ((float) drive.getValue() * 100.0f)) + "%",
                328, 394, 100, 20, juce::Justification::centredLeft);
    g.setFont (mello::display (14.0f));
    g.drawText ("+" + juce::String ((float) bass.getValue() * 9.0f, 1) + " dB",
                (int) kBassCx - 50, 392, 100, 18, juce::Justification::centred);
}

//==============================================================================
PixelOverlay::PixelOverlay (juce::Component& t, const float& lvl) : target (t), level (lvl)
{
    setInterceptsMouseClicks (false, false);   // clicks reach the real knobs underneath
    setOpaque (false);
}

void PixelOverlay::rerollGlitch()
{
    bands.clear();
    if (level <= 0.45f || bandBlock <= 0)
        return;

    const int n = 1 + (int) (level * 4.0f);
    for (int i = 0; i < n; ++i)
    {
        Band b;
        const int rows = juce::jmax (1, juce::roundToInt ((1.0f + rng.nextFloat() * 2.0f) * (1.0f + level)));
        b.h  = rows * bandBlock;
        b.y  = rng.nextInt (juce::jmax (1, getHeight() / bandBlock - rows)) * bandBlock;
        b.dx = juce::roundToInt ((rng.nextFloat() - 0.4f) * (3.0f + level * 10.0f)) * bandBlock;
        const int count = 2 + (int) (level * 5.0f);
        for (int k = 0; k < count; ++k)
            b.blocks.add ({ rng.nextInt (juce::jmax (1, getWidth() / bandBlock)) * bandBlock, b.y,
                            (2 + rng.nextInt (8)) * bandBlock, b.h });
        bands.add (b);
    }
}

void PixelOverlay::paint (juce::Graphics& g)
{
    const int block = juce::roundToInt (1.0f + 15.0f * std::pow (juce::jmax (0.0f, level - 0.06f) / 0.94f, 1.6f));
    if (block <= 1)
        return;                                   // nothing to pixelate yet: the real UI shows through

    bandBlock = block;

    const auto snap = target.createComponentSnapshot (target.getLocalBounds(), false, 1.0f);
    const int w = snap.getWidth(), h = snap.getHeight();
    juce::Image out (juce::Image::ARGB, w, h, false);
    {
        juce::Image::BitmapData in (snap, juce::Image::BitmapData::readOnly);
        juce::Image::BitmapData dst (out, juce::Image::BitmapData::writeOnly);

        for (int by = 0; by < h; by += block)
            for (int bx = 0; bx < w; bx += block)
            {
                const int x1 = juce::jmin (bx + block, w), y1 = juce::jmin (by + block, h);
                int r = 0, gg = 0, b = 0, count = 0;
                for (int yy = by; yy < y1; ++yy)
                    for (int xx = bx; xx < x1; ++xx)
                    {
                        const auto px = in.getPixelColour (xx, yy);
                        r += px.getRed(); gg += px.getGreen(); b += px.getBlue(); ++count;
                    }
                const auto avg = juce::Colour ((juce::uint8) (r / count), (juce::uint8) (gg / count), (juce::uint8) (b / count));
                for (int yy = by; yy < y1; ++yy)
                    for (int xx = bx; xx < x1; ++xx)
                        dst.setPixelColour (xx, yy, avg);
            }
    }
    g.drawImageAt (out, 0, 0);

    const float t = mello::tint (level);
    if (level > 0.45f)
    {
        const float k = juce::jlimit (0.0f, 1.0f, (level - 0.45f) / 0.55f);
        for (const auto& band : bands)
        {
            g.drawImage (out, band.dx, band.y, w, band.h, 0, band.y, w, band.h);   // shifted slice
            g.setColour (juce::Colour (255, 150, 70).withAlpha (0.10f + 0.14f * k));
            g.fillRect (0, band.y, w, band.h);
            g.setColour (juce::Colour (255, 232, 214).withAlpha (0.28f + 0.30f * k));
            for (const auto& r : band.blocks)
                g.fillRect (r);
        }
    }

    g.setColour (juce::Colour (255, 24, 12).withAlpha (0.25f * t));
    g.fillAll();
}

//==============================================================================
MelluratorEditor::MelluratorEditor (MelluratorProcessor& p)
    : juce::AudioProcessorEditor (p), proc (p), laf (level), content (p, laf, level), overlay (content, level)
{
    setSize ((int) kW, (int) kH);
    setResizable (false, false);

    addAndMakeVisible (content);
    addAndMakeVisible (overlay);       // added last = on top
    startTimerHz (30);
}

MelluratorEditor::~MelluratorEditor()
{
    stopTimer();
}

void MelluratorEditor::resized()
{
    content.setBounds (getLocalBounds());
    overlay.setBounds (getLocalBounds());
}

void MelluratorEditor::timerCallback()
{
    const float target = proc.apvts.getRawParameterValue ("drive")->load();
    const bool changed = std::abs (target - level) > 0.0005f;
    level = target;

    const bool animated = level > 0.3f;          // shake and glitch keep moving at high drive
    if (animated && (ticks++ % 4 == 0))
        overlay.rerollGlitch();

    if (changed || animated)
    {
        content.repaint();
        overlay.repaint();
    }
}
