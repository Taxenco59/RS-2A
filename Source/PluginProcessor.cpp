#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

AudioProcessorValueTreeState::ParameterLayout RS2AAudioProcessor::createLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto pct = [] (const char* id, const char* name, float def)
    {
        return std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name,
                                                      NormalisableRange<float> (0.f, 100.f, 0.1f), def);
    };

    p.push_back (pct (ParamIDs::peak,    "Peak Reduction", 50.f));
    p.push_back (pct (ParamIDs::gain,    "Gain",           50.f));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::hpf, 1 }, "Sidechain HPF",
                     StringArray { "Off", "60 Hz", "90 Hz", "150 Hz", "300 Hz" }, 2));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::tube, 1 }, "Tube Saturation",
                     StringArray { "Off", "1", "2", "3", "4", "5" }, 2));
    p.push_back (pct (ParamIDs::mix,     "Mix",            100.f));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::limit, 1 }, "Compress / Limit", false));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::noise, 1 }, "Noise", false));
    p.push_back (pct (ParamIDs::attack,  "Attack",         50.f));
    p.push_back (pct (ParamIDs::release, "Release",        50.f));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::vugr, 1 }, "VU / GR", false));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::output, 1 }, "Output",
                     NormalisableRange<float> (-18.f, 18.f, 0.1f), 0.f));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::bypass, 1 }, "Bypass", false));

    return { p.begin(), p.end() };
}

RS2AAudioProcessor::RS2AAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  AudioChannelSet::stereo(), true)
                          .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    peakP    = apvts.getRawParameterValue (ParamIDs::peak);
    gainP    = apvts.getRawParameterValue (ParamIDs::gain);
    hpfP     = apvts.getRawParameterValue (ParamIDs::hpf);
    tubeP    = apvts.getRawParameterValue (ParamIDs::tube);
    mixP     = apvts.getRawParameterValue (ParamIDs::mix);
    limitP   = apvts.getRawParameterValue (ParamIDs::limit);
    noiseP   = apvts.getRawParameterValue (ParamIDs::noise);
    attackP  = apvts.getRawParameterValue (ParamIDs::attack);
    releaseP = apvts.getRawParameterValue (ParamIDs::release);
    vugrP    = apvts.getRawParameterValue (ParamIDs::vugr);
    outputP  = apvts.getRawParameterValue (ParamIDs::output);
    bypassP  = apvts.getRawParameterValue (ParamIDs::bypass);

    oversampling = std::make_unique<dsp::Oversampling<float>> (
        2, 1, dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
}

bool RS2AAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    if (in != out) return false;
    return out == AudioChannelSet::mono() || out == AudioChannelSet::stereo();
}

void RS2AAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    core.prepare (sr);

    oversampling->initProcessing ((size_t) kChunk);
    oversampling->reset();
    latency = jlimit (0, kRing / 2, (int) std::lround (oversampling->getLatencyInSamples()));
    setLatencySamples (latency);

    for (auto& r : dryRing) r.fill (0.f);
    ringPos = 0;
    dryTemp.clear();

    for (auto* s : { &makeupSm, &mixSm, &outSm, &bypSm, &noiseSm })
        s->reset (sr, 0.02);

    makeupSm.setCurrentAndTargetValue (Decibels::decibelsToGain ((gainP->load() - 30.f) * 0.4f));
    mixSm.setCurrentAndTargetValue (mixP->load() * 0.01f);
    outSm.setCurrentAndTargetValue (Decibels::decibelsToGain (outputP->load()));
    bypSm.setCurrentAndTargetValue (bypassP->load() > 0.5f ? 1.f : 0.f);
    noiseSm.setCurrentAndTargetValue (noiseP->load() > 0.5f ? 1.f : 0.f);

    dcR = 1.f - (float) (2.0 * MathConstants<double>::pi * 8.0 / sr);
    for (int c = 0; c < 2; ++c) { dcX1[c] = dcY1[c] = 0.f; noiseLp[c] = 0.f; }

    aVu   = (float) (1.0 - std::exp (-1.0 / (0.300 * sr)));   // integración VU ~300 ms
    vuPow = 0.f;
}

void RS2AAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, n);

    const int numCh = jmin (2, buffer.getNumChannels());
    if (numCh < 1 || n < 1) return;

    // --- parámetros ---
    const float peak    = peakP->load();
    const float gainV   = gainP->load();
    const int   hpfIdx  = (int) std::lround (hpfP->load());
    const int   tubeIdx = (int) std::lround (tubeP->load());
    const float mixV    = mixP->load() * 0.01f;
    const bool  limit   = limitP->load() > 0.5f;
    const bool  noiseOn = noiseP->load() > 0.5f;
    const bool  grMode  = vugrP->load() > 0.5f;
    (void) grMode;

    core.setParams (peak, limit, attackP->load(), releaseP->load(), hpfIdx);

    makeupSm.setTargetValue (Decibels::decibelsToGain ((gainV - 30.f) * 0.4f)); // -12 .. +28 dB
    mixSm.setTargetValue (mixV);
    outSm.setTargetValue (Decibels::decibelsToGain (outputP->load()));
    bypSm.setTargetValue (bypassP->load() > 0.5f ? 1.f : 0.f);
    noiseSm.setTargetValue (noiseOn ? 1.f : 0.f);

    Cfg cfg;
    cfg.tube      = (float) tubeIdx;
    cfg.noiseOn   = noiseOn;
    cfg.noiseGain = Decibels::decibelsToGain (-74.f);

    float blockPeak = 0.f;
    double blockPow = 0.0;

    for (int start = 0; start < n; start += kChunk)
    {
        const int len = jmin (kChunk, n - start);
        float* ptrs[2] = { buffer.getWritePointer (0) + start,
                           numCh > 1 ? buffer.getWritePointer (1) + start : nullptr };
        processChunk (ptrs, len, numCh, cfg, blockPeak, blockPow);
    }

    // --- medidores ---
    outPeakLin.store (jmax (outPeakLin.load(), blockPeak));
    const float vu = 10.f * std::log10 (vuPow + 1.0e-10f) + 18.f;
    vuDb.store (vu);
    grDb.store (bypassP->load() > 0.5f ? 0.f : core.getGrDb());
}

void RS2AAudioProcessor::processChunk (float** ch, int len, int numCh, const Cfg& cfg,
                                       float& blockPeak, double& blockPow)
{
    // 1) Compresor óptico (detector sobre la entrada) + copia retardada del dry
    float* dry[2] = { dryTemp.getWritePointer (0), dryTemp.getWritePointer (1) };

    for (int i = 0; i < len; ++i)
    {
        float in[2] = { ch[0][i], numCh > 1 ? ch[1][i] : ch[0][i] };

        for (int c = 0; c < numCh; ++c)
        {
            dryRing[(size_t) c][(size_t) ringPos] = in[c];
            dry[c][i] = dryRing[(size_t) c][(size_t) ((ringPos - latency) & kRingMask)];
        }
        ringPos = (ringPos + 1) & kRingMask;

        const float g = core.processSample (in[0], in[1]);
        for (int c = 0; c < numCh; ++c)
            ch[c][i] = in[c] * g;
    }

    // 2) Saturación valvular con sobremuestreo 2x
    dsp::AudioBlock<float> blk (ch, (size_t) numCh, (size_t) len);
    auto up = oversampling->processSamplesUp (blk);

    if (cfg.tube > 0.f)
    {
        const float d    = 1.f + cfg.tube * 1.1f;
        const float b    = 0.06f * cfg.tube;            // asimetría -> armónicos pares
        const float tb   = std::tanh (b);
        const float norm = 1.f / (d * (1.f - tb * tb)); // ganancia unitaria en señal pequeña

        for (size_t c = 0; c < up.getNumChannels(); ++c)
        {
            float* x = up.getChannelPointer (c);
            for (size_t k = 0; k < up.getNumSamples(); ++k)
                x[k] = (std::tanh (d * x[k] + b) - tb) * norm;
        }
    }
    oversampling->processSamplesDown (blk);

    // 3) Etapa de salida: makeup, ruido, mezcla paralela, nivel de salida, bypass
    for (int i = 0; i < len; ++i)
    {
        const float makeup = makeupSm.getNextValue();
        const float mixV   = mixSm.getNextValue();
        const float outG   = outSm.getNextValue();
        const float byp    = bypSm.getNextValue();
        const float nzG    = noiseSm.getNextValue() * cfg.noiseGain;

        float sumSq = 0.f;
        for (int c = 0; c < numCh; ++c)
        {
            float x = ch[c][i];

            // bloqueo de DC (la saturación asimétrica genera algo)
            const float y = x - dcX1[c] + dcR * dcY1[c];
            dcX1[c] = x;
            dcY1[c] = y;

            float wet = y * makeup;

            if (nzG > 0.f)
            {
                const float w = (rng.nextFloat() * 2.f - 1.f);
                noiseLp[c] += 0.35f * (w - noiseLp[c]);
                wet += noiseLp[c] * nzG;
            }

            const float d = dry[c][i];
            const float processed = (d * (1.f - mixV) + wet * mixV) * outG;
            const float out = processed + (d - processed) * byp;

            ch[c][i] = out;
            blockPeak = jmax (blockPeak, std::abs (out));
            sumSq += out * out;
        }
        const float pw = sumSq / (float) numCh;
        vuPow += aVu * (pw - vuPow);
        blockPow += pw;
    }
}

AudioProcessorEditor* RS2AAudioProcessor::createEditor()
{
    return new RS2AAudioProcessorEditor (*this);
}

void RS2AAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void RS2AAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (ValueTree::fromXml (*xml));
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RS2AAudioProcessor();
}
