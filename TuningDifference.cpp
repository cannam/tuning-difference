
#include "TuningDifference.h"

#include <iostream>

using std::cerr;
using std::endl;


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
    return "tuning-difference";
}

string
TuningDifference::getName() const
{
    return "Tuning Difference";
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
    return FrequencyDomain;
}

size_t
TuningDifference::getPreferredBlockSize() const
{
    return 16384;
}

size_t 
TuningDifference::getPreferredStepSize() const
{
    return 0;
}

size_t
TuningDifference::getMinChannelCount() const
{
    return 2;
}

size_t
TuningDifference::getMaxChannelCount() const
{
    return 2;
}

TuningDifference::ParameterList
TuningDifference::getParameterDescriptors() const
{
    ParameterList list;
    return list;
}

float
TuningDifference::getParameter(string) const
{
    return 0;
}

void
TuningDifference::setParameter(string, float) 
{
}

TuningDifference::ProgramList
TuningDifference::getPrograms() const
{
    ProgramList list;
    return list;
}

string
TuningDifference::getCurrentProgram() const
{
    return ""; // no programs
}

void
TuningDifference::selectProgram(string)
{
}

TuningDifference::OutputList
TuningDifference::getOutputDescriptors() const
{
    OutputList list;

    OutputDescriptor d;
    d.identifier = "cents";
    d.name = "Tuning Difference";
    d.description = "Difference in averaged frequency profile between channels 1 and 2, in cents. A positive value means channel 2 is higher.";
    d.unit = "cents";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::VariableSampleRate;
    d.hasDuration = false;
    list.push_back(d);

    d.identifier = "tuningfreq";
    d.name = "Relative Tuning Frequency";
    d.description = "Tuning frequency of channel 2, if channel 1 is assumed to contain the same music as it at a tuning frequency of A=440Hz.";
    d.unit = "cents";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::VariableSampleRate;
    d.hasDuration = false;
    list.push_back(d);

    d.identifier = "correlation";
    d.name = "Frequency-shift correlation curve";
    d.description = "";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::FixedSampleRate;
    d.sampleRate = 100;
    d.hasDuration = false;
    list.push_back(d);

    return list;
}

bool
TuningDifference::initialise(size_t channels, size_t stepSize, size_t blockSize)
{
    if (channels < getMinChannelCount() ||
	channels > getMaxChannelCount()) return false;

    if (blockSize != getPreferredBlockSize() ||
	stepSize != blockSize/2) return false;

    m_blockSize = blockSize;

    reset();
    
    return true;
}

void
TuningDifference::reset()
{
    m_sum[0].clear();
    m_sum[1].clear();
    m_frameCount = 0;
}

TuningDifference::FeatureSet
TuningDifference::process(const float *const *inputBuffers, Vamp::RealTime timestamp)
{
    for (int c = 0; c < 2; ++c) {
	m_sum[c].resize(m_blockSize/2 - 1);
	for (int i = 1; i < m_blockSize/2; ++i) { // discarding DC and Nyquist
	    double energy =
		inputBuffers[c][i*2  ] * inputBuffers[c][i*2  ] +
		inputBuffers[c][i*2+1] * inputBuffers[c][i*2+1];
	    m_sum[c][i-1] += energy;
	}
    }
    
    ++m_frameCount;
    return FeatureSet();
}

TuningDifference::FeatureSet
TuningDifference::getRemainingFeatures()
{
    int n = m_sum[0].size();
    if (n == 0) return FeatureSet();

    Feature f;
    FeatureSet fs;
    
    vector<double> corr(n * 2 - 1, 0.0);
    for (int shift = -(n-1); shift <= n-1; ++shift) {
	int index = shift + n-1;
	int count = 0;
//	cerr << "index = " << index << ", n = " << n << endl;
	for (int i = 0; i < n; ++i) {
	    int j = i + shift;
	    if (j >= 0 && j < n) {
		corr[index] += m_sum[1][i] * m_sum[0][j];
		++count;
	    }
	}
	if (count > 0) {
	    corr[index] /= count;
	}
	f.values.clear();
//	cerr << "value = " << corr[index] << endl;
	f.values.push_back(corr[index]);
	fs[2].push_back(f);
    }

    return fs;
}

