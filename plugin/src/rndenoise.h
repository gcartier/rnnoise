#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <gst/gst.h>
#include <gst/audio/audio.h>
#include <gst/audio/gstaudiofilter.h>

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-function"
#endif


G_DECLARE_FINAL_TYPE(GstRNDenoise, gst_rndenoise,
    GST, RNDENOISE, GstAudioFilter)
