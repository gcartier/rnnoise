#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <gst/gst.h>
#include <gst/audio/audio.h>

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-function"
#endif


G_DECLARE_FINAL_TYPE(GstRNDenoiser, gst_rndenoiser,
    GST, RNDENOISER, GstElement)
