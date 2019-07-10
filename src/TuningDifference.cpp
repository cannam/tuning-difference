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

using namespace std;

TuningDifference::TuningDifference(float inputSampleRate) :
    Plugin(inputSampleRate),
    m_bulk(inputSampleRate)
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
    return "Estimate the tuning frequency of a recording, by comparing it to another recording of the same music whose tuning frequency is known";
}

string
TuningDifference::getMaker() const
{
    return "Chris Cannam";
}

int
TuningDifference::getPluginVersion() const
{
    // Increment this each time you release a version that behaves
    // differently from the previous one
    return 3;
}

string
TuningDifference::getCopyright() const
{
    // This function is not ideally named.  It does not necessarily
    // need to say who made the plugin -- getMaker does that -- but it
    // should indicate the terms under which it is distributed.  For
    // example, "Copyright (year). All Rights Reserved", or "GPL"
    return "GPL";
}

TuningDifference::InputDomain
TuningDifference::getInputDomain() const
{
    return m_bulk.getInputDomain();
}

size_t
TuningDifference::getPreferredBlockSize() const
{
    return m_bulk.getInputDomain();
}

size_t 
TuningDifference::getPreferredStepSize() const
{
    return m_bulk.getInputDomain();
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
    return m_bulk.getParameterDescriptors();
}

float
TuningDifference::getParameter(string id) const
{
    return m_bulk.getParameter(id);
}

void
TuningDifference::setParameter(string id, float value) 
{
    m_bulk.setParameter(id, value);
}

TuningDifference::ProgramList
TuningDifference::getPrograms() const
{
    return m_bulk.getPrograms();
}

string
TuningDifference::getCurrentProgram() const
{
    return m_bulk.getCurrentProgram();
}

void
TuningDifference::selectProgram(string program)
{
    m_bulk.selectProgram(program);
}

TuningDifference::OutputList
TuningDifference::getOutputDescriptors() const
{
    OutputList list;

    OutputList bulkOutputs = m_bulk.getOutputDescriptors();
    
    OutputDescriptor d;
    d.identifier = "cents";
    d.name = "Tuning Difference";
    d.description = "Difference in averaged frequency profile between the first (reference) channel and the other channel, in cents. A positive value means the other channel is higher than the reference.";
    d.unit = "cents";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::VariableSampleRate;
    d.hasDuration = false;
    m_outputs[d.identifier] = int(list.size());
    list.push_back(d);

    d.identifier = "tuningfreq";
    d.name = "Relative Tuning Frequency";
    d.description = "Tuning frequency of the second (other) channel, if the first (reference) channel is assumed to contain the same music as it at a tuning frequency of A=440Hz.";
    d.unit = "hz";
    d.hasFixedBinCount = true;
    d.binCount = 1;
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
    // caution: implementation dependency on BulkTuningDifference
    d.binCount = bulkOutputs[list.size()].binCount;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::FixedSampleRate;
    d.sampleRate = 1;
    d.hasDuration = false;
    m_outputs[d.identifier] = int(list.size());
    list.push_back(d);

    d.identifier = "otherfeature";
    d.name = "Other Feature";
    d.description = "Chroma feature from non-reference channel, before rotation.";
    d.unit = "";
    d.hasFixedBinCount = true;
    // caution: implementation dependency on BulkTuningDifference
    d.binCount = bulkOutputs[list.size()].binCount;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::FixedSampleRate;
    d.sampleRate = 1;
    d.hasDuration = false;
    m_outputs[d.identifier] = int(list.size());
    list.push_back(d);

    d.identifier = "rotfeature";
    d.name = "Other Feature at Rotated Frequency";
    d.description = "Chroma feature from non-reference channel calculated with the tuning frequency obtained from rotation matching. Note that this does not take into account any fine tuning, only the basic rotation match.";
    d.unit = "";
    d.hasFixedBinCount = true;
    // caution: implementation dependency on BulkTuningDifference
    d.binCount = bulkOutputs[list.size()].binCount;
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
TuningDifference::initialise(size_t channels, size_t stepSize, size_t blockSize)
{
    if (channels < getMinChannelCount() ||
	channels > getMaxChannelCount()) {
        return false;
    }

    return m_bulk.initialise(channels, stepSize, blockSize);
}

void
TuningDifference::reset()
{
    m_bulk.reset();
}

TuningDifference::FeatureSet
TuningDifference::process(const float *const *inputBuffers,
                          Vamp::RealTime timestamp)
{
    (void)m_bulk.process(inputBuffers, timestamp);
    return FeatureSet();
}

TuningDifference::FeatureSet
TuningDifference::getRemainingFeatures()
{
    FeatureSet bulkFs = m_bulk.getRemainingFeatures();
        
    FeatureSet fs;

    // caution: implementation dependency on BulkTuningDifference
    
    fs[m_outputs["reffeature"]] = bulkFs[m_outputs["reffeature"]];

    fs[m_outputs["otherfeature"]]
        .push_back(bulkFs[m_outputs["otherfeature"]][0]);

    fs[m_outputs["rotfeature"]]
        .push_back(bulkFs[m_outputs["rotfeature"]][0]);

    Feature f;
    f.hasTimestamp = true;
    f.timestamp = Vamp::RealTime::zeroTime;

    f.values.clear();
    f.values.push_back(bulkFs[m_outputs["cents"]][0].values[0]);
    fs[m_outputs["cents"]].push_back(f);
    
    f.values.clear();
    f.values.push_back(bulkFs[m_outputs["tuningfreq"]][0].values[0]);
    fs[m_outputs["tuningfreq"]].push_back(f);
    
    return fs;
}

