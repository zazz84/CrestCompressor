/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
CrestCompressorAudioProcessorEditor::CrestCompressorAudioProcessorEditor (CrestCompressorAudioProcessor& p, juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor (&p), audioProcessor (p), valueTreeState(vts)
{
	juce::Colour light = juce::Colour::fromHSV(HUE * 0.01f, 0.5f, 0.6f, 1.0f);
	juce::Colour medium = juce::Colour::fromHSV(HUE * 0.01f, 0.5f, 0.5f, 1.0f);
	juce::Colour dark = juce::Colour::fromHSV(HUE * 0.01f, 0.5f, 0.4f, 1.0f);

	getLookAndFeel().setColour(juce::Slider::thumbColourId, dark);
	getLookAndFeel().setColour(juce::Slider::rotarySliderFillColourId, medium);
	getLookAndFeel().setColour(juce::Slider::rotarySliderOutlineColourId, light);

	for (int i = 0; i < N_SLIDERS_COUNT; i++)
	{
		auto& label = m_labels[i];
		auto& slider = m_sliders[i];

		//Lable
		label.setText(CrestCompressorAudioProcessor::paramsNames[i], juce::dontSendNotification);
		label.setFont(juce::Font(24.0f * 0.01f * SCALE, juce::Font::bold));
		label.setJustificationType(juce::Justification::centred);
		addAndMakeVisible(label);

		//Slider
		slider.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
		slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
		addAndMakeVisible(slider);
		m_sliderAttachment[i].reset(new SliderAttachment(valueTreeState, CrestCompressorAudioProcessor::paramsNames[i], slider));
	}

#if DEBUG
	crestFactorLabel.setText("Crest: 0", juce::dontSendNotification);
	crestFactorLabel.setFont(juce::Font(24.0f * 0.01f * SCALE, juce::Font::bold));
	crestFactorLabel.setJustificationType(juce::Justification::left);
	addAndMakeVisible(crestFactorLabel);

	gainReductionLabel.setText("GR: 0", juce::dontSendNotification);
	gainReductionLabel.setFont(juce::Font(24.0f * 0.01f * SCALE, juce::Font::bold));
	gainReductionLabel.setJustificationType(juce::Justification::left);
	addAndMakeVisible(gainReductionLabel);
#endif

#if DEBUG
	setSize((int)(SLIDER_WIDTH * 0.01f * SCALE * N_SLIDERS_COUNT), (int)(SLIDER_WIDTH * 0.01f * SCALE) + MENU_HEIGHT);

	startTimerHz(8);
#else
	setSize((int)(SLIDER_WIDTH * 0.01f * SCALE * N_SLIDERS_COUNT), (int)(SLIDER_WIDTH * 0.01f * SCALE));
#endif
}

CrestCompressorAudioProcessorEditor::~CrestCompressorAudioProcessorEditor()
{
}

//==============================================================================
#ifdef DEBUG
void CrestCompressorAudioProcessorEditor::timerCallback()
{
	const int crestFactorSQ = (int)audioProcessor.getCrestFactor();
	crestFactorLabel.setText("Crest: " + juce::String(crestFactorSQ), juce::dontSendNotification);

	const int gainReduction = (int)audioProcessor.getGainReduction();
	gainReductionLabel.setText("GR: " + juce::String(gainReduction), juce::dontSendNotification);
	
	repaint();
}
#endif

void CrestCompressorAudioProcessorEditor::paint (juce::Graphics& g)
{
	g.fillAll(juce::Colour::fromHSV(HUE * 0.01f, 0.5f, 0.7f, 1.0f));
}

void CrestCompressorAudioProcessorEditor::resized()
{
	int width = getWidth() / N_SLIDERS_COUNT;

#if DEBUG
	int height = getHeight() - MENU_HEIGHT;
#else
	int height = getHeight();
#endif
	
	// Sliders + Menus
	juce::Rectangle<int> rectangles[N_SLIDERS_COUNT];

	for (int i = 0; i < N_SLIDERS_COUNT; ++i)
	{
		rectangles[i].setSize(width, height);
		rectangles[i].setPosition(i * width, 0);
		m_sliders[i].setBounds(rectangles[i]);

		rectangles[i].removeFromBottom((int)(20.0f * 0.01f * SCALE));
		m_labels[i].setBounds(rectangles[i]);
	}

#if DEBUG
	// Debug menus
	juce::Rectangle<int> debugMenuRectangle;
	const int menuWidth = (int)(width * 0.9f);
	const int debugMenuPosY = (int)(height + MENU_HEIGHT * 0.3f);
	debugMenuRectangle.setSize(menuWidth, (int)(MENU_HEIGHT * 0.4f));

	//1
	debugMenuRectangle.setPosition((int)(0.05f * width), debugMenuPosY);

	//2
	debugMenuRectangle.setPosition((int)(1.05f * width), debugMenuPosY);

	//3
	debugMenuRectangle.setPosition((int)(2.05f * width), debugMenuPosY);
	gainReductionLabel.setBounds(debugMenuRectangle);

	//4
	debugMenuRectangle.setPosition((int)(3.05f * width), debugMenuPosY);
	crestFactorLabel.setBounds(debugMenuRectangle);

	//5
	debugMenuRectangle.setPosition((int)(4.05f * width), debugMenuPosY);

	//6
#endif
}