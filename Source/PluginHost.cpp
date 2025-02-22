/*
	==============================================================================

	PluginHost
	by Daniel Rothmann
	This is a tiny audio plugin host which can take care of plugin instantiation
	as well as processing blocks with plugin and setting parameters.

	==============================================================================
*/

#include "PluginHost.h"

/**
*Constructs a host for a single plugin.
*/
PluginHost::PluginHost()
{
	formatManager = new AudioPluginFormatManager;
	formatManager->addDefaultFormats();
	audioData = new AudioBuffer<float>(1, 512);
	midiData = new MidiBuffer();
}

PluginHost::~PluginHost()
{
}

/**
 *Instantiates a plugin from an XML Document representing a JUCE PluginDescription.
 *@param xmlPluginDescription A string containing the XML Document which can be read as a PluginDescription.
 *@param sampleRate The sample rate to initialize plugin with.
 *@param bufferSize The buffer size to initialize plugin with.
 *@return A boolean representing wether instantiation was successful or not.
*/
bool PluginHost::instantiatePlugin(char* xmlPluginDescription, double sampleRate, int bufferSize)
{
	if (!pluginInstantiated)
	{
		pluginDescription = new PluginDescription();
		ScopedPointer<XmlDocument> document = new XmlDocument(xmlPluginDescription);
		XmlElement* element = document->getDocumentElement();
		
		if (pluginDescription->loadFromXml(*element))
		{
			String error = "Could not create plugin instance.";
			pluginInstance = formatManager->createPluginInstance(*pluginDescription, sampleRate, bufferSize, error);

			if (pluginInstance)
			{
				pluginEditor = pluginInstance->createEditor();
				pluginInstantiated = true;
				return true;
			}
		}
	}

	return false;
}

/**
*Prepares the plugin instance for playback.
*@param sampleRate The sample rate to initialize plugin with.
*@param expectedSamplesPerBlock The maximum buffer size to expect.
*/
void PluginHost::prepareToPlay(double sampleRate, int expectedSamplesPerBlock)
{
	if (pluginInstantiated)
	{
		pluginInstance->setRateAndBufferSizeDetails(sampleRate, expectedSamplesPerBlock);
		pluginInstance->prepareToPlay(sampleRate, expectedSamplesPerBlock);
	}
}

/**
 *Suspends the plugin processing.
 *@param shouldBeSuspended Wether the processing should be suspended or not.
 *@return A boolean representing wether suspension change was successful.
 */
bool PluginHost::suspendPlugin(bool shouldBeSuspended)
{
	if (pluginInstantiated)
	{
		pluginInstance->suspendProcessing(shouldBeSuspended);
		return true;
	}
	return false;
}

/**
*Releases and deletes the plugin instance.
*@return A boolean representing wether the instance was successfully released.
*/
bool PluginHost::releasePlugin()
{
	if (pluginInstantiated)
	{
		// TODO: Might need to manually delete editor
		delete pluginInstance;
		pluginInstantiated = false;
		return true;
	}
	return false;
}

/**
*Processes a block of audio with plugin. If the plugin is not instantiated, the buffer won't be changed.
*@param buffer A buffer of floats containing audio data to be processed.
*@param bufferLength The length of the buffer in samples.
*@param numChannels The total number of channels contained in buffer.
*/
void PluginHost::processBlock(float* buffer, int bufferLength, int numChannels)
{
	if (pluginInstantiated)
	{
		audioData = new AudioBuffer<float>(&buffer, numChannels, bufferLength);
		//audioData->setDataToReferTo(&buffer, numChannels, bufferLength);
		pluginInstance->processBlock(*audioData, *midiData);
	}
}

/**
*Gets the total number of inputs on plugin.
*@return The number of input channels. Returns 0 if plugin is not instantiated.
*/
int PluginHost::getNumInputs()
{
	if (pluginInstantiated)
		return pluginInstance->getTotalNumInputChannels();
	else
		return 0;
}

/**
*Gets the total number of outputs on plugin.
*@return The number of output channels. Returns 0 if plugin is not instantiated.
*/
int PluginHost::getNumOutputs()
{
	if (pluginInstantiated)
		return pluginInstance->getTotalNumOutputChannels();
	else
		return 0;
}

/**
*Gets the total number of parameters on plugin.
*@return The number of parameters. Returns 0 if plugin is not instantiated.
*/
int PluginHost::getNumParameters()
{
	if (pluginInstantiated)
	{
		return pluginInstance->getNumParameters();
	}
	else
		return 0;
}

/**
*Gets the total number of managed parameters (defined by plugin) on plugin.
*@return The number of parameters. Returns 0 if plugin is not instantiated or no managed parameters exist.
*/
int PluginHost::getNumNamedParameters()
{
	if (pluginInstantiated)
	{
		for (int i = 0; i < getNumParameters(); i++)
		{
			if (getParameterName(i) == "Bypass" || getParameterName(i) == "")
			{
				return i;
			}
		}
	}
	
	return 0;
}

/**
*Gets the name of a plugin parameter at a given index.
*@param index The parameter index from which to get name.
*@return The name of the parameter as const char*. If plugin or parameter does not exist, nullptr is returned.
*/
String PluginHost::getParameterName(int index)
{
	if (pluginInstantiated)
	{
		if (index >= 0 && index < getNumParameters())
			return pluginInstance->getParameterName(index);
	}

	return "";
}

/**
*Gets the value of a parameter with a given name.
*@param name The name of the parameter to get value of.
*@return The value of the parameter as float. If plugin or parameter does not exist, 0f is returned.
*/
float PluginHost::getValueByName(char* name)
{
	if (pluginInstantiated)
	{
		String nameToFind = String(name);
		for (int i = 0; i < getNumNamedParameters(); i++)
		{
			if (getParameterName(i) == nameToFind)
			{
				return getValueByIndex(i);
			}
		}
	}

	return 0.0f;
}

/**
*Sets the value of a parameter with a given name.
*@param name The name of the parameter whose value to set.
*@param value The value to set.
*@return Wether setting value succeeded. If plugin or parameter does not exist, false is returned.
*/
bool PluginHost::setValueByName(char* name, float value)
{
	if (pluginInstantiated)
	{
		String nameToFind = String(name);
		for (int i = 0; i < getNumNamedParameters(); i++)
		{
			if (getParameterName(i) == nameToFind)
			{
				setValueByIndex(i, value);
				return true;
			}
		}
	}
	return false;
}

/**
*Gets the value of a parameter with a given index.
*@param index The index of the parameter to get value of.
*@return The value of the parameter as float. If plugin or parameter does not exist, 0f is returned.
*/
float PluginHost::getValueByIndex(int index)
{
	if (pluginInstantiated)
	{
		if (index >= 0 && index < getNumParameters())
		{
			return pluginInstance->getParameter(index);
		}
	}

	return 0.0f;
}

/**
*Sets the value of a parameter with a given index.
*@param index The index of the parameter to whose value to set.
*@param value The value to set.
*@return Wether setting value succeeded. If plugin or parameter does not exist, false is returned.
*/
bool PluginHost::setValueByIndex(int index, float value)
{
	if (pluginInstantiated)
	{
		if (index >= 0 && index < getNumParameters())
		{
			pluginInstance->setParameter(index, value);
			return true;
		}
	}

	return false;
}

/**
*Gets a reference to a parameter with a given name.
*@param name The name of the parameter to get.
*@return A reference to the AudioProcessorParameter. If parameter does not exist, nullptr is returned.
*/
AudioProcessorParameter* PluginHost::getParameterByName(char* name)
{
	ScopedPointer<String> paramName = new String(name);
	if (paramName->isNotEmpty())
	{
		const auto &params = pluginInstance->getParameters();

		for (auto param : params)
		{
			if (param->getName(maxNameLength).compare(*paramName))
			{
				return param;
			}
		}
	}

	return nullptr;
}
