
#include "TuningDifference.h"

#include <iostream>

#include <cmath>
#include <cstdio>

#include <algorithm>

using namespace std;

static double pitchToFrequency(int pitch,
			       double centsOffset = 0.,
			       double concertA = 440.)
{
    double p = double(pitch) + (centsOffset / 100.);
    return concertA * pow(2.0, (p - 69.0) / 12.0); 
}

static double frequencyForCentsAbove440(double cents)
{
    return pitchToFrequency(69, cents, 440.);
}

TuningDifference::TuningDifference(float inputSampleRate) :
    Plugin(inputSampleRate),
    m_bpo(60),
    m_refCQ(new CQSpectrogram(paramsForTuningFrequency(440.),
			      CQSpectrogram::InterpolateHold))
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
    return TimeDomain;
}

size_t
TuningDifference::getPreferredBlockSize() const
{
    return 0;
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
    m_outputs[d.identifier] = list.size();
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
    m_outputs[d.identifier] = list.size();
    list.push_back(d);

    d.identifier = "reffeature";
    d.name = "Reference Feature";
    d.description = "Chroma feature from reference audio.";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = m_bpo;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::FixedSampleRate;
    d.sampleRate = 1;
    d.hasDuration = false;
    m_outputs[d.identifier] = list.size();
    list.push_back(d);

    d.identifier = "otherfeature";
    d.name = "Other Feature";
    d.description = "Chroma feature from other audio, before rotation.";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = m_bpo;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::FixedSampleRate;
    d.sampleRate = 1;
    d.hasDuration = false;
    m_outputs[d.identifier] = list.size();
    list.push_back(d);

    d.identifier = "rotfeature";
    d.name = "Other Feature at Rotated Frequency";
    d.description = "Chroma feature from reference audio calculated with the tuning frequency obtained from rotation matching.";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = m_bpo;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::FixedSampleRate;
    d.sampleRate = 1;
    d.hasDuration = false;
    m_outputs[d.identifier] = list.size();
    list.push_back(d);

    return list;
}

bool
TuningDifference::initialise(size_t channels, size_t stepSize, size_t blockSize)
{
    if (channels < getMinChannelCount() ||
	channels > getMaxChannelCount()) return false;

    if (stepSize != blockSize) return false;

    m_blockSize = blockSize;

    reset();
    
    return true;
}

void
TuningDifference::reset()
{
    if (m_frameCount > 0) {
	m_refCQ.reset(new CQSpectrogram(paramsForTuningFrequency(440.),
					CQSpectrogram::InterpolateHold));
	m_frameCount = 0;
    }
    m_refTotals = Chroma(m_refCQ->getTotalBins(), 0.0);
    m_other.clear();
}

template<typename T>
void addTo(vector<T> &a, const vector<T> &b)
{
    transform(a.begin(), a.end(), b.begin(), a.begin(), plus<T>());
}

template<typename T>
T distance(const vector<T> &a, const vector<T> &b)
{
    return inner_product(a.begin(), a.end(), b.begin(), T(),
			 plus<T>(), [](T x, T y) { return fabs(x - y); });
}

TuningDifference::TFeature
TuningDifference::computeFeatureFromTotals(const Chroma &totals) const
{
    TFeature feature(m_bpo);
    double sum = 0.0;
    
    for (int i = 0; i < (int)totals.size(); ++i) {
	double value = totals[i] / m_frameCount;
	feature[i % m_bpo] += value;
	sum += value;
    }

    for (int i = 0; i < m_bpo; ++i) {
	feature[i] /= sum;
    }

    cerr << "computeFeatureFromTotals: feature values:" << endl;
    for (auto v: feature) cerr << v << " ";
    cerr << endl;
    
    return feature;
}

CQParameters
TuningDifference::paramsForTuningFrequency(double hz) const
{
    return CQParameters(m_inputSampleRate,
			pitchToFrequency(36, hz),
			pitchToFrequency(96, hz),
			m_bpo);
}

TuningDifference::TFeature
TuningDifference::computeFeatureFromSignal(const Signal &signal, double hz) const
{
    CQSpectrogram cq(paramsForTuningFrequency(hz),
		     CQSpectrogram::InterpolateHold);

    Chroma totals(m_refCQ->getTotalBins(), 0.0);
    
    for (int i = 0; i < m_frameCount; ++i) {
	Signal::const_iterator first = signal.begin() + i * m_blockSize;
	Signal::const_iterator last = first + m_blockSize;
	if (last > signal.end()) last = signal.end();
	CQSpectrogram::RealSequence input(first, last);
	input.resize(m_blockSize);
	CQSpectrogram::RealBlock block = cq.process(input);
	for (const auto &v: block) addTo(totals, v);
    }

    return computeFeatureFromTotals(totals);
}

TuningDifference::FeatureSet
TuningDifference::process(const float *const *inputBuffers, Vamp::RealTime)
{
    CQSpectrogram::RealBlock block;
    CQSpectrogram::RealSequence input;

    input = CQSpectrogram::RealSequence
	(inputBuffers[0], inputBuffers[0] + m_blockSize);
    block = m_refCQ->process(input);
    for (const auto &v: block) addTo(m_refTotals, v);

    m_other.insert(m_other.end(),
		   inputBuffers[1], inputBuffers[1] + m_blockSize);
    
    ++m_frameCount;
    return FeatureSet();
}

double
TuningDifference::featureDistance(const TFeature &other, int rotation) const
{
    if (rotation == 0) {
	return distance(m_refFeature, other);
    } else {
	TFeature r(other);
	if (rotation > 0) {
	    rotate(r.begin(), r.begin() + rotation, r.end());
	} else {
	    rotate(r.begin(), r.end() + rotation, r.end());
	}
	return distance(m_refFeature, r);
    }
}

int
TuningDifference::findBestRotation(const TFeature &other) const
{
    map<double, int> dists;

    int maxSemis = 6;
    int maxRotation = (m_bpo * maxSemis) / 12;

    for (int r = -maxRotation; r <= maxRotation; ++r) {
	double dist = featureDistance(other, r);
	dists[dist] = r;
	cerr << "rotation " << r << ": score " << dist << endl;
    }

    int best = dists.begin()->second;

    cerr << "best is " << best << endl;
    return best;
}

TuningDifference::FeatureSet
TuningDifference::getRemainingFeatures()
{
    FeatureSet fs;
    if (m_frameCount == 0) return fs;

    m_refFeature = computeFeatureFromTotals(m_refTotals);
    TFeature otherFeature = computeFeatureFromSignal(m_other, 440.);

    Feature f;

    f.values.clear();
    for (auto v: m_refFeature) f.values.push_back(v);
    fs[m_outputs["reffeature"]].push_back(f);

    f.values.clear();
    for (auto v: otherFeature) f.values.push_back(v);
    fs[m_outputs["otherfeature"]].push_back(f); 
   
    int rotation = findBestRotation(otherFeature);

    int coarseCents = -(rotation * 100) / (m_bpo / 12);

    cerr << "rotation " << rotation << " -> cents " << coarseCents << endl;

    double coarseHz = frequencyForCentsAbove440(coarseCents);

    TFeature coarseFeature = computeFeatureFromSignal(m_other, coarseHz);
    double coarseScore = featureDistance(coarseFeature);

    cerr << "corresponding Hz " << coarseHz << " scores " << coarseScore << endl;

    f.values.clear();
    for (auto v: coarseFeature) f.values.push_back(v);
    fs[m_outputs["rotfeature"]].push_back(f);
    
    return fs;
}

