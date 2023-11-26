/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
#if DEBUG
class CrestCompressorAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::Timer
#else
class CrestCompressorAudioProcessorEditor : public juce::AudioProcessorEditor
#endif
{
public:
    CrestCompressorAudioProcessorEditor (CrestCompressorAudioProcessor&, juce::AudioProcessorValueTreeState&);
    ~CrestCompressorAudioProcessorEditor() override;

	// GUI setup
	static const int N_SLIDERS_COUNT = 6;
	static const int SCALE = 70;

#ifdef DEBUG
	static const int MENU_HEIGHT = 60;
#endif

    //==============================================================================
#ifdef DEBUG
	void timerCallback() override;
#endif
	void paint (juce::Graphics&) override;
    void resized() override;

	typedef juce::AudioProcessorValueTreeState::SliderAttachment SliderAttachment;
	typedef juce::AudioProcessorValueTreeState::ComboBoxAttachment ComboBoxAttachment;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    CrestCompressorAudioProcessor& audioProcessor;

	juce::AudioProcessorValueTreeState& valueTreeState;

	juce::Label m_labels[N_SLIDERS_COUNT] = {};
	juce::Slider m_sliders[N_SLIDERS_COUNT] = {};
	std::unique_ptr<SliderAttachment> m_sliderAttachment[N_SLIDERS_COUNT] = {};

	juce::Label automationTLabel;
	juce::Label smoothingTypeLabel;
	juce::Label detectionTypeLabel;

#ifdef DEBUG
	juce::Label gainReductionLabel;
	juce::Label crestFactorLabel;	
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CrestCompressorAudioProcessorEditor)
};
