#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "rndenoiser.h"


static gboolean
plugin_init(GstPlugin * plugin)
{
    if (!gst_element_register(plugin, "rndenoiser", GST_RANK_NONE, gst_rndenoiser_get_type()))
        return FALSE;
    
    return TRUE;
}


GST_PLUGIN_DEFINE(
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    rnnoise,
    "RNNoise plugin",
    plugin_init,
    PACKAGE_VERSION,
    GST_LICENSE,
    GST_PACKAGE_NAME,
    GST_PACKAGE_ORIGIN
);
