#include "PluginEditor.h"

using namespace juce;
using namespace rs2a::gui;

namespace
{
    constexpr float kW = 1536.f, kH = 798.f;
    // Nombres que aparecen en la placa frontal (cámbialos aquí)
    const char* kBrand = "SARMI";
    const char* kBrandSub = "STUDIOS";
    const char* kModel = "RS-2A";

    struct Mark { float v; String label; bool major; };

    std::vector<Mark> marks100()
    {
        std::vector<Mark> m;
        for (int i = 0; i <= 20; ++i)
            m.push_back ({ (float) i / 20.f, (i % 2 == 0) ? String (i / 2 * 10) : String(), i % 2 == 0 });
        return m;
    }

    std::vector<Mark> marksStops (const StringArray& labels)
    {
        std::vector<Mark> m;
        const int n = labels.size();
        for (int i = 0; i < n; ++i)
            m.push_back ({ (float) i / (float) (n - 1), labels[i], true });
        return m;
    }

    std::vector<Mark> marksMix()
    {
        std::vector<Mark> m;
        for (int i = 0; i <= 10; ++i)
            m.push_back ({ (float) i / 10.f,
                           i == 0 ? "0" : i == 5 ? "50" : i == 10 ? "100" : "", i % 5 == 0 });
        return m;
    }

    std::vector<Mark> marksDots()
    {
        std::vector<Mark> m;
        for (int i = 0; i <= 10; ++i) m.push_back ({ (float) i / 10.f, {}, false });
        return m;
    }

    void drawScale (Graphics& g, Point<float> c, float R, const std::vector<Mark>& marks,
                    float labelMul, float fontH, Colour col, bool arc = true)
    {
        const float arcR = R * 1.10f;
        g.setColour (col);
        if (arc)
        {
            Path p;
            p.addCentredArc (c.x, c.y, arcR, arcR, 0.f, degreesToRadians (-135.f), degreesToRadians (135.f), true);
            g.strokePath (p, PathStrokeType (1.4f));
        }
        for (auto& m : marks)
        {
            const float deg = -135.f + 270.f * m.v;
            const auto a = polar (c, arcR, deg);
            const auto b = polar (c, arcR + (m.major ? 0.13f : 0.07f) * R, deg);
            g.drawLine (a.x, a.y, b.x, b.y, m.major ? 2.2f : 1.3f);
            if (m.label.isNotEmpty())
            {
                g.setFont (makeFont (fontH, false));
                g.drawText (m.label, Rectangle<float> (fontH * 4.f, fontH * 1.6f).withCentre (polar (c, R * labelMul, deg)),
                            Justification::centred, false);
            }
        }
    }

    void drawScrew (Graphics& g, Point<float> c, float r)
    {
        g.setColour (Colours::black.withAlpha (0.5f));
        g.fillEllipse (c.x - r, c.y - r + 2.f, r * 2.f, r * 2.f);
        g.setGradientFill (ColourGradient (Colour (0xff5b5b58), c.x - r * 0.5f, c.y - r * 0.6f,
                                           Colour (0xff111111), c.x + r, c.y + r, true));
        g.fillEllipse (c.x - r, c.y - r, r * 2.f, r * 2.f);
        g.setColour (Colour (0xff050505));
        g.drawLine (c.x - r * 0.6f, c.y - r * 0.15f, c.x + r * 0.6f, c.y - r * 0.15f, 2.4f);
        g.drawLine (c.x - r * 0.15f, c.y - r * 0.6f, c.x - r * 0.15f, c.y + r * 0.6f, 2.4f);
    }
}

//==============================================================================
RS2AAudioProcessorEditor::RS2AAudioProcessorEditor (RS2AAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p), meter (p), ledMeter (p)
{
    // textura de ruido para el metal
    noiseTile = Image (Image::ARGB, 256, 256, true);
    {
        Random r (1234);
        Image::BitmapData bd (noiseTile, Image::BitmapData::writeOnly);
        for (int y = 0; y < 256; ++y)
            for (int x = 0; x < 256; ++x)
            {
                const bool dark = r.nextBool();
                const uint8 a = (uint8) r.nextInt (24);
                bd.setPixelColour (x, y, Colour::fromRGBA (dark ? 0 : 255, dark ? 0 : 255, dark ? 0 : 255, a));
            }
    }

    setupKnob (peakK,    ParamIDs::peak);
    setupKnob (gainK,    ParamIDs::gain);
    setupKnob (hpfK,     ParamIDs::hpf);
    setupKnob (tubeK,    ParamIDs::tube);
    setupKnob (mixK,     ParamIDs::mix);
    setupKnob (attackK,  ParamIDs::attack);
    setupKnob (releaseK, ParamIDs::release);
    setupKnob (outputK,  ParamIDs::output);

    peakA    = std::make_unique<SA> (proc.apvts, ParamIDs::peak,    peakK);
    gainA    = std::make_unique<SA> (proc.apvts, ParamIDs::gain,    gainK);
    hpfA     = std::make_unique<SA> (proc.apvts, ParamIDs::hpf,     hpfK);
    tubeA    = std::make_unique<SA> (proc.apvts, ParamIDs::tube,    tubeK);
    mixA     = std::make_unique<SA> (proc.apvts, ParamIDs::mix,     mixK);
    attackA  = std::make_unique<SA> (proc.apvts, ParamIDs::attack,  attackK);
    releaseA = std::make_unique<SA> (proc.apvts, ParamIDs::release, releaseK);
    outputA  = std::make_unique<SA> (proc.apvts, ParamIDs::output,  outputK);

    // doble clic = valor por defecto
    {
        const std::vector<std::pair<Slider*, String>> defaults {
            { &peakK, ParamIDs::peak },     { &gainK, ParamIDs::gain },
            { &hpfK, ParamIDs::hpf },       { &tubeK, ParamIDs::tube },
            { &mixK, ParamIDs::mix },       { &attackK, ParamIDs::attack },
            { &releaseK, ParamIDs::release }, { &outputK, ParamIDs::output } };

        for (auto& d : defaults)
        {
            auto* prm = proc.apvts.getParameter (d.second);
            d.first->setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
        }
    }

    for (auto* c : std::initializer_list<Component*> { &limitSw, &noiseSw, &vuGrSw, &bypassBtn,
                                                       &limitLed, &noiseLed, &vuLed, &meter, &ledMeter })
        addAndMakeVisible (c);

    limitA  = std::make_unique<BA> (proc.apvts, ParamIDs::limit,  limitSw);
    noiseA  = std::make_unique<BA> (proc.apvts, ParamIDs::noise,  noiseSw);
    vugrA   = std::make_unique<BA> (proc.apvts, ParamIDs::vugr,   vuGrSw);
    bypassA = std::make_unique<BA> (proc.apvts, ParamIDs::bypass, bypassBtn);

    limitRaw = proc.apvts.getRawParameterValue (ParamIDs::limit);
    noiseRaw = proc.apvts.getRawParameterValue (ParamIDs::noise);
    vugrRaw  = proc.apvts.getRawParameterValue (ParamIDs::vugr);

    constrainer.setFixedAspectRatio ((double) kW / (double) kH);
    constrainer.setSizeLimits (640, 332, 1920, 997);
    setConstrainer (&constrainer);
    setResizable (true, true);
    setSize (1000, (int) std::round (1000.0 * kH / kW));

    startTimerHz (20);
    timerCallback();
}

RS2AAudioProcessorEditor::~RS2AAudioProcessorEditor()
{
    for (auto* s : { &peakK, &gainK, &hpfK, &tubeK, &mixK, &attackK, &releaseK, &outputK })
        s->setLookAndFeel (nullptr);
}

void RS2AAudioProcessorEditor::setupKnob (Slider& s, const char*)
{
    s.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    s.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    s.setMouseDragSensitivity (220);
    s.setLookAndFeel (&lnf);
    addAndMakeVisible (s);
}

void RS2AAudioProcessorEditor::timerCallback()
{
    limitLed.setOn (limitRaw->load() > 0.5f);
    noiseLed.setOn (noiseRaw->load() > 0.5f);
    vuLed.setOn (vugrRaw->load() < 0.5f);   // encendido = modo VU
}

void RS2AAudioProcessorEditor::resized()
{
    const float s = (float) getWidth() / kW;
    auto place = [s] (Component& c, float cx, float cy, float w, float h)
    {
        c.setBounds (Rectangle<float> (w * s, h * s).withCentre ({ cx * s, cy * s }).toNearestInt());
    };

    place (peakK,    240.f,  450.f, 200.f, 200.f);
    place (gainK,    583.f,  460.f, 180.f, 180.f);
    place (hpfK,     887.f,  475.f, 116.f, 116.f);
    place (tubeK,   1145.f,  475.f, 118.f, 118.f);
    place (mixK,    1373.f,  475.f, 122.f, 122.f);
    place (attackK,  613.f,  717.f,  90.f,  90.f);
    place (releaseK, 868.f,  717.f,  90.f,  90.f);
    place (outputK, 1316.f,  715.f,  76.f,  76.f);

    place (limitSw, 1303.f, 226.f, 64.f, 100.f);
    place (noiseSw,  365.f, 712.f, 56.f,  90.f);
    place (vuGrSw,  1141.f, 718.f, 56.f,  90.f);
    place (bypassBtn, 141.f, 690.f, 74.f,  74.f);

    place (limitLed, 1352.f, 217.f, 40.f, 40.f);
    place (noiseLed,  410.f, 715.f, 40.f, 40.f);
    place (vuLed,    1093.f, 717.f, 40.f, 40.f);

    place (meter, 493.f + 275.f, 68.f + 87.5f, 550.f, 175.f);
    place (ledMeter, 1399.f, 706.5f, 30.f, 147.f);

    background = Image (Image::ARGB, jmax (1, getWidth()), jmax (1, getHeight()), true);
    Graphics g (background);
    g.addTransform (AffineTransform::scale (s));
    renderBackground (g);
}

void RS2AAudioProcessorEditor::paint (Graphics& g)
{
    g.drawImageAt (background, 0, 0);
}

//==============================================================================
void RS2AAudioProcessorEditor::renderBackground (Graphics& g)
{
    const Colour inkTop (0xff2a2823), inkBot (0xffd2d0c8);

    // ---- panel superior metálico ----
    g.setGradientFill (ColourGradient (Colour (0xffa6a39a), 0.f, 0.f, Colour (0xff8d8b83), 0.f, 612.f, false));
    g.fillRect (0.f, 0.f, kW, 612.f);
    g.setGradientFill (ColourGradient (Colour (0x30ffffff), 770.f, 300.f, Colour (0x00000000), 0.f, 0.f, true));
    g.fillRect (0.f, 0.f, kW, 612.f);
    {
        Random r (7);
        for (int i = 0; i < 60; ++i)   // manchas suaves
        {
            const float x = r.nextFloat() * kW, y = r.nextFloat() * 612.f, rad = 30.f + r.nextFloat() * 120.f;
            g.setGradientFill (ColourGradient (Colour (0x16000000), x, y, Colour (0x00000000), x + rad, y, true));
            g.fillEllipse (x - rad, y - rad, rad * 2.f, rad * 2.f);
        }
    }
    g.setTiledImageFill (noiseTile, 0, 0, 1.f);
    g.fillRect (0.f, 0.f, kW, 612.f);

    // ---- panel inferior negro ----
    g.setGradientFill (ColourGradient (Colour (0xff202020), 0.f, 612.f, Colour (0xff0e0e0e), 0.f, kH, false));
    g.fillRect (0.f, 612.f, kW, kH - 612.f);
    g.setTiledImageFill (noiseTile, 0, 0, 0.6f);
    g.fillRect (0.f, 612.f, kW, kH - 612.f);
    g.setColour (Colour (0xff3d3d3d));
    g.fillRect (0.f, 608.f, kW, 3.f);
    g.setColour (Colours::black);
    g.fillRect (0.f, 611.f, kW, 3.f);
    g.setColour (Colour (0xff333333));
    for (float x : { 251.f, 478.f, 1006.f })
        g.drawLine (x, 640.f, x, 780.f, 1.5f);

    for (auto p : { Point<float> { 33.f, 27.f }, { 1502.f, 27.f }, { 33.f, 771.f }, { 1502.f, 771.f } })
        drawScrew (g, p, 16.f);

    // ---- marco del medidor ----
    {
        const Rectangle<float> fr (447.f, 32.f, 643.f, 240.f);
        g.setColour (Colours::black.withAlpha (0.45f));
        g.fillRoundedRectangle (fr.translated (3.f, 6.f), 16.f);
        g.setGradientFill (ColourGradient (Colour (0xff2a2a2a), fr.getX(), fr.getY(),
                                           Colour (0xff050505), fr.getRight(), fr.getBottom(), false));
        g.fillRoundedRectangle (fr, 16.f);
        g.setColour (Colour (0xff4c4c4c));
        g.drawRoundedRectangle (fr.reduced (1.f), 16.f, 2.f);
        g.setColour (Colours::black);
        g.fillRect (Rectangle<float> (493.f, 68.f, 550.f, 175.f).expanded (7.f));
        for (auto p : { Point<float> { 461.f, 47.f }, { 1075.f, 47.f }, { 461.f, 257.f }, { 1075.f, 257.f } })
            drawScrew (g, p, 8.f);
    }

    // ---- logotipo y modelo ----
    g.setColour (inkTop);
    drawSpaced (g, kBrand, 238.f, 100.f, 74.f, 6.f, true);
    drawSpaced (g, kBrandSub, 240.f, 141.f, 23.f, 13.f, true);
    g.fillRect (122.f, 140.f, 44.f, 2.f);
    g.fillRect (314.f, 140.f, 40.f, 2.f);

    drawSpaced (g, kModel, 1303.f, 96.f, 74.f, 5.f, true);
    drawSpaced (g, "OPTICAL COMPRESSOR", 1303.f, 140.f, 20.f, 5.f, true);
    g.fillRect (1148.f, 139.f, 30.f, 2.f);
    g.fillRect (1430.f, 139.f, 30.f, 2.f);

    // ---- escalas de las perillas ----
    drawScale (g, { 240.f, 450.f }, 100.f, marks100(), 1.38f, 21.f, inkTop);
    drawScale (g, { 583.f, 460.f },  90.f, marks100(), 1.40f, 20.f, inkTop);
    drawScale (g, { 887.f, 475.f },  58.f, marksStops ({ "OFF", "60", "90", "150", "300" }), 1.62f, 19.f, inkTop, false);
    drawScale (g, { 1145.f, 475.f }, 58.f, marksStops ({ "OFF", "1", "2", "3", "4", "5" }), 1.62f, 19.f, inkTop, false);
    drawScale (g, { 1373.f, 475.f }, 60.f, marksMix(), 1.50f, 19.f, inkTop);
    drawScale (g, { 613.f, 717.f },  45.f, marksDots(), 1.f, 0.f, inkBot, false);
    drawScale (g, { 868.f, 717.f },  45.f, marksDots(), 1.f, 0.f, inkBot, false);
    drawScale (g, { 1316.f, 715.f }, 37.f, marksDots(), 1.f, 0.f, inkBot, false);

    // ---- rótulos ----
    g.setColour (inkTop);
    drawSpaced (g, "PEAK REDUCTION", 241.f, 278.f, 22.f, 2.6f);
    drawSpaced (g, "GAIN",           585.f, 310.f, 22.f, 2.6f);
    drawSpaced (g, "SIDECHAIN HPF",  887.f, 349.f, 21.f, 2.4f);
    drawSpaced (g, "TUBE SATURATION", 1141.f, 349.f, 21.f, 2.4f);
    drawSpaced (g, "MIX",            1373.f, 349.f, 21.f, 2.6f);
    drawSpaced (g, "COMPRESS", 1221.f, 213.f, 18.f, 1.8f);
    drawSpaced (g, "LIMIT",    1398.f, 217.f, 18.f, 1.8f);
    g.setFont (makeFont (19.f, true));
    g.drawText ("Hz", Rectangle<float> (60.f, 28.f).withCentre ({ 887.f, 554.f }), Justification::centred, false);
    g.drawText ("%",  Rectangle<float> (60.f, 28.f).withCentre ({ 1373.f, 562.f }), Justification::centred, false);

    g.setColour (inkBot);
    drawSpaced (g, "NOISE",   365.f, 661.f, 18.f, 2.f);
    drawSpaced (g, "ATTACK",  614.f, 642.f, 18.f, 2.f);
    drawSpaced (g, "RELEASE", 870.f, 642.f, 18.f, 2.f);
    drawSpaced (g, "VU / GR", 1141.f, 671.f, 18.f, 2.f);
    drawSpaced (g, "OUTPUT",  1316.f, 643.f, 18.f, 2.f);
    drawSpaced (g, "BYPASS",  141.f, 749.f, 18.f, 2.f);
    g.setFont (makeFont (17.f, true));
    auto lbl = [&] (const String& t, float x, float y)
    {
        g.drawText (t, Rectangle<float> (80.f, 26.f).withCentre ({ x, y }), Justification::centred, false);
    };
    lbl ("SLOW", 551.f, 771.f);  lbl ("FAST", 674.f, 771.f);
    lbl ("SLOW", 806.f, 771.f);  lbl ("FAST", 930.f, 771.f);
    lbl ("OFF", 314.f, 716.f);   lbl ("ON", 410.f, 746.f);
    lbl ("GR", 1191.f, 718.f);
    g.setFont (makeFont (15.f, false));
    static const char* ledLabels[6] = { "+3", "0", "-3", "-6", "-12", "-18" };
    for (int i = 0; i < 6; ++i)
        lbl (ledLabels[i], 1440.f, 647.f + 23.8f * (float) i);
}
