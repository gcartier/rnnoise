plugin_c_args = ['-DHAVE_CONFIG_H']

cdata = configuration_data()
cdata.set_quoted('PACKAGE_VERSION', gst_version)
cdata.set_quoted('PACKAGE', 'rnnoise plugin')
cdata.set_quoted('GST_LICENSE', 'LGPL')
cdata.set_quoted('GST_API_VERSION', api_version)
cdata.set_quoted('GST_PACKAGE_NAME', 'RNNoise plugin')
cdata.set_quoted('GST_PACKAGE_ORIGIN', 'https://github.com/gcartier/rnnoise')
configure_file(output : 'config.h', configuration : cdata)

gstaudio_dep = dependency('gstreamer-audio-1.0',
    fallback: ['gst-plugins-base', 'audio_dep'])

rnnoise_sources = [
    'src/gstrnnoise.c',
]

library('gstrnnoise',
    rnnoise_sources,
    c_args: plugin_c_args,
    dependencies : [gst_dep, gstaudio_dep]
)
