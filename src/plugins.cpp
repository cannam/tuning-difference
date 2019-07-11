
#include <vamp/vamp.h>
#include <vamp-sdk/PluginAdapter.h>

#include "TuningDifference.h"

static Vamp::PluginAdapter<TuningDifference> tdAdapter;


const VampPluginDescriptor *
vampGetPluginDescriptor(unsigned int version, unsigned int index)
{
    if (version < 1) return 0;

    switch (index) {
    case  0: return tdAdapter.getDescriptor();
    default: return 0;
    }
}


