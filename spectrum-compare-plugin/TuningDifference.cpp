
#include "TuningDifference.h"

#include <iostream>

#include <cmath>
#include <cstdio>

using std::cerr;
using std::endl;

static double targetFmin = 60.0;
static double targetFmax = 1500.0;

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
    d.unit = "hz";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::VariableSampleRate;
    d.hasDuration = false;
    list.push_back(d);

    d.identifier = "curve";
    d.name = "Shift Correlation Curve";
    d.description = "Correlation between shifted and unshifted sources, for each probed shift in cents.";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::FixedSampleRate;
    d.sampleRate = 100;
    d.hasDuration = false;
    list.push_back(d);

    int targetBinMin = int(floor(targetFmin * m_blockSize / m_inputSampleRate));
    int targetBinMax = int(ceil(targetFmax * m_blockSize / m_inputSampleRate));
    cerr << "target bin range: " << targetBinMin << " -> " << targetBinMax << endl;

    d.identifier = "averages";
    d.name = "Spectrum averages";
    d.description = "Average magnitude spectrum for each channel.";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = (targetBinMax > targetBinMin ? targetBinMax - targetBinMin + 1 : 100);
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::FixedSampleRate;
    d.sampleRate = 1;
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
	if (m_sum[c].size() == 0) {
	    m_sum[c].resize(m_blockSize/2 + 1, 0.0);
	}
	for (int i = 0; i <= m_blockSize/2; ++i) {
	    double energy =
		inputBuffers[c][i*2  ] * inputBuffers[c][i*2  ] +
		inputBuffers[c][i*2+1] * inputBuffers[c][i*2+1];
	    double mag = sqrt(energy);
	    m_sum[c][i] += mag;
	    m_sum[c][i/2] += mag;
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

    int maxshift = 2400; // cents

    int bestshift = -1;
    double bestdist = 0;

    for (int i = 0; i < n; ++i) {
	m_sum[0][i] /= m_frameCount;
	m_sum[1][i] /= m_frameCount;
    }

    for (int c = 0; c < 2; ++c) {
	double tot = 0.0;
	for (int i = 0; i < n; ++i) {
	    tot += m_sum[c][i];
	}
	if (tot != 0.0) {
	    for (int i = 0; i < n; ++i) {
		m_sum[c][i] /= tot;
	    }
	}
    }

    int targetBinMin = int(floor(targetFmin * m_blockSize / m_inputSampleRate));
    int targetBinMax = int(ceil(targetFmax * m_blockSize / m_inputSampleRate));
    cerr << "target bin range: " << targetBinMin << " -> " << targetBinMax << endl;
	
    f.values.clear();
    for (int i = targetBinMin; i < targetBinMax; ++i) {
	f.values.push_back(m_sum[0][i]);
    }
    fs[3].push_back(f);
    f.values.clear();
    for (int i = targetBinMin; i < targetBinMax; ++i) {
	f.values.push_back(m_sum[1][i]);
    }
    fs[3].push_back(f);

    f.values.clear();
    
    for (int shift = -maxshift; shift <= maxshift; ++shift) {

	double multiplier = pow(2.0, double(shift) / 1200.0);
	double dist = 0.0;

//	cerr << "shift = " << shift << ", multiplier = " << multiplier << endl;

	int contributing = 0;
	
	for (int i = targetBinMin; i < targetBinMax; ++i) {

	    double source = i / multiplier;
	    int s0 = int(source), s1 = s0 + 1;
	    double p1 = source - s0;
	    double p0 = 1.0 - p1;

	    double value = 0.0;
	    if (s0 >= 0 && s0 < n) {
		value += p0 * m_sum[1][s0];
		++contributing;
	    }
	    if (s1 >= 0 && s1 < n) {
		value += p1 * m_sum[1][s1];
		++contributing;
	    }

//	    if (shift == -1) {
//		cerr << "for multiplier " << multiplier << ", target " << i << ", source " << source << ", value " << p0 << " * " << m_sum[1][s0] << " + " << p1 << " * " << m_sum[1][s1] << " = " << value << ", other " << m_sum[0][i] << endl;
//	    }
	    
	    double diff = fabs(m_sum[0][i] - value);
	    dist += diff;
	}

	dist /= contributing;
	
	f.values.clear();
	f.values.push_back(dist);
	char label[100];
	sprintf(label, "%f at shift %d freq mult %f", dist, shift, multiplier);
	f.label = label;
	fs[2].push_back(f);

	if (bestshift == -1 || dist < bestdist) {
	    bestshift = shift;
	    bestdist = dist;
	}
    }

    f.timestamp = Vamp::RealTime::zeroTime;
    f.hasTimestamp = true;
    f.label = "";

    f.values.clear();
//    cerr << "best dist = " << bestdist << " at shift " << bestshift << endl;
    f.values.push_back(-bestshift);
    fs[0].push_back(f);

    f.values.clear();
    f.values.push_back(440.0 / pow(2.0, double(bestshift) / 1200.0));
    fs[1].push_back(f);
    
    return fs;
}

