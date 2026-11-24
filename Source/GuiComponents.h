#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

// Todo el dibujo está pensado sobre el lienzo de la imagen original (1536 x 798 px)
// y se escala con el tamaño de la ventana.
namespace rs2a::gui
{
using namespace juce;

inline Font makeFont (float h, bool bold = false)
{
    return Font (FontOptions (h, bold ? Font::bold : Font::plain));
}

// Texto con separación entre letras, centrado en (cx, cy)
inline void drawSpaced (Graphics& g, const String& text, float cx, float cy,
                        float h, float spacing, bool bold = true)
{
    const Font f = makeFont (h, bold);
    g.setFont (f);

    std::vector<float> w;
    float total = 0.f;
    for (int i = 0; i < text.length(); ++i)
    {
        const float cw = GlyphArrangement::getStringWidth (f, String::charToString (text[i]));
        w.push_back (cw);
        total += cw;
    }
    total += spacing * (float) jmax (0, text.length() - 1);

    float x = cx - total * 0.5f;
    for (int i = 0; i < text.length(); ++i)
    {
        g.drawText (String::charToString (text[i]),
                    Rectangle<float> (x - 1.f, cy - h * 0.7f, w[(size_t) i] + 2.f, h * 1.4f),
                    Justification::centred, false);
        x += w[(size_t) i] + spacing;
    }
}

inline Point<float> polar (Point<float> c, float r, float degFromTop)
{
    const float a = degreesToRadians (degFromTop);
    return { c.x + r * std::sin (a), c.y - r * std::cos (a) };
}

inline void drawLedGlow (Graphics& g, Point<float> c, float r, bool on, Colour col)
{
    if (on)
    {
        g.setGradientFill (ColourGradient (col.withAlpha (0.55f), c.x, c.y,
                                           col.withAlpha (0.0f), c.x + r * 3.f, c.y, true));
        g.fillEllipse (c.x - r * 3.f, c.y - r * 3.f, r * 6.f, r * 6.f);
    }
    g.setColour (Colours::black);
    g.fillEllipse (c.x - r * 1.25f, c.y - r * 1.25f, r * 2.5f, r * 2.5f);
    g.setGradientFill (ColourGradient (on ? col.brighter (0.7f) : col.darker (0.9f).withAlpha (0.8f),
                                       c.x - r * 0.3f, c.y - r * 0.3f,
                                       on ? col : col.darker (1.2f).withAlpha (0.7f),
                                       c.x + r, c.y + r, true));
    g.fillEllipse (c.x - r, c.y - r, r * 2.f, r * 2.f);
}

//==============================================================================
class KnobLookAndFeel : public LookAndFeel_V4
{
public:
    void drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                           float startA, float endA, Slider&) override
    {
        const Rectangle<float> b ((float) x, (float) y, (float) w, (float) h);
        const auto c = b.getCentre();
        const float R = jmin (b.getWidth(), b.getHeight()) * 0.5f - 1.f;
        const float ang = startA + pos * (endA - startA);

        // sombra
        g.setColour (Colours::black.withAlpha (0.35f));
        g.fillEllipse (c.x - R, c.y - R + R * 0.06f, R * 2.f, R * 2.f);

        // base negra
        g.setGradientFill (ColourGradient (Colour (0xff343434), c.x - R * 0.4f, c.y - R * 0.5f,
                                           Colour (0xff050505), c.x + R, c.y + R, true));
        g.fillEllipse (c.x - R, c.y - R, R * 2.f, R * 2.f);
        g.setColour (Colours::white.withAlpha (0.09f));
        g.drawEllipse (c.x - R + 0.5f, c.y - R + 0.5f, R * 2.f - 1.f, R * 2.f - 1.f, 1.f);

        // cuerpo estriado
        {
            Path p;
            const int lobes = 18;
            const float r1 = R * 0.80f, r2 = R * 0.72f;
            for (int k = 0; k < lobes * 2; ++k)
            {
                const float a = (float) k * MathConstants<float>::pi / (float) lobes;
                const float r = (k % 2 == 0) ? r1 : r2;
                const float px = c.x + r * std::sin (a), py = c.y - r * std::cos (a);
                if (k == 0) p.startNewSubPath (px, py); else p.lineTo (px, py);
            }
            p.closeSubPath();
            p.applyTransform (AffineTransform::rotation (ang, c.x, c.y));

            g.setGradientFill (ColourGradient (Colour (0xff2c2c2c), c.x - R * 0.5f, c.y - R * 0.6f,
                                               Colour (0xff0a0a0a), c.x + R * 0.7f, c.y + R * 0.8f, true));
            g.fillPath (p);
            g.setColour (Colours::white.withAlpha (0.12f));
            g.strokePath (p, PathStrokeType (1.f));
        }

        // tapa metálica
        const float rc = R * 0.52f;
        g.setColour (Colours::black.withAlpha (0.55f));
        g.fillEllipse (c.x - rc - 2.f, c.y - rc - 1.f, rc * 2.f + 4.f, rc * 2.f + 4.f);
        g.setGradientFill (ColourGradient (Colour (0xfff0f0ee), c.x - rc * 0.5f, c.y - rc * 0.6f,
                                           Colour (0xff6a6a68), c.x + rc, c.y + rc, true));
        g.fillEllipse (c.x - rc, c.y - rc, rc * 2.f, rc * 2.f);

        // acabado cepillado
        for (int k = 1; k <= 7; ++k)
        {
            const float rr = rc * (float) k / 7.5f;
            g.setColour ((k % 2 ? Colours::white : Colours::black).withAlpha (0.10f));
            g.drawEllipse (c.x - rr, c.y - rr, rr * 2.f, rr * 2.f, 0.7f);
        }
        {
            Path sheen;
            sheen.addPieSegment (c.x - rc, c.y - rc, rc * 2.f, rc * 2.f,
                                 ang + 0.4f, ang + 1.1f, 0.f);
            sheen.addPieSegment (c.x - rc, c.y - rc, rc * 2.f, rc * 2.f,
                                 ang + 0.4f + MathConstants<float>::pi,
                                 ang + 1.1f + MathConstants<float>::pi, 0.f);
            g.setColour (Colours::white.withAlpha (0.16f));
            g.fillPath (sheen);
        }
        g.setColour (Colours::black.withAlpha (0.35f));
        g.drawEllipse (c.x - rc, c.y - rc, rc * 2.f, rc * 2.f, 1.f);

        // indicador blanco
        const float r0 = R * 0.56f, r1 = R * 0.84f;
        g.setColour (Colour (0xfff4f3ee));
        g.drawLine (c.x + r0 * std::sin (ang), c.y - r0 * std::cos (ang),
                    c.x + r1 * std::sin (ang), c.y - r1 * std::cos (ang),
                    jmax (1.6f, R * 0.05f));
    }
};

//==============================================================================
// Interruptor de palanca (toggle). Activado = palanca hacia arriba.
class BatSwitch : public Button
{
public:
    BatSwitch() : Button ("switch") { setClickingTogglesState (true); }

    void paintButton (Graphics& g, bool, bool) override
    {
        const auto b = getLocalBounds().toFloat();
        const auto c = b.getCentre();
        const float w = jmin (b.getWidth(), b.getHeight() * 0.75f);

        // tuerca base
        const float rb = w * 0.42f;
        g.setColour (Colours::black.withAlpha (0.5f));
        g.fillEllipse (c.x - rb, c.y - rb + 2.f, rb * 2.f, rb * 2.f);
        g.setGradientFill (ColourGradient (Colour (0xffe9e9e6), c.x - rb * 0.5f, c.y - rb * 0.6f,
                                           Colour (0xff55554f), c.x + rb, c.y + rb, true));
        g.fillEllipse (c.x - rb, c.y - rb, rb * 2.f, rb * 2.f);
        g.setColour (Colours::black.withAlpha (0.5f));
        g.drawEllipse (c.x - rb, c.y - rb, rb * 2.f, rb * 2.f, 1.2f);

        // palanca
        const float dir = getToggleState() ? -1.f : 1.f;
        const float len = b.getHeight() * 0.36f;
        const Point<float> tip (c.x + dir * -w * 0.03f, c.y + dir * len);
        const float lw = w * 0.20f;

        g.setColour (Colours::black.withAlpha (0.4f));
        g.drawLine (c.x + 2.f, c.y + 2.f, tip.x + 2.f, tip.y + 2.f, lw);
        g.setGradientFill (ColourGradient (Colour (0xfff2f2ef), c.x - lw, c.y,
                                           Colour (0xff6a6a66), c.x + lw, c.y, false));
        g.drawLine (c.x, c.y, tip.x, tip.y, lw);

        const float rt = lw * 0.9f;
        g.setGradientFill (ColourGradient (Colour (0xffffffff), tip.x - rt * 0.4f, tip.y - rt * 0.4f,
                                           Colour (0xff77776f), tip.x + rt, tip.y + rt, true));
        g.fillEllipse (tip.x - rt, tip.y - rt, rt * 2.f, rt * 2.f);
    }
};

// Botón BYPASS: iluminado = procesando; apagado = bypass.
class BypassButton : public Button
{
public:
    BypassButton() : Button ("bypass") { setClickingTogglesState (true); }

    void paintButton (Graphics& g, bool, bool down) override
    {
        const auto b = getLocalBounds().toFloat().reduced (2.f);
        const bool lit = ! getToggleState();

        g.setGradientFill (ColourGradient (Colour (0xff3a3a3a), b.getX(), b.getY(),
                                           Colour (0xff070707), b.getRight(), b.getBottom(), false));
        g.fillRoundedRectangle (b, b.getWidth() * 0.12f);
        g.setColour (Colours::white.withAlpha (0.15f));
        g.drawRoundedRectangle (b, b.getWidth() * 0.12f, 1.2f);

        auto f = b.reduced (b.getWidth() * 0.14f);
        if (down) f = f.reduced (1.5f);
        if (lit)
        {
            g.setGradientFill (ColourGradient (Colour (0xffffe3a0), f.getCentreX(), f.getCentreY(),
                                               Colour (0xffe8891a), f.getX(), f.getY(), true));
        }
        else
            g.setColour (Colour (0xff4b3b22));
        g.fillRoundedRectangle (f, f.getWidth() * 0.1f);
        g.setColour (Colours::black.withAlpha (0.55f));
        g.drawRoundedRectangle (f, f.getWidth() * 0.1f, 1.4f);
    }
};

// LED simple (el color es ámbar por defecto)
class Led : public Component
{
public:
    explicit Led (Colour c = Colour (0xffffb02e)) : col (c) { setInterceptsMouseClicks (false, false); }
    void setOn (bool v) { if (v != on) { on = v; repaint(); } }
    void paint (Graphics& g) override
    {
        const auto c = getLocalBounds().toFloat().getCentre();
        drawLedGlow (g, c, getWidth() / 7.f, on, col);
    }
private:
    Colour col;
    bool on = false;
};

//==============================================================================
// Ventana del VU / GR (ocupa el rectángulo ámbar 550 x 175 de la imagen)
class MeterView : public Component, private Timer
{
public:
    explicit MeterView (RS2AAudioProcessor& p) : proc (p)
    {
        vugr = proc.apvts.getRawParameterValue (ParamIDs::vugr);
        setInterceptsMouseClicks (false, false);
        startTimerHz (30);
    }

    void timerCallback() override
    {
        const float gr = proc.grDb.load();
        const float vuT = (vugr->load() > 0.5f) ? -gr : proc.vuDb.load();
        vuShown += 0.30f * (jlimit (-22.f, 4.f, vuT) - vuShown);
        grShown += 0.30f * (jlimit (0.f, 22.f, gr) - grShown);
        repaint();
    }

    void paint (Graphics& g) override
    {
        g.addTransform (AffineTransform::scale (getWidth() / 550.f));
        const Rectangle<float> win (0.f, 0.f, 550.f, 175.f);

        // vidrio ámbar retroiluminado
        g.setGradientFill (ColourGradient (Colour (0xfffae3a6), 275.f, 95.f,
                                           Colour (0xffb87a22), 0.f, 0.f, true));
        g.fillRect (win);

        auto edge = [&] (float x1, float y1, float x2, float y2)
        {
            g.setGradientFill (ColourGradient (Colours::black.withAlpha (0.45f), x1, y1,
                                               Colours::transparentBlack, x2, y2, false));
            g.fillRect (win);
        };
        edge (0.f, 0.f, 0.f, 26.f);
        edge (0.f, 175.f, 0.f, 150.f);
        edge (0.f, 0.f, 30.f, 0.f);
        edge (550.f, 0.f, 520.f, 0.f);

        const Colour ink (0xff2b1c08);
        const Point<float> pVU (237.f, 327.f), pGR (357.f, 307.f);

        // ---- escala VU ----
        {
            g.setColour (ink);
            Path arc;
            arc.addCentredArc (pVU.x, pVU.y, 253.f, 253.f, 0.f,
                               degreesToRadians (vuDeg[0]), degreesToRadians (vuDeg[9]), true);
            g.strokePath (arc, PathStrokeType (1.6f));

            Path red;
            red.addCentredArc (pVU.x, pVU.y, 255.f, 255.f, 0.f,
                               degreesToRadians (vuDeg[7]), degreesToRadians (vuDeg[9]), true);
            g.setColour (Colour (0xffc9281a));
            g.strokePath (red, PathStrokeType (7.f));

            g.setColour (ink);
            for (int db = -20; db <= 3; ++db)
            {
                bool labelled = false;
                for (float v : vuDb) if ((int) v == db) labelled = true;
                const float deg = interp (vuDb, vuDeg, 10, (float) db);
                const auto a = polar (pVU, 253.f, deg);
                const auto b = polar (pVU, labelled ? 239.f : 246.f, deg);
                g.drawLine (a.x, a.y, b.x, b.y, labelled ? 1.8f : 1.1f);
            }
            static const char* labels[10] = { "-20", "-10", "-7", "-5", "-3", "-1", "0", "+1", "+2", "+3" };
            g.setFont (makeFont (15.f, false));
            for (int i = 0; i < 10; ++i)
            {
                const auto lp = polar (pVU, 278.f, vuDeg[i]);
                g.drawText (labels[i], Rectangle<float> (44.f, 20.f).withCentre (lp), Justification::centred, false);
            }
            g.setFont (makeFont (19.f, true));
            g.drawText ("VU", Rectangle<float> (50.f, 24.f).withCentre ({ 39.f, 47.f }), Justification::centred, false);
        }

        // ---- escala GR ----
        {
            g.setColour (ink);
            Path arc;
            arc.addCentredArc (pGR.x, pGR.y, 222.f, 222.f, 0.f,
                               degreesToRadians (grDeg[0]), degreesToRadians (grDeg[5]), true);
            g.strokePath (arc, PathStrokeType (1.6f));

            Path red;
            red.addCentredArc (pGR.x, pGR.y, 224.f, 224.f, 0.f,
                               degreesToRadians (grDeg[4]), degreesToRadians (grDeg[5]), true);
            g.setColour (Colour (0xffc9281a));
            g.strokePath (red, PathStrokeType (6.f));

            g.setColour (ink);
            static const float minor[] = { 2.f, 4.f, 5.f, 8.f, 15.f };
            for (float v : minor)
            {
                const float deg = interp (grDbT, grDeg, 6, v);
                const auto a = polar (pGR, 222.f, deg), b = polar (pGR, 213.f, deg);
                g.drawLine (a.x, a.y, b.x, b.y, 1.1f);
            }
            static const char* labels[6] = { "0", "1", "3", "6", "10", "20" };
            g.setFont (makeFont (15.f, false));
            for (int i = 0; i < 6; ++i)
            {
                const auto a = polar (pGR, 222.f, grDeg[i]), b = polar (pGR, 208.f, grDeg[i]);
                g.drawLine (a.x, a.y, b.x, b.y, 1.8f);
                const auto lp = polar (pGR, 242.f, grDeg[i]);
                g.drawText (labels[i], Rectangle<float> (34.f, 20.f).withCentre (lp), Justification::centred, false);
            }
            g.setFont (makeFont (19.f, true));
            g.drawText ("GR", Rectangle<float> (50.f, 24.f).withCentre ({ 504.f, 57.f }), Justification::centred, false);
        }

        // marca
        g.setColour (ink.withAlpha (0.85f));
        drawSpaced (g, "SARMI STUDIOS", 270.f, 154.f, 12.f, 2.2f, false);

        // ---- agujas ----
        auto needle = [&] (Point<float> pivot, float deg, float len)
        {
            const auto tip = polar (pivot, len, deg);
            g.setColour (Colours::black.withAlpha (0.22f));
            g.drawLine (pivot.x + 3.f, pivot.y + 2.f, tip.x + 3.f, tip.y + 2.f, 2.6f);
            g.setColour (Colour (0xff111111));
            g.drawLine (pivot.x, pivot.y, tip.x, tip.y, 2.6f);
        };
        needle (pVU, interp (vuDb, vuDeg, 10, vuShown), 255.f);
        needle (pGR, interp (grDbT, grDeg, 6, grShown), 222.f);

        // reflejo del cristal y borde
        {
            Path glare;
            glare.startNewSubPath (0.f, 0.f);
            glare.lineTo (300.f, 0.f);
            glare.lineTo (110.f, 175.f);
            glare.lineTo (0.f, 175.f);
            glare.closeSubPath();
            g.setColour (Colours::white.withAlpha (0.045f));
            g.fillPath (glare);
        }
        g.setColour (Colours::black.withAlpha (0.6f));
        g.drawRect (win, 2.5f);
    }

private:
    static float interp (const float* xs, const float* ys, int n, float x)
    {
        int i = 0;
        while (i < n - 2 && x > xs[i + 1]) ++i;
        const float t = (x - xs[i]) / (xs[i + 1] - xs[i]);
        return ys[i] + t * (ys[i + 1] - ys[i]);
    }

    static constexpr float vuDb[10]  = { -20.f, -10.f, -7.f, -5.f, -3.f, -1.f, 0.f, 1.f, 2.f, 3.f };
    static constexpr float vuDeg[10] = { -42.8f, -33.4f, -26.f, -19.5f, -12.9f, -6.4f, 0.f, 6.8f, 14.2f, 22.4f };
    static constexpr float grDbT[6]  = { 0.f, 1.f, 3.f, 6.f, 10.f, 20.f };
    static constexpr float grDeg[6]  = { 9.7f, 16.1f, 22.f, 27.7f, 34.1f, 40.4f };

    RS2AAudioProcessor& proc;
    std::atomic<float>* vugr = nullptr;
    float vuShown = -22.f, grShown = 0.f;
};

//==============================================================================
// Medidor de nivel de salida de 6 LEDs (+3 .. -18)
class LedMeter : public Component, private Timer
{
public:
    explicit LedMeter (RS2AAudioProcessor& p) : proc (p)
    {
        setInterceptsMouseClicks (false, false);
        startTimerHz (30);
    }

    void timerCallback() override
    {
        const float pk = proc.outPeakLin.exchange (0.f);
        const float db = Decibels::gainToDecibels (pk, -90.f);
        shown = jmax (db, shown - 1.5f);
        repaint();
    }

    void paint (Graphics& g) override
    {
        g.addTransform (AffineTransform::scale (getWidth() / 30.f));
        g.setColour (Colour (0xff050505));
        g.fillRoundedRectangle (0.f, 0.f, 30.f, 147.f, 3.f);
        g.setColour (Colour (0xff4a4a4a));
        g.drawRoundedRectangle (0.5f, 0.5f, 29.f, 146.f, 3.f, 1.2f);

        static const float th[6] = { 3.f, 0.f, -3.f, -6.f, -12.f, -18.f };
        static const Colour cols[6] = { Colour (0xffff5a2a), Colour (0xffffb020), Colour (0xffc8e020),
                                        Colour (0xff3cf03c), Colour (0xff3cf03c), Colour (0xff3cf03c) };
        for (int i = 0; i < 6; ++i)
        {
            const Rectangle<float> r (5.f, 14.f + 23.8f * (float) i - 8.f, 20.f, 16.f);
            const bool on = shown >= th[i];
            g.setColour (on ? cols[i] : cols[i].withAlpha (0.16f));
            g.fillRoundedRectangle (r, 2.f);
            if (on)
            {
                g.setColour (cols[i].withAlpha (0.35f));
                g.drawRoundedRectangle (r.expanded (1.5f), 3.f, 2.f);
            }
        }
    }

private:
    RS2AAudioProcessor& proc;
    float shown = -90.f;
};
} // namespace rs2a::gui
