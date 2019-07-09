
#include <vamp/vamp.h>
#include <vamp-sdk/PluginAdapter.h>

#include "TuningDifference.h"
#include "BulkTuningDifference.h"

static Vamp::PluginAdapter<TuningDifference> tdAdapter;
static Vamp::PluginAdapter<BulkTuningDifference> bulkTdAdapter;


const VampPluginDescriptor *
vampGetPluginDescriptor(unsigned int version, unsigned int index)
{
    if (version < 1) return 0;

    switch (index) {
    case  0: return tdAdapter.getDescriptor();
    case  1: return bulkTdAdapter.getDescriptor();
    default: return 0;
    }
}


