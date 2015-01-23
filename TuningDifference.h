#ifndef TUNING_DIFFERENCE_H
#define TUNING_DIFFERENCE_H

#include <vamp-sdk/Plugin.h>

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
    int m_blockSize;
    vector<double> m_sum[2];
    int m_frameCount;
};


#endif
