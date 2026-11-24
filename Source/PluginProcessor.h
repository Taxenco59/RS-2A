#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include "OptoCompressorCore.h"

namespace ParamIDs
{
    inline constexpr const char* peak    = "peak";
    inline constexpr const char* gain    = "gain";
    inline constexpr const char* hpf     = "hpf";
    inline constexpr const char* tube    = "tube";
    inline constexpr const char* mix     = "mix";
    inline constexpr const char* limit   = "limit";
    inline constexpr const char* noise   = "noise";
    inline constexpr const char* attack  = "attack";
    inline constexpr const char* release = "release";
    inline constexpr const char* vugr    = "vugr";
    inline constexpr const char* output  = "output";
    inline constexpr const char* bypass  = "bypass";
}

class RS2AAudioProcessor : public juce::AudioProcessor
{
public:
    RS2AAudioProcessor();
    ~RS2AAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "RS-2A"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Medidores (los lee la interfaz)
    std::atomic<float> vuDb      { -60.f };  // nivel de salida en VU (0 VU = -18 dBFS RMS)
    std::atomic<float> grDb      { 0.f };    // reducción de ganancia en dB
    std::atomic<float> outPeakLin { 0.f };   // pico de salida desde la última lectura

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    static constexpr int kChunk = 512;
    static constexpr int kRing  = 4096;
    static constexpr int kRingMask = kRing - 1;

    struct Cfg
    {
        float tube = 0.f;
        bool  noiseOn = false;
        float noiseGain = 0.f;
    };

    void processChunk (float** ch, int len, int numCh, const Cfg& cfg, float& blockPeak, double& blockPow);

    rs2a::OptoCore core;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;

    std::array<std::array<float, kRing>, 2> dryRing {};
    int ringPos = 0;
    int latency = 0;
    juce::AudioBuffer<float> dryTemp { 2, kChunk };

    juce::SmoothedValue<float> makeupSm, mixSm, outSm, bypSm, noiseSm;

    float dcX1[2] { 0.f, 0.f }, dcY1[2] { 0.f, 0.f };
    float dcR = 0.999f;
    float noiseLp[2] { 0.f, 0.f };
    juce::Random rng;

    float vuPow = 0.f, aVu = 0.f;
    double sr = 44100.0;

    std::atomic<float> *peakP, *gainP, *hpfP, *tubeP, *mixP, *limitP, *noiseP,
                       *attackP, *releaseP, *vugrP, *outputP, *bypassP;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RS2AAudioProcessor)
};
