#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "rndenoise.h"

#include <rnnoise.h>


GST_DEBUG_CATEGORY_STATIC(rndenoise_debug);
#define GST_CAT_DEFAULT rndenoise_debug


struct _GstRNDenoise
{
    GstAudioFilter audiofilter;
    gboolean denoise;
    DenoiseState* st;
};

G_DEFINE_TYPE(GstRNDenoise, gst_rndenoise,
    GST_TYPE_AUDIO_FILTER);


static GstElementClass *parent_class = NULL;


enum
{
    PROP_0,
    PROP_DENOISE
};


#define SUPPORTED_CAPS_STRING                        \
    "audio/x-raw, "                                  \
    "format = (string) " GST_AUDIO_NE(S16) ", "      \
    "rate = " "48000" ", "                           \
    "channels = " "1"


static void
gst_rndenoise_set_property(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstRNDenoise *denoise = GST_RNDENOISE(object);

    GST_OBJECT_LOCK(denoise);
    switch (prop_id) {
         case PROP_DENOISE:
            denoise->denoise = g_value_get_boolean(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
    GST_OBJECT_UNLOCK(denoise);
}


static void
gst_rndenoise_get_property(GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
    GstRNDenoise *denoise = GST_RNDENOISE(object);

    GST_OBJECT_LOCK(denoise);
    switch (prop_id) {
        case PROP_DENOISE:
            g_value_set_boolean(value, denoise->denoise);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
    GST_OBJECT_UNLOCK(denoise);
}


static gboolean
gst_rndenoise_start(GstBaseTransform * base_transform)
{
    GstRNDenoise *denoise = GST_RNDENOISE(base_transform);
  
    denoise->st = rnnoise_create();
  
    return 1;
}


static gboolean
gst_rndenoise_stop(GstBaseTransform * base_transform)
{
    GstRNDenoise *denoise = GST_RNDENOISE(base_transform);
  
    rnnoise_destroy(denoise->st);
  
    return 1;
}


static GstFlowReturn
gst_rndenoise_prepare_output_buffer(GstBaseTransform * base_transform,
    GstBuffer * inbuf, GstBuffer ** outbuf)
{
    *outbuf = gst_buffer_new_allocate(NULL, 10000, NULL);
    
    return GST_FLOW_OK;
}


#define FRAME_SIZE 480

static GstFlowReturn
gst_rndenoise_transform(GstBaseTransform * base_transform,
    GstBuffer * inbuf, GstBuffer * outbuf)
{
    float x[FRAME_SIZE];
    
    GstRNDenoise *denoise = GST_RNDENOISE(base_transform);
    GstMapInfo map_in;
    GstMapInfo map_out;

    GST_LOG_OBJECT(denoise, "transform buffer");

    if (!denoise->denoise) {
        if (gst_buffer_map(inbuf, &map_in, GST_MAP_READ)) {
            if (gst_buffer_map(outbuf, &map_out, GST_MAP_WRITE)) {
                g_assert(map_out.size == map_in.size);
                memcpy(map_out.data, map_in.data, map_out.size);
                gst_buffer_unmap(outbuf, &map_out);
            }
            gst_buffer_unmap(inbuf, &map_in);
        }
        
        return GST_FLOW_OK;
    }
    else {
        if (gst_buffer_map(inbuf, &map_in, GST_MAP_READ)) {
            if (gst_buffer_map(outbuf, &map_out, GST_MAP_WRITE)) {
                g_assert(map_out.size == map_in.size);
                g_assert(map_out.size == FRAME_SIZE * sizeof(gint16));
                gint16* ptr_in = (gint16*) map_in.data;
                gint16* ptr_out = (gint16*) map_out.data;
                float* ptr_x = x;
                int i;
                for (i=0; i<FRAME_SIZE; i++)
                    *ptr_x++ = *ptr_in++;
                rnnoise_process_frame(denoise->st, x, x);
                ptr_x = x;
                for (i=0; i<FRAME_SIZE; i++)
                    *ptr_out++ = *ptr_x++;
                // memcpy(map_out.data, map_in.data, map_out.size);
                gst_buffer_unmap(outbuf, &map_out);
            }
            gst_buffer_unmap(inbuf, &map_in);
        }
        
        return GST_FLOW_OK;
    }
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

    return TRUE;
}


static void
gst_rndenoise_class_init(GstRNDenoiseClass * klass)
{
    GObjectClass *gobject_class;
    GstElementClass *element_class;
    GstBaseTransformClass *base_transform_class;
    GstAudioFilterClass *audio_filter_class;
    GstCaps *caps;

    gobject_class = (GObjectClass *) klass;
    element_class = (GstElementClass *) klass;
    base_transform_class = (GstBaseTransformClass *) klass;
    audio_filter_class = (GstAudioFilterClass *) klass;
    
    parent_class = g_type_class_peek_parent(klass);

    gobject_class->set_property = gst_rndenoise_set_property;
    gobject_class->get_property = gst_rndenoise_get_property;

    base_transform_class->start = gst_rndenoise_start;
    base_transform_class->stop = gst_rndenoise_stop;
    // base_transform_class->prepare_output_buffer = gst_rndenoise_prepare_output_buffer;
    base_transform_class->transform = gst_rndenoise_transform;

    audio_filter_class->setup = gst_rndenoise_setup;

    GST_DEBUG_CATEGORY_INIT(rndenoise_debug, "rndenoise", 0,
        "Recurrent neural network denoiser");

    gst_element_class_set_details_simple(element_class,
        "RNDenoise",
        "Filter/Denoiser/Audio",
        "Recurrent neural network denoiser",
        "Guillaume Cartier <gucartier@gmail.com>");

    g_object_class_install_property(gobject_class, PROP_DENOISE,
        g_param_spec_boolean ("denoise", "Denoise", "Enable denoising",
            TRUE,
            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_CONTROLLABLE));

    caps = gst_caps_from_string(SUPPORTED_CAPS_STRING);
    gst_audio_filter_class_add_pad_templates(audio_filter_class, caps);
    gst_caps_unref(caps);
}


static void
gst_rndenoise_init(GstRNDenoise * denoise)
{
    denoise->denoise = TRUE;
}
