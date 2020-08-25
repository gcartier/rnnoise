#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/audio/audio.h>
#include <gst/audio/gstaudiofilter.h>
#include <string.h>

#pragma GCC diagnostic ignored "-Wunused-function"

GST_DEBUG_CATEGORY_STATIC(rndenoise_debug);
#define GST_CAT_DEFAULT rndenoise_debug

G_DECLARE_FINAL_TYPE(GstRNDenoise, gst_rndenoise,
    GST, RNDENOISE, GstAudioFilter)

struct _GstRNDenoise
{
  GstAudioFilter audiofilter;
};

G_DEFINE_TYPE(GstRNDenoise, gst_rndenoise,
    GST_TYPE_AUDIO_FILTER);

static void gst_rndenoise_set_property(GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_rndenoise_get_property(GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);
static gboolean gst_rndenoise_setup(GstAudioFilter * filter, const GstAudioInfo * info);
static GstFlowReturn gst_rndenoise_transform_ip(GstBaseTransform * base_transform, GstBuffer * buf);

#define SUPPORTED_CAPS_STRING \
    GST_AUDIO_CAPS_MAKE(GST_AUDIO_NE(S16))

static void
gst_rndenoise_class_init(GstRNDenoiseClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *element_class;
  GstBaseTransformClass *btrans_class;
  GstAudioFilterClass *audio_filter_class;
  GstCaps *caps;

  gobject_class = (GObjectClass *) klass;
  element_class = (GstElementClass *) klass;
  btrans_class = (GstBaseTransformClass *) klass;
  audio_filter_class = (GstAudioFilterClass *) klass;

  gobject_class->set_property = gst_rndenoise_set_property;
  gobject_class->get_property = gst_rndenoise_get_property;

  audio_filter_class->setup = gst_rndenoise_setup;

  btrans_class->transform_ip = gst_rndenoise_transform_ip;
  
  gst_element_class_set_details_simple(element_class,
    "RNDenoise",
    "Filter/Denoiser/Audio",
    "Recurrent neural network denoiser",
    "Guillaume Cartier <gucartier@gmail.com>");

  caps = gst_caps_from_string(SUPPORTED_CAPS_STRING);
  gst_audio_filter_class_add_pad_templates(audio_filter_class, caps);
  gst_caps_unref(caps);
}

static void
gst_rndenoise_init(GstRNDenoise * filter)
{
}

static void
gst_rndenoise_set_property(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstRNDenoise *filter = GST_RNDENOISE(object);

  GST_OBJECT_LOCK(filter);
  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK(filter);
}

static void
gst_rndenoise_get_property(GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstRNDenoise *filter = GST_RNDENOISE(object);

  GST_OBJECT_LOCK(filter);
  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK(filter);
}

static gboolean
gst_rndenoise_setup(GstAudioFilter * filter,
    const GstAudioInfo * info)
{
  GstRNDenoise *denoise;
  GstAudioFormat fmt;
  gint chans, rate;

  denoise = GST_RNDENOISE(filter);

  rate = GST_AUDIO_INFO_RATE(info);
  chans = GST_AUDIO_INFO_CHANNELS(info);
  fmt = GST_AUDIO_INFO_FORMAT(info);

  GST_INFO_OBJECT(denoise, "format %d (%s), rate %d, %d channels",
      fmt, GST_AUDIO_INFO_NAME(info), rate, chans);

  /* if any setup needs to be done (like memory allocated), do it here */

  /* The audio filter base class also saves the audio info in
   * GST_AUDIO_FILTER_INFO(filter) so it's automatically available
   * later from there as well */

  return TRUE;
}

static GstFlowReturn
gst_rndenoise_transform_ip(GstBaseTransform * base_transform,
    GstBuffer * buf)
{
  GstRNDenoise *filter = GST_RNDENOISE(base_transform);
  GstFlowReturn flow = GST_FLOW_OK;
  GstMapInfo map;

  GST_LOG_OBJECT(filter, "transform buffer in place");

  /* FIXME: do something interesting here.  Doing nothing means the input
   * buffer is simply pushed out as is without any modification */
  if (gst_buffer_map(buf, &map, GST_MAP_READWRITE)) {
#if 0
    switch (GST_AUDIO_FILTER_FORMAT(filter)) {
      case GST_AUDIO_FORMAT_S16LE:
      case GST_AUDIO_FORMAT_S16BE: {
        gint16 *samples = map.data;
        guint n_samples = map.size / sizeof(gint16);
        guint i;

        for (i = 0; i < n; ++n) {
          samples[i] = samples[i];
        }
        break;
      }
      default:
        g_warning("Unexpected audio format %s!",
            GST_AUDIO_INFO_NAME(GST_AUDIO_FILTER_INFO(filter)));
        flow = GST_FLOW_ERROR;
        break;
    }
#endif
    gst_buffer_unmap(buf, &map);
  }

  return flow;
}

static gboolean
plugin_init(GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT(rndenoise_debug, "rndenoise", 0,
      "Recurrent neural network denoiser");

  return gst_element_register(plugin, "rndenoise", GST_RANK_NONE,
      gst_rndenoise_get_type());
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
