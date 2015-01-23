
#include "TuningDifference.h"


TuningDifference::TuningDifference(float inputSampleRate) :
    Plugin(inputSampleRate)
{
}

TuningDifference::~TuningDifference()
{
}

string
TuningDifference::getIdentifier() const
{
    return "myplugin";
}

string
TuningDifference::getName() const
{
    return "My Plugin";
}

string
TuningDifference::getDescription() const
{
    // Return something helpful here!
    return "";
}

string
TuningDifference::getMaker() const
{
    // Your name here
    return "";
}

int
TuningDifference::getPluginVersion() const
{
    // Increment this each time you release a version that behaves
    // differently from the previous one
    return 1;
}

string
TuningDifference::getCopyright() const
{
    // This function is not ideally named.  It does not necessarily
    // need to say who made the plugin -- getMaker does that -- but it
    // should indicate the terms under which it is distributed.  For
    // example, "Copyright (year). All Rights Reserved", or "GPL"
    return "";
}

TuningDifference::InputDomain
TuningDifference::getInputDomain() const
{
    return TimeDomain;
}

size_t
TuningDifference::getPreferredBlockSize() const
{
    return 0; // 0 means "I can handle any block size"
}

size_t 
TuningDifference::getPreferredStepSize() const
{
    return 0; // 0 means "anything sensible"; in practice this
              // means the same as the block size for TimeDomain
              // plugins, or half of it for FrequencyDomain plugins
}

size_t
TuningDifference::getMinChannelCount() const
{
    return 1;
}

size_t
TuningDifference::getMaxChannelCount() const
{
    return 1;
}

TuningDifference::ParameterList
TuningDifference::getParameterDescriptors() const
{
    ParameterList list;

    // If the plugin has no adjustable parameters, return an empty
    // list here (and there's no need to provide implementations of
    // getParameter and setParameter in that case either).

    // Note that it is your responsibility to make sure the parameters
    // start off having their default values (e.g. in the constructor
    // above).  The host needs to know the default value so it can do
    // things like provide a "reset to default" function, but it will
    // not explicitly set your parameters to their defaults for you if
    // they have not changed in the mean time.

    ParameterDescriptor d;
    d.identifier = "parameter";
    d.name = "Some Parameter";
    d.description = "";
    d.unit = "";
    d.minValue = 0;
    d.maxValue = 10;
    d.defaultValue = 5;
    d.isQuantized = false;
    list.push_back(d);

    return list;
}

float
TuningDifference::getParameter(string identifier) const
{
    if (identifier == "parameter") {
        return 5; // return the ACTUAL current value of your parameter here!
    }
    return 0;
}

void
TuningDifference::setParameter(string identifier, float value) 
{
    if (identifier == "parameter") {
        // set the actual value of your parameter
    }
}

TuningDifference::ProgramList
TuningDifference::getPrograms() const
{
    ProgramList list;

    // If you have no programs, return an empty list (or simply don't
    // implement this function or getCurrentProgram/selectProgram)

    return list;
}

string
TuningDifference::getCurrentProgram() const
{
    return ""; // no programs
}

void
TuningDifference::selectProgram(string name)
{
}

TuningDifference::OutputList
TuningDifference::getOutputDescriptors() const
{
    OutputList list;

    // See OutputDescriptor documentation for the possibilities here.
    // Every plugin must have at least one output.

    OutputDescriptor d;
    d.identifier = "output";
    d.name = "My Output";
    d.description = "";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::OneSamplePerStep;
    d.hasDuration = false;
    list.push_back(d);

    return list;
}

bool
TuningDifference::initialise(size_t channels, size_t stepSize, size_t blockSize)
{
    if (channels < getMinChannelCount() ||
	channels > getMaxChannelCount()) return false;

    // Real initialisation work goes here!

    return true;
}

void
TuningDifference::reset()
{
    // Clear buffers, reset stored values, etc
}

TuningDifference::FeatureSet
TuningDifference::process(const float *const *inputBuffers, Vamp::RealTime timestamp)
{
    // Do actual work!
    return FeatureSet();
}

TuningDifference::FeatureSet
TuningDifference::getRemainingFeatures()
{
    return FeatureSet();
}

