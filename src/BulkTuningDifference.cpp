/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
  Centre for Digital Music, Queen Mary University of London.
    
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/

#include "BulkTuningDifference.h"

#include <iostream>

#include <cmath>
#include <cstdio>
#include <climits>

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

static float defaultMaxDuration = 0.f;
static int defaultMaxSemis = 5;
static bool defaultFineTuning = true;

BulkTuningDifference::BulkTuningDifference(float inputSampleRate) :
    Plugin(inputSampleRate),
    m_channelCount(0),
    m_bpo(120),
    m_blockSize(0),
    m_frameCount(0),
    m_maxDuration(defaultMaxDuration),
    m_maxSemis(defaultMaxSemis),
    m_fineTuning(defaultFineTuning)
{
}

BulkTuningDifference::~BulkTuningDifference()
{
}

string
BulkTuningDifference::getIdentifier() const
{
    return "bulk-tuning-difference";
}

string
BulkTuningDifference::getName() const
{
    return "Bulk Tuning Difference";
}

string
BulkTuningDifference::getDescription() const
{
    return "Estimate the tuning frequencies of a set of recordings at once, by comparing them to a reference recording of the same music whose tuning frequency is known";
}

string
BulkTuningDifference::getMaker() const
{
    return "Chris Cannam";
}

int
BulkTuningDifference::getPluginVersion() const
{
    // Increment this each time you release a version that behaves
    // differently from the previous one
    return 3;
}

string
BulkTuningDifference::getCopyright() const
{
    // This function is not ideally named.  It does not necessarily
    // need to say who made the plugin -- getMaker does that -- but it
    // should indicate the terms under which it is distributed.  For
    // example, "Copyright (year). All Rights Reserved", or "GPL"
    return "GPL";
}

BulkTuningDifference::InputDomain
BulkTuningDifference::getInputDomain() const
{
    return TimeDomain;
}

size_t
BulkTuningDifference::getPreferredBlockSize() const
{
    return 0;
}

size_t 
BulkTuningDifference::getPreferredStepSize() const
{
    return 0;
}

size_t
BulkTuningDifference::getMinChannelCount() const
{
    return 2;
}

size_t
BulkTuningDifference::getMaxChannelCount() const
{
    return 1000;
}

BulkTuningDifference::ParameterList
BulkTuningDifference::getParameterDescriptors() const
{
    ParameterList list;

    ParameterDescriptor desc;

    desc.identifier = "maxduration";
    desc.name = "Maximum duration to analyse";
    desc.description = "The maximum duration (in seconds) to consider from either input file, always taken from the start of the input. Zero means there is no limit.";
    desc.minValue = 0;
    desc.maxValue = 3600;
    desc.defaultValue = defaultMaxDuration;
    desc.isQuantized = false;
    desc.unit = "s";
    list.push_back(desc);
    
    desc.identifier = "maxrange";
    desc.name = "Maximum range in semitones";
    desc.description = "The maximum difference in semitones that will be searched.";
    desc.minValue = 1;
    desc.maxValue = 11;
    desc.defaultValue = defaultMaxSemis;
    desc.isQuantized = true;
    desc.quantizeStep = 1;
    desc.unit = "semitones";
    list.push_back(desc);
    
    desc.identifier = "finetuning";
    desc.name = "Fine tuning";
    desc.description = "Use a fine tuning stage to increase nominal resolution from 10 cents to 1 cent.";
    desc.minValue = 0;
    desc.maxValue = 1;
    desc.defaultValue = (defaultFineTuning ? 1.f : 0.f);
    desc.isQuantized = true;
    desc.quantizeStep = 1;
    desc.unit = "";
    list.push_back(desc);

    return list;
}

float
BulkTuningDifference::getParameter(string id) const
{
    if (id == "maxduration") {
        return m_maxDuration;
    } else if (id == "maxrange") {
        return float(m_maxSemis);
    } else if (id == "finetuning") {
        return m_fineTuning ? 1.f : 0.f;
    }
    return 0;
}

void
BulkTuningDifference::setParameter(string id, float value) 
{
    if (id == "maxduration") {
        m_maxDuration = value;
    } else if (id == "maxrange") {
        m_maxSemis = int(roundf(value));
    } else if (id == "finetuning") {
        m_fineTuning = (value > 0.5f);
    }
}

BulkTuningDifference::ProgramList
BulkTuningDifference::getPrograms() const
{
    ProgramList list;
    return list;
}

string
BulkTuningDifference::getCurrentProgram() const
{
    return ""; // no programs
}

void
BulkTuningDifference::selectProgram(string)
{
}

BulkTuningDifference::OutputList
BulkTuningDifference::getOutputDescriptors() const
{
    OutputList list;

    OutputDescriptor d;
    d.identifier = "cents";
    d.name = "Tuning Differences";
    d.description = "A single feature vector containing a value for each input channel after the first (reference) channel, containing the difference in averaged frequency profile between that channel and the reference channel, in cents. A positive value means the corresponding channel is higher than the reference.";
    d.unit = "cents";
    d.hasFixedBinCount = true;
    if (m_channelCount > 1) {
        d.binCount = m_channelCount - 1;
    } else {
        d.binCount = 1;
    }
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::VariableSampleRate;
    d.hasDuration = false;
    m_outputs[d.identifier] = int(list.size());
    list.push_back(d);

    d.identifier = "tuningfreq";
    d.name = "Relative Tuning Frequencies";
    d.description = "A single feature vector containing a value for each input channel after the first (reference) channel, containing the tuning frequency of that channel, if the reference channel is assumed to contain the same music as it at a tuning frequency of A=440Hz.";
    d.unit = "hz";
    d.hasFixedBinCount = true;
    if (m_channelCount > 1) {
        d.binCount = m_channelCount - 1;
    } else {
        d.binCount = 1;
    }
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::VariableSampleRate;
    d.hasDuration = false;
    m_outputs[d.identifier] = int(list.size());
    list.push_back(d);

    d.identifier = "reffeature";
    d.name = "Reference Feature";
    d.description = "Chroma feature from reference channel.";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = m_bpo;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::FixedSampleRate;
    d.sampleRate = 1;
    d.hasDuration = false;
    m_outputs[d.identifier] = int(list.size());
    list.push_back(d);

    d.identifier = "otherfeature";
    d.name = "Other Features";
    d.description = "Series of chroma feature vectors from the non-reference audio channels, before rotation.";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = m_bpo;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::FixedSampleRate;
    d.sampleRate = 1;
    d.hasDuration = false;
    m_outputs[d.identifier] = int(list.size());
    list.push_back(d);

    d.identifier = "rotfeature";
    d.name = "Other Features at Rotated Frequency";
    d.description = "Series of chroma feature vectors from the non-reference audio channels, calculated with the tuning frequency obtained from rotation matching. Note that this does not take into account any fine tuning, only the basic rotation match.";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = m_bpo;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::FixedSampleRate;
    d.sampleRate = 1;
    d.hasDuration = false;
    m_outputs[d.identifier] = int(list.size());
    list.push_back(d);

    return list;
}

bool
BulkTuningDifference::initialise(size_t channels, size_t stepSize, size_t blockSize)
{
    if (channels < getMinChannelCount()) return false;
    if (stepSize != blockSize) return false;
    if (m_blockSize > INT_MAX) return false;

    m_channelCount = int(channels);
    m_blockSize = int(blockSize);

    reset();
    
    return true;
}

void
BulkTuningDifference::reset()
{
    Chromagram::Parameters params(paramsForTuningFrequency(440.));
    m_reference.clear();
    m_refChroma.reset(new Chromagram(params));
    m_refTotals = TFeature(m_bpo, 0.0);
    m_refFeatures.clear();
    m_otherChroma.clear();
    for (int i = 1; i < m_channelCount; ++i) {
        m_otherChroma.push_back(std::make_shared<Chromagram>(params));
    }
    m_otherTotals = vector<TFeature>(m_channelCount-1, TFeature(m_bpo, 0.0));
    m_frameCount = 0;
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

BulkTuningDifference::TFeature
BulkTuningDifference::computeFeatureFromTotals(const TFeature &totals) const
{
    if (m_frameCount == 0) return totals;
    
    TFeature feature(m_bpo);
    double sum = 0.0;

    for (int i = 0; i < m_bpo; ++i) {
	double value = totals[i] / m_frameCount;
	feature[i] += value;
	sum += value;
    }

    if (sum != 0.0) {
        for (int i = 0; i < m_bpo; ++i) {
            feature[i] /= sum;
        }
    }
    
    return feature;
}

Chromagram::Parameters
BulkTuningDifference::paramsForTuningFrequency(double hz) const
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

BulkTuningDifference::TFeature
BulkTuningDifference::computeFeatureFromSignal(const Signal &signal, double hz) const
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

BulkTuningDifference::FeatureSet
BulkTuningDifference::process(const float *const *inputBuffers, Vamp::RealTime)
{
    if (m_maxDuration > 0) {
        int maxFrames = int((m_maxDuration * m_inputSampleRate) /
                            float(m_blockSize));
        if (m_frameCount > maxFrames) return FeatureSet();
    }

    CQBase::RealBlock block;
    CQBase::RealSequence input;

    input = CQBase::RealSequence
        (inputBuffers[0], inputBuffers[0] + m_blockSize);
    block = m_refChroma->process(input);
    for (const auto &v: block) addTo(m_refTotals, v);

    if (m_fineTuning) {
        m_reference.insert(m_reference.end(),
                           inputBuffers[0],
                           inputBuffers[0] + m_blockSize);
    }

    for (int c = 1; c < m_channelCount; ++c) {
        input = CQBase::RealSequence
            (inputBuffers[c], inputBuffers[c] + m_blockSize);
        block = m_otherChroma[c-1]->process(input);
        for (const auto &v: block) addTo(m_otherTotals[c-1], v);
    }
    
    ++m_frameCount;
    return FeatureSet();
}

BulkTuningDifference::FeatureSet
BulkTuningDifference::getRemainingFeatures()
{
    FeatureSet fs;
    if (m_frameCount == 0) return fs;

    m_refFeatures[0] = computeFeatureFromTotals(m_refTotals);

    Feature f;
    f.hasTimestamp = true;
    f.timestamp = Vamp::RealTime::zeroTime;
    f.values.clear();
    fs[m_outputs["cents"]].push_back(f);
    fs[m_outputs["tuningfreq"]].push_back(f);

    for (int c = 1; c < m_channelCount; ++c) {
        getRemainingFeaturesForChannel(c, fs);
    }

    return fs;
}

void
BulkTuningDifference::getRemainingFeaturesForChannel(int channel,
                                                     FeatureSet &fs)
{
    TFeature otherFeature =
        computeFeatureFromTotals(m_otherTotals[channel-1]);

    Feature f;
    f.hasTimestamp = true;
    f.timestamp = Vamp::RealTime::zeroTime;

    f.values.clear();
    for (auto v: m_refFeatures[0]) f.values.push_back(float(v));
    fs[m_outputs["reffeature"]].push_back(f);

    f.values.clear();
    for (auto v: otherFeature) f.values.push_back(float(v));
    fs[m_outputs["otherfeature"]].push_back(f); 
   
    int rotation = findBestRotation(m_refFeatures[0], otherFeature);

    int coarseCents = -(rotation * 1200) / m_bpo;

    cerr << "channel " << channel << ": rotation " << rotation << " -> cents " << coarseCents << endl;

    TFeature rotatedFeature = otherFeature;
    if (rotation != 0) {
        rotateFeature(rotatedFeature, rotation);
    }

    f.values.clear();
    for (auto v: rotatedFeature) f.values.push_back(float(v));
    fs[m_outputs["rotfeature"]].push_back(f);

    if (m_fineTuning) {
    
        pair<int, double> fine = findFineFrequency(rotatedFeature, coarseCents);

        int fineCents = fine.first;
        double fineHz = fine.second;

        fs[m_outputs["cents"]][0].values.push_back(float(fineCents));
        fs[m_outputs["tuningfreq"]][0].values.push_back(float(fineHz));
    
        cerr << "channel " << channel << ": overall best Hz = " << fineHz << endl;

    } else {

        fs[m_outputs["cents"]][0].values.push_back(float(coarseCents));
        fs[m_outputs["tuningfreq"]][0].values.push_back
            (float(frequencyForCentsAbove440(coarseCents)));
    }        
}

void
BulkTuningDifference::rotateFeature(TFeature &r, int rotation) const
{
    if (rotation < 0) {
        rotate(r.begin(), r.begin() - rotation, r.end());
    } else {
        rotate(r.begin(), r.end() - rotation, r.end());
    }
}

double
BulkTuningDifference::featureDistance(const TFeature &ref,
                                      const TFeature &other,
                                      int rotation) const
{
    if (rotation == 0) {
	return distance(ref, other);
    } else {
	// A positive rotation pushes the tuning frequency up for this
	// chroma, negative one pulls it down. If a positive rotation
	// makes this chroma match an un-rotated reference, then this
	// chroma must have initially been lower than the reference.
	TFeature r(other);
        rotateFeature(r, rotation);
	return distance(ref, r);
    }
}

int
BulkTuningDifference::findBestRotation(const TFeature &ref,
                                       const TFeature &other) const
{
    map<double, int> dists;

    int maxRotation = (m_bpo * m_maxSemis) / 12;

    for (int r = -maxRotation; r <= maxRotation; ++r) {
	double dist = featureDistance(ref, other, r);
	dists[dist] = r;
    }

    int best = dists.begin()->second;

    return best;
}

pair<int, double>
BulkTuningDifference::findFineFrequency(const TFeature &rotatedOtherFeature,
                                        int coarseCents)
{
    int coarseResolution = 1200 / m_bpo;
    int searchDistance = coarseResolution/2 - 1;

    int bestCents = coarseCents;
    double bestHz = frequencyForCentsAbove440(coarseCents);

    cerr << "findFineFrequency: coarse frequency is " << bestHz << endl;
    cerr << "searchDistance = " << searchDistance << endl;

    double bestScore = 0;
    bool firstScore = true;
    
    for (int sign = -1; sign <= 1; sign += 2) {
	for (int offset = (sign < 0 ? 0 : 1);
             offset <= searchDistance;
             ++offset) {

	    int fineCents = coarseCents + sign * offset;
	    double fineHz = frequencyForCentsAbove440(fineCents);

	    cerr << "trying with fineCents = " << fineCents << "..." << endl;

            // compare the rotated "other" chroma with a reference
            // chroma shifted by the offset in the opposite direction

            int compensatingCents = -sign * offset;
            TFeature compensatedReference;

            if (m_refFeatures.find(compensatingCents) == m_refFeatures.end()) {
                double compensatingHz = frequencyForCentsAbove440
                    (compensatingCents);
            
                compensatedReference = computeFeatureFromSignal
                    (m_reference, compensatingHz);

                m_refFeatures[compensatingCents] = compensatedReference;

            } else {

                compensatedReference = m_refFeatures[compensatingCents];
            }

	    double fineScore = featureDistance(compensatedReference,
                                               rotatedOtherFeature,
                                               0); // we are rotated already

	    cerr << "fine offset = " << offset << ", cents = " << fineCents
		 << ", Hz = " << fineHz << ", score " << fineScore
		 << " (best score so far " << bestScore << ")" << endl;
	    
	    if ((fineScore < bestScore) || firstScore) {
		cerr << "is good!" << endl;
		bestScore = fineScore;
		bestCents = fineCents;
		bestHz = fineHz;
                firstScore = false;
	    } else {
		break;
	    }
	}
    }
    
    return pair<int, double>(bestCents, bestHz);
}

