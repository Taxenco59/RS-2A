#pragma once
// Núcleo DSP del RS-2A: emulación de compresor óptico tipo LA-2A (T4 cell).
// Sin dependencias de JUCE: solo la librería estándar.
#include <algorithm>
#include <cmath>

namespace rs2a
{
class OptoCore
{
public:
    void prepare (double sampleRate)
    {
        fs       = sampleRate;
        aDet     = coef (0.010);   // detector RMS ~10 ms
        aAtkSlow = coef (0.35);    // memoria lenta de la celda
        hpfIdx   = -1;
        reset();
        setParams (50.f, false, 50.f, 50.f, 0);
    }

    void reset()
    {
        for (int c = 0; c < 2; ++c) { z1[c] = 0.f; z2[c] = 0.f; }
        detPow = fastGr = slowGr = grDb = 0.f;
    }

    // peak, attack, release: 0..100.  hpfIndex: 0=OFF 1=60 2=90 3=150 4=300 Hz
    void setParams (float peak, bool limit, float attack, float release, int hpfIndex)
    {
        thresholdDb = 6.f - peak * 0.55f;       // PEAK REDUCTION = ganancia del sidechain
        slope       = limit ? 0.90f : 0.667f;   // ~10:1 (limit)  /  ~3:1 (compress)
        kneeDb      = limit ? 6.f : 12.f;

        const float a = attack  * 0.01f;
        const float r = release * 0.01f;
        const float atkMs     = 40.f   * std::pow (0.05f, a);   // 40 ms (slow) .. 2 ms (fast)
        const float relFastMs = 150.f  * std::pow (0.10f, r);   // 150 .. 15 ms
        const float relSlowMs = 2500.f * std::pow (0.20f, r);   // 2.5 s .. 0.5 s
        aAtkFast = coef (atkMs * 0.001);
        aRelFast = coef (relFastMs * 0.001);
        aRelSlow = coef (relSlowMs * 0.001);

        if (hpfIndex != hpfIdx)
        {
            hpfIdx = hpfIndex;
            static const float freqs[5] = { 0.f, 60.f, 90.f, 150.f, 300.f };
            hpfOn = hpfIdx > 0;
            if (hpfOn)
            {
                const double w0 = 2.0 * 3.14159265358979323846 * freqs[hpfIdx] / fs;
                const double cw = std::cos (w0);
                const double alpha = std::sin (w0) / (2.0 * 0.70710678);
                const double a0 = 1.0 + alpha;
                b0 = (float) (((1.0 + cw) * 0.5) / a0);
                b1 = (float) (-(1.0 + cw) / a0);
                b2 = b0;
                a1 = (float) ((-2.0 * cw) / a0);
                a2 = (float) ((1.0 - alpha) / a0);
            }
        }
    }

    // Devuelve la ganancia lineal (<= 1) a aplicar a ambos canales (enlace estéreo).
    float processSample (float l, float r)
    {
        float sl = l, sr = r;
        if (hpfOn) { sl = hp (0, l); sr = hp (1, r); }

        const float pw = std::max (sl * sl, sr * sr);
        detPow += aDet * (pw - detPow);
        const float levelDb = 10.f * std::log10 (detPow + 1.0e-10f);

        const float over = levelDb - thresholdDb;
        float target = 0.f;
        if (2.f * over > -kneeDb)
        {
            if (2.f * over < kneeDb)
            {
                const float t = over + kneeDb * 0.5f;
                target = slope * t * t / (2.f * kneeDb);
            }
            else
                target = slope * over;
        }

        // Celda óptica: etapa rápida + etapa lenta con "memoria".
        fastGr += (target > fastGr ? aAtkFast : aRelFast) * (target - fastGr);
        slowGr += (target > slowGr ? aAtkSlow : aRelSlow) * (target - slowGr);
        grDb = std::max (fastGr, slowGr * 0.7f);

        return std::pow (10.f, -grDb * 0.05f);
    }

    float getGrDb() const { return grDb; }

private:
    float coef (double tau) const { return (float) (1.0 - std::exp (-1.0 / (tau * fs))); }

    float hp (int c, float x)
    {
        const float y = b0 * x + z1[c];
        z1[c] = b1 * x - a1 * y + z2[c];
        z2[c] = b2 * x - a2 * y;
        return y;
    }

    double fs = 44100.0;
    float aDet = 0.f, aAtkFast = 0.f, aRelFast = 0.f, aAtkSlow = 0.f, aRelSlow = 0.f;
    float thresholdDb = -20.f, slope = 0.667f, kneeDb = 12.f;
    float detPow = 0.f, fastGr = 0.f, slowGr = 0.f, grDb = 0.f;

    int  hpfIdx = -1;
    bool hpfOn  = false;
    float b0 = 1.f, b1 = 0.f, b2 = 0.f, a1 = 0.f, a2 = 0.f;
    float z1[2] = { 0.f, 0.f }, z2[2] = { 0.f, 0.f };
};
} // namespace rs2a
