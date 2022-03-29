#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "rndenoiser.h"

#include <rnnoise.h>


GST_DEBUG_CATEGORY_STATIC(rndenoiser_debug);
#define GST_CAT_DEFAULT rndenoiser_debug


struct _GstRNDenoiser
{
    GstElement element;
    GstPad *sinkpad;
    GstPad *srcpad;
    GstAudioInfo info;
    gboolean denoise;
    gboolean voice;
    gboolean has_voice;
    DenoiseState* st;
};

G_DEFINE_TYPE(GstRNDenoiser, gst_rndenoiser,
    GST_TYPE_ELEMENT);


static GstElementClass *parent_class = NULL;


enum
{
    PROP_0,
    PROP_DENOISE,
    PROP_VOICE
};


#define SUPPORTED_CAPS_STRING                        \
    "audio/x-raw, "                                  \
    "format = (string) " GST_AUDIO_NE(S16) ", "      \
    "rate = " "48000" ", "                           \
    "channels = " "1"


static GstStaticPadTemplate gst_rndenoiser_src_template =
GST_STATIC_PAD_TEMPLATE("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS(SUPPORTED_CAPS_STRING)
    );

static GstStaticPadTemplate gst_rndenoiser_sink_template =
GST_STATIC_PAD_TEMPLATE("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS(SUPPORTED_CAPS_STRING)
    );


static void
gst_rndenoiser_set_property(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstRNDenoiser *denoiser = GST_RNDENOISER(object);

    GST_OBJECT_LOCK(denoiser);
    switch (prop_id) {
         case PROP_DENOISE:
            denoiser->denoise = g_value_get_boolean(value);
            break;
         case PROP_VOICE:
            denoiser->voice = g_value_get_boolean(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
    GST_OBJECT_UNLOCK(denoiser);
}


static void
gst_rndenoiser_get_property(GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
    GstRNDenoiser *denoiser = GST_RNDENOISER(object);

    GST_OBJECT_LOCK(denoiser);
    switch (prop_id) {
        case PROP_DENOISE:
            g_value_set_boolean(value, denoiser->denoise);
            break;
        case PROP_VOICE:
            g_value_set_boolean(value, denoiser->voice);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
    GST_OBJECT_UNLOCK(denoiser);
}


static gboolean
gst_rndenoiser_setcaps(GstRNDenoiser * rndenoiser, GstCaps * caps)
{
  GstAudioInfo info;

  if (!gst_audio_info_from_caps(&info, caps))
    goto wrong_caps;

  rndenoiser->info = info;

  return TRUE;

wrong_caps:
  {
    GST_DEBUG_OBJECT(rndenoiser, "could not parse caps");
    return FALSE;
  }
}


static gboolean
gst_rndenoiser_sink_event(GstPad * pad, GstObject * parent, GstEvent * event)
{
    gboolean res;
    GstRNDenoiser *rndenoiser;

    rndenoiser = GST_RNDENOISER(parent);

    switch (GST_EVENT_TYPE(event)) {
        case GST_EVENT_CAPS:
        {
            GstCaps *caps;
      
            gst_event_parse_caps(event, &caps);
            if ((res = gst_rndenoiser_setcaps(rndenoiser, caps))) {
                res = gst_pad_push_event(rndenoiser->srcpad, event);
            } else {
                gst_event_unref(event);
            }
            break;
        }  
        default:
            res = gst_pad_event_default(pad, parent, event);
            break;
    }

    return res;
}


static gboolean
gst_rndenoiser_src_event(GstPad * pad, GstObject * parent, GstEvent * event)
{
    GstRNDenoiser *rndenoiser;

    rndenoiser = GST_RNDENOISER(parent);
    
    return gst_pad_push_event(rndenoiser->sinkpad, event);
}


#define FRAME_SIZE 480

static float denoising[FRAME_SIZE];

static int filled = 0;


static GstFlowReturn
gst_rndenoiser_chain(GstPad * pad, GstObject * parent, GstBuffer * buf)
{
    GstRNDenoiser *rndenoiser;
    
    rndenoiser = GST_RNDENOISER(parent);
    
    if (! rndenoiser->denoise && ! rndenoiser->voice)
        return gst_pad_push(rndenoiser->srcpad, buf);
    else
    {
        GstMapInfo map;
        gst_buffer_map(buf, &map, GST_MAP_READ);
        
        int count = map.size / sizeof(gint16);
        gint16 *ptr = (gint16*) map.data;
        float *denoising_ptr = denoising + filled;
        int remain = count;
        
        while (remain > 0)
        {
            int avail = FRAME_SIZE - filled;
            
            if (remain < avail)
            {
                int i;
            
                for (i=0; i<remain; i++)
                    *denoising_ptr++ = *ptr++;
                
                filled += remain;
                remain = 0;
            }
            else
            {
                int i;
                float vad_prob;
            
                for (i=0; i<avail; i++)
                    *denoising_ptr++ = *ptr++;
                
                vad_prob = rnnoise_process_frame(rndenoiser->st, denoising, denoising);
                if (rndenoiser->voice)
                {
                    if (vad_prob >= .5)
                    {
                        if (! rndenoiser->has_voice)
                        {
                            rndenoiser->has_voice = TRUE;
                            gst_pad_push_event(rndenoiser->srcpad,
                                gst_event_new_custom(GST_EVENT_CUSTOM_DOWNSTREAM,
                                    gst_structure_new_empty("voice")));
                        }
                    }
                    else
                    {
                        if (rndenoiser->has_voice)
                        {
                            rndenoiser->has_voice = FALSE;
                            gst_pad_push_event(rndenoiser->srcpad,
                                gst_event_new_custom(GST_EVENT_CUSTOM_DOWNSTREAM,
                                    gst_structure_new_empty("silence")));
                        }
                    }
                }
                
                if (rndenoiser->denoise)
                {
                    GstBuffer *denoised_buffer = gst_buffer_new_allocate(NULL, FRAME_SIZE * sizeof(short), NULL);
                    GstMapInfo denoised_map;
                    gst_buffer_map(denoised_buffer, &denoised_map, GST_MAP_WRITE);
                    short* denoised_ptr = (short*) denoised_map.data;
                    
                    for (i=0; i<FRAME_SIZE; i++)
                        denoised_ptr[i] = denoising[i];
                    
                    GST_BUFFER_DTS(denoised_buffer) = GST_BUFFER_DTS(buf);
                    GST_BUFFER_PTS(denoised_buffer) = GST_BUFFER_PTS(buf);
                    GST_BUFFER_DURATION(denoised_buffer) = GST_BUFFER_DURATION(buf);
                    
                    gst_buffer_unmap(denoised_buffer, &denoised_map);
                    
                    gst_pad_push(rndenoiser->srcpad, denoised_buffer);
                }
                
                denoising_ptr = denoising;
                filled = 0;
                remain -= avail;
            }
        }
        
        gst_buffer_unmap(buf, &map);
        
        if (rndenoiser->denoise)
        {
            gst_buffer_unref(buf);
    
            return GST_FLOW_OK;
        }
        else
            return gst_pad_push(rndenoiser->srcpad, buf);
    }
}


static void
gst_rndenoiser_init(GstRNDenoiser * denoiser)
{
    denoiser->sinkpad = gst_pad_new_from_static_template(&gst_rndenoiser_sink_template, "sink");
    gst_pad_set_event_function(denoiser->sinkpad, gst_rndenoiser_sink_event);
    gst_pad_set_chain_function(denoiser->sinkpad, gst_rndenoiser_chain);
    GST_PAD_SET_PROXY_CAPS(denoiser->sinkpad);
    gst_element_add_pad(GST_ELEMENT(denoiser), denoiser->sinkpad);

    denoiser->srcpad = gst_pad_new_from_static_template(&gst_rndenoiser_src_template, "src");
    gst_pad_set_event_function(denoiser->srcpad, gst_rndenoiser_src_event);
    GST_PAD_SET_PROXY_CAPS(denoiser->srcpad);
    gst_element_add_pad(GST_ELEMENT(denoiser), denoiser->srcpad);

    denoiser->denoise = TRUE;
    denoiser->voice = FALSE;
    
    denoiser->has_voice = FALSE;
  
#ifdef _WIN32
    denoiser->st = rnnoise_create(NULL);
#else
    denoiser->st = rnnoise_create();
#endif
}


static void
gst_rndenoiser_finalize(GObject * obj)
{
    GstRNDenoiser *denoiser = GST_RNDENOISER(obj);
    
    rnnoise_destroy(denoiser->st);
}


static void
gst_rndenoiser_class_init(GstRNDenoiserClass * klass)
{
    GObjectClass *gobject_class;
    GstElementClass *element_class;

    gobject_class = (GObjectClass *) klass;
    element_class = (GstElementClass *) klass;
    
    parent_class = g_type_class_peek_parent(klass);

    gobject_class->finalize = gst_rndenoiser_finalize;
    gobject_class->set_property = gst_rndenoiser_set_property;
    gobject_class->get_property = gst_rndenoiser_get_property;

    GST_DEBUG_CATEGORY_INIT(rndenoiser_debug, "rndenoiser", 0,
        "Recurrent neural network denoiser");

    gst_element_class_set_details_simple(element_class,
        "RNDenoiser",
        "Filter/Denoiser/Audio",
        "Recurrent neural network denoiser",
        "Guillaume Cartier <gucartier@gmail.com>");

    g_object_class_install_property(gobject_class, PROP_DENOISE,
        g_param_spec_boolean ("denoise", "Denoise", "Enable denoising",
            TRUE,
            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_CONTROLLABLE));

    g_object_class_install_property(gobject_class, PROP_VOICE,
        g_param_spec_boolean ("voice", "Voice detection", "Enable voice activity detection",
            FALSE,
            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_CONTROLLABLE));

    gst_element_class_add_static_pad_template(element_class,
        &gst_rndenoiser_sink_template);
    gst_element_class_add_static_pad_template(element_class,
        &gst_rndenoiser_src_template);
}
