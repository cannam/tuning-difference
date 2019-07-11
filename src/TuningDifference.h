/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
  Centre for Digital Music, Queen Mary University of London.
    
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/

#ifndef TUNING_DIFFERENCE_H
#define TUNING_DIFFERENCE_H

#include <vamp-sdk/Plugin.h>

#include <cq/Chromagram.h>

#include <memory>

using std::string;
using std::vector;

class TuningDifference : public Vamp::Plugin
{
public:
    TuningDifference(float inputSampleRate);
    virtual ~TuningDifference();

    string getIdentifier() const;
    string getName() const;
    string getDescription() const;
    string getMaker() const;
    int getPluginVersion() const;
    string getCopyright() const;

    InputDomain getInputDomain() const;
    size_t getPreferredBlockSize() const;
    size_t getPreferredStepSize() const;
    size_t getMinChannelCount() const;
    size_t getMaxChannelCount() const;

    ParameterList getParameterDescriptors() const;
    float getParameter(string identifier) const;
    void setParameter(string identifier, float value);

    ProgramList getPrograms() const;
    string getCurrentProgram() const;
    void selectProgram(string name);

    OutputList getOutputDescriptors() const;

    bool initialise(size_t channels, size_t stepSize, size_t blockSize);
    void reset();

    FeatureSet process(const float *const *inputBuffers,
                       Vamp::RealTime timestamp);

    FeatureSet getRemainingFeatures();

protected:
    typedef vector<float> Signal;
    typedef vector<double> TFeature;

    int m_channelCount;
    int m_bpo;
    int m_blockSize;
    int m_frameCount;
    float m_maxDuration;
    int m_maxSemis;
    bool m_fineTuning;

    std::unique_ptr<Chromagram> m_refChroma;
    TFeature m_refTotals;
    std::map<int, TFeature> m_refFeatures; // map from cents-offset to feature
    Signal m_reference; // we have to retain this when fine-tuning is enabled
    std::vector<std::shared_ptr<Chromagram>> m_otherChroma;
    std::vector<TFeature> m_otherTotals;

    Chromagram::Parameters paramsForTuningFrequency(double hz) const;
    TFeature computeFeatureFromTotals(const TFeature &totals) const;
    TFeature computeFeatureFromSignal(const Signal &signal, double hz) const;
    void rotateFeature(TFeature &feature, int rotation) const;
    double featureDistance(const TFeature &ref, const TFeature &other,
                           int rotation) const;
    int findBestRotation(const TFeature &ref, const TFeature &other) const;
    std::pair<int, double> findFineFrequency(const TFeature &rotated,
                                             int coarseCents);
    void getRemainingFeaturesForChannel(int channel, FeatureSet &fs);

    mutable std::map<string, int> m_outputs;
};


#endif
