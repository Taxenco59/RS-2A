#pragma once
#include "PluginProcessor.h"
#include "GuiComponents.h"

class RS2AAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit RS2AAudioProcessorEditor (RS2AAudioProcessor&);
    ~RS2AAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setupKnob (juce::Slider& s, const char* paramId);
    void renderBackground (juce::Graphics& g);

    RS2AAudioProcessor& proc;
    rs2a::gui::KnobLookAndFeel lnf;

    juce::Slider peakK, gainK, hpfK, tubeK, mixK, attackK, releaseK, outputK;
    rs2a::gui::BatSwitch limitSw, noiseSw, vuGrSw;
    rs2a::gui::BypassButton bypassBtn;
    rs2a::gui::Led limitLed, noiseLed, vuLed;
    rs2a::gui::MeterView meter;
    rs2a::gui::LedMeter ledMeter;

    juce::Image background, noiseTile;
    juce::ComponentBoundsConstrainer constrainer;

    std::atomic<float>* limitRaw;
    std::atomic<float>* noiseRaw;
    std::atomic<float>* vugrRaw;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SA> peakA, gainA, hpfA, tubeA, mixA, attackA, releaseA, outputA;
    std::unique_ptr<BA> limitA, noiseA, vugrA, bypassA;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RS2AAudioProcessorEditor)
};
