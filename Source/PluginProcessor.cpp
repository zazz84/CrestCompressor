/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
EnvelopeFollower::EnvelopeFollower()
{
}

void EnvelopeFollower::setCoef(float attackTimeMs, float releaseTimeMs)
{
	m_AttackCoef = exp(-1000.0f / (attackTimeMs * m_SampleRate));
	m_ReleaseCoef = exp(-1000.0f / (releaseTimeMs * m_SampleRate));

	m_One_Minus_AttackCoef = 1.0f - m_AttackCoef;
	m_One_Minus_ReleaseCoef = 1.0f - m_ReleaseCoef;
}

float EnvelopeFollower::process(float in)
{
	const float inAbs = fabs(in);
	m_Out1Last = fmaxf(inAbs, m_ReleaseCoef * m_Out1Last + m_One_Minus_ReleaseCoef * inAbs);
	return m_OutLast = m_AttackCoef * m_OutLast + m_One_Minus_AttackCoef * m_Out1Last;
}

const std::string CrestCompressorAudioProcessor::paramsNames[] = { "Attack", "Release", "Ratio", "Threshold", "Mix", "Volume" };
const float CrestCompressorAudioProcessor::CREST_LIMIT = 50.0f;

//==============================================================================
CrestFactor::CrestFactor()
{
}

float CrestFactor::process(float in)
{
	const float inSQ = in * in;
	const float inFactor = (1.0f - m_Coef) * inSQ;

	m_PeakLastSQ = std::max(inSQ, m_Coef * m_PeakLastSQ + inFactor);
	m_RMSLastSQ = m_Coef * m_RMSLastSQ + inFactor;

	return std::sqrtf(m_PeakLastSQ / m_RMSLastSQ);
}
//==============================================================================
CrestCompressorAudioProcessor::CrestCompressorAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
	attackParameter    = apvts.getRawParameterValue(paramsNames[0]);
	releaseParameter   = apvts.getRawParameterValue(paramsNames[1]);
	ratioParameter     = apvts.getRawParameterValue(paramsNames[2]);
	thresholdParameter = apvts.getRawParameterValue(paramsNames[3]);
	mixParameter       = apvts.getRawParameterValue(paramsNames[4]);
	volumeParameter    = apvts.getRawParameterValue(paramsNames[5]);
}

CrestCompressorAudioProcessor::~CrestCompressorAudioProcessor()
{
}

//==============================================================================
const juce::String CrestCompressorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool CrestCompressorAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool CrestCompressorAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool CrestCompressorAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double CrestCompressorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int CrestCompressorAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int CrestCompressorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void CrestCompressorAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String CrestCompressorAudioProcessor::getProgramName (int index)
{
    return {};
}

void CrestCompressorAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void CrestCompressorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
	m_envelopeFollower[0].init((int)(sampleRate));
	m_envelopeFollower[1].init((int)(sampleRate));

	m_crestFactorCalculator[0].init((int)(sampleRate));
	m_crestFactorCalculator[1].init((int)(sampleRate));

	m_crestFactorCalculator[0].setCoef(0.1f);
	m_crestFactorCalculator[1].setCoef(0.1f);
}

void CrestCompressorAudioProcessor::releaseResources()
{
	
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool CrestCompressorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void CrestCompressorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
	// Get params
	const auto attack = attackParameter->load();
	const auto release = releaseParameter->load();
	const auto ratio = ratioParameter->load();	
	const auto threshold = thresholdParameter->load();
	const auto mix = mixParameter->load();
	const auto volume = juce::Decibels::decibelsToGain(volumeParameter->load());

	// Mics constants
	const float attenuationFactor = ratio * 4.0f;
	const float thresholdNormalized = threshold / CREST_LIMIT;
	const float mixInverse = 1.0f - mix;
	const int channels = getTotalNumOutputChannels();
	const int samples = buffer.getNumSamples();		

	for (int channel = 0; channel < channels; ++channel)
	{
		// Channel pointer
		auto* channelBuffer = buffer.getWritePointer(channel);
		
		// Envelope reference
		auto& envelopeFollower = m_envelopeFollower[channel];

		// CrestFactor
		auto& crestFactorCalculator = m_crestFactorCalculator[channel];
		
		// Set attack and release
		envelopeFollower.setCoef(attack, release);

		const float factor = (ratio > 0.0f) ? -1.0f : 1.0f;

		for (int sample = 0; sample < samples; ++sample)
		{
			// Get input
			const float in = channelBuffer[sample];

			// Get crest factor
			const float crestFactor = crestFactorCalculator.process(in);
			const float crestFactorNormalized = std::min(crestFactor / CREST_LIMIT, 1.0f);
			const float crestSkewed = powf(crestFactorNormalized, 0.5f);
			
			//Get gain reduction, positive values
			const float attenuatedB = (crestSkewed >= thresholdNormalized) ? (crestSkewed - thresholdNormalized) * attenuationFactor : 0.0f;

			// Smooth
			const float smoothdB = factor * envelopeFollower.process(attenuatedB);

#ifdef DEBUG
			if (crestSkewed * CREST_LIMIT > m_crestFactorPercentage)
				m_crestFactorPercentage = crestSkewed * CREST_LIMIT;

			// Store gain reduction
			if (fabsf(smoothdB) > fabsf(m_gainReductiondB))
				m_gainReductiondB = smoothdB;
#endif

			// Apply gain reduction
			const float out = in * juce::Decibels::decibelsToGain(smoothdB);

			// Apply volume, mix and send to output
			channelBuffer[sample] = volume * (mix * out + mixInverse * in);
		}
	}
}

//==============================================================================
bool CrestCompressorAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* CrestCompressorAudioProcessor::createEditor()
{
    return new CrestCompressorAudioProcessorEditor (*this, apvts);
}

//==============================================================================
void CrestCompressorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{	
	auto state = apvts.copyState();
	std::unique_ptr<juce::XmlElement> xml(state.createXml());
	copyXmlToBinary(*xml, destData);
}

void CrestCompressorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
	std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

	if (xmlState.get() != nullptr)
		if (xmlState->hasTagName(apvts.state.getType()))
			apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessorValueTreeState::ParameterLayout CrestCompressorAudioProcessor::createParameterLayout()
{
	APVTS::ParameterLayout layout;

	using namespace juce;

	layout.add(std::make_unique<juce::AudioParameterFloat>(paramsNames[0], paramsNames[0], NormalisableRange<float>( 0.01f, 200.0f, 0.01f, 0.5f),  10.0f));
	layout.add(std::make_unique<juce::AudioParameterFloat>(paramsNames[1], paramsNames[1], NormalisableRange<float>( 0.01f, 200.0f, 0.01f, 0.5f), 100.0f));
	layout.add(std::make_unique<juce::AudioParameterFloat>(paramsNames[2], paramsNames[2], NormalisableRange<float>(-24.0f,  24.0f,  1.0f, 1.0f),   0.0f));
	layout.add(std::make_unique<juce::AudioParameterFloat>(paramsNames[3], paramsNames[3], NormalisableRange<float>(  0.0f, CREST_LIMIT,  1.0f, 1.0f), CREST_LIMIT * 0.5f));
	layout.add(std::make_unique<juce::AudioParameterFloat>(paramsNames[4], paramsNames[4], NormalisableRange<float>(  0.0f,   1.0f, 0.05f, 1.0f),   1.0f));
	layout.add(std::make_unique<juce::AudioParameterFloat>(paramsNames[5], paramsNames[5], NormalisableRange<float>(-24.0f,  24.0f,  0.1f, 1.0f),   0.0f));

	return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CrestCompressorAudioProcessor();
}
