/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
  Centre for Digital Music, Queen Mary University of London.
    
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/

#include "TuningDifference.h"

#include <iostream>

#include <cmath>
#include <cstdio>

#include <algorithm>
#include <numeric>

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

static float defaultMaxDuration = 0;

TuningDifference::TuningDifference(float inputSampleRate) :
    Plugin(inputSampleRate),
    m_bpo(60),
    m_refChroma(new Chromagram(paramsForTuningFrequency(440.))),
    m_blockSize(0),
    m_frameCount(0),
    m_maxDuration(defaultMaxDuration)
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

    ParameterDescriptor desc;

    desc.identifier = "maxduration";
    desc.name = "Maximum duration to analyse";
    desc.description = "The maximum duration (in seconds) to consider from either input file. Zero means there is no limit.";
    desc.minValue = 0;
    desc.maxValue = 3600;
    desc.defaultValue = defaultMaxDuration;
    desc.isQuantized = false;
    desc.unit = "s";
    list.push_back(desc);
    
    //!!! parameter: max search range
    //!!! parameter: fine search precision
    //!!! parameter: max amount of audio to listen to
    return list;
}

float
TuningDifference::getParameter(string id) const
{
    if (id == "maxduration") {
        return m_maxDuration;
    }
    return 0;
}

void
TuningDifference::setParameter(string id, float value) 
{
    if (id == "maxduration") {
        m_maxDuration = value;
    }
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
	m_refChroma.reset(new Chromagram(paramsForTuningFrequency(440.)));
	m_frameCount = 0;
    }
    m_refTotals = TFeature(m_bpo, 0.0);
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
TuningDifference::computeFeatureFromTotals(const TFeature &totals) const
{
    if (m_frameCount == 0) return totals;
    
    TFeature feature(m_bpo);
    double sum = 0.0;

    for (int i = 0; i < m_bpo; ++i) {
	double value = totals[i] / m_frameCount;
	feature[i] += value;
	sum += value;
    }

    for (int i = 0; i < m_bpo; ++i) {
	feature[i] /= sum;
    }

//    cerr << "computeFeatureFromTotals: feature values:" << endl;
//    for (auto v: feature) cerr << v << " ";
//    cerr << endl;
    
    return feature;
}

Chromagram::Parameters
TuningDifference::paramsForTuningFrequency(double hz) const
{
    Chromagram::Parameters params(m_inputSampleRate);
    params.lowestOctave = 2;
    params.octaveCount = 4;
    params.binsPerOctave = m_bpo;
    params.tuningFrequency = hz;
    params.atomHopFactor = 0.5;
    params.window = CQParameters::Hann;
    return params;
}

TuningDifference::TFeature
TuningDifference::computeFeatureFromSignal(const Signal &signal, double hz) const
{
    Chromagram chromagram(paramsForTuningFrequency(hz));

    TFeature totals(m_bpo, 0.0);

    cerr << "computeFeatureFromSignal: hz = " << hz << ", frame count = " << m_frameCount << endl;
    
    for (int i = 0; i < m_frameCount; ++i) {
	Signal::const_iterator first = signal.begin() + i * m_blockSize;
	Signal::const_iterator last = first + m_blockSize;
	if (last > signal.end()) last = signal.end();
	CQBase::RealSequence input(first, last);
	input.resize(m_blockSize);
	CQBase::RealBlock block = chromagram.process(input);
	for (const auto &v: block) addTo(totals, v);
    }

    return computeFeatureFromTotals(totals);
}

TuningDifference::FeatureSet
TuningDifference::process(const float *const *inputBuffers, Vamp::RealTime)
{
    if (m_maxDuration > 0) {
        int maxFrames = (m_maxDuration * m_inputSampleRate) / m_blockSize;
        if (m_frameCount > maxFrames) return FeatureSet();
    }
    
    CQBase::RealBlock block;
    CQBase::RealSequence input;

    input = CQBase::RealSequence
	(inputBuffers[0], inputBuffers[0] + m_blockSize);
    block = m_refChroma->process(input);
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
	// A positive rotation pushes the tuning frequency up for this
	// chroma, negative one pulls it down. If a positive rotation
	// makes this chroma match an un-rotated reference, then this
	// chroma must have initially been lower than the reference.
	TFeature r(other);
	if (rotation < 0) {
	    rotate(r.begin(), r.begin() - rotation, r.end());
	} else {
	    rotate(r.begin(), r.end() - rotation, r.end());
	}
	return distance(m_refFeature, r);
    }
}

int
TuningDifference::findBestRotation(const TFeature &other) const
{
    map<double, int> dists;

    int maxSemis = 4;
    int maxRotation = (m_bpo * maxSemis) / 12;

    for (int r = -maxRotation; r <= maxRotation; ++r) {
	double dist = featureDistance(other, r);
	dists[dist] = r;
//	cerr << "rotation " << r << ": score " << dist << endl;
    }

    int best = dists.begin()->second;

//    cerr << "best is " << best << endl;
    return best;
}

pair<int, double>
TuningDifference::findFineFrequency(int coarseCents, double coarseScore)
{
    int coarseResolution = 1200 / m_bpo;
    int searchDistance = coarseResolution/2 - 1;

    double bestScore = coarseScore;
    int bestCents = coarseCents;
    double bestHz = frequencyForCentsAbove440(coarseCents);

    cerr << "corresponding coarse Hz " << bestHz << " scores " << coarseScore << endl;
    cerr << "searchDistance = " << searchDistance << endl;
    
    for (int sign = -1; sign <= 1; sign += 2) {
	for (int offset = 1; offset <= searchDistance; ++offset) {

	    int fineCents = coarseCents + sign * offset;

	    cerr << "trying with fineCents = " << fineCents << "..." << endl;
	    
	    double fineHz = frequencyForCentsAbove440(fineCents);
	    TFeature fineFeature = computeFeatureFromSignal(m_other, fineHz);
	    double fineScore = featureDistance(fineFeature);

	    cerr << "fine offset = " << offset << ", cents = " << fineCents
		 << ", Hz = " << fineHz << ", score " << fineScore
		 << " (best score so far " << bestScore << ")" << endl;
	    
	    if (fineScore < bestScore) {
		cerr << "is good!" << endl;
		bestScore = fineScore;
		bestCents = fineCents;
		bestHz = fineHz;
	    } else {
		break;
	    }
	}
    }

    //!!! could keep a vector of scores & then interpolate...
    
    return pair<int, double>(bestCents, bestHz);
}

TuningDifference::FeatureSet
TuningDifference::getRemainingFeatures()
{
    FeatureSet fs;
    if (m_frameCount == 0) return fs;

    m_refFeature = computeFeatureFromTotals(m_refTotals);
    TFeature otherFeature = computeFeatureFromSignal(m_other, 440.);

    Feature f;
    f.hasTimestamp = true;
    f.timestamp = Vamp::RealTime::zeroTime;

    f.values.clear();
    for (auto v: m_refFeature) f.values.push_back(v);
    fs[m_outputs["reffeature"]].push_back(f);

    f.values.clear();
    for (auto v: otherFeature) f.values.push_back(v);
    fs[m_outputs["otherfeature"]].push_back(f); 
   
    int rotation = findBestRotation(otherFeature);

    int coarseCents = -(rotation * 1200) / m_bpo;

    cerr << "rotation " << rotation << " -> cents " << coarseCents << endl;

    double coarseHz = frequencyForCentsAbove440(coarseCents);

    TFeature coarseFeature;
    if (rotation == 0) {
        coarseFeature = otherFeature;
    } else {
        coarseFeature = computeFeatureFromSignal(m_other, coarseHz);
    }
    double coarseScore = featureDistance(coarseFeature);

    cerr << "corresponding Hz " << coarseHz << " scores " << coarseScore << endl;

    //!!! This should be returning the fine chroma, not the coarse
    f.values.clear();
    for (auto v: coarseFeature) f.values.push_back(v);
    fs[m_outputs["rotfeature"]].push_back(f);

    pair<int, double> fine = findFineFrequency(coarseCents, coarseScore);
    int fineCents = fine.first;
    double fineHz = fine.second;

    f.values.clear();
    f.values.push_back(fineHz);
    fs[m_outputs["tuningfreq"]].push_back(f);

    f.values.clear();
    f.values.push_back(fineCents);
    fs[m_outputs["cents"]].push_back(f);
    
    cerr << "overall best Hz = " << fineHz << endl;
    
    return fs;
}

