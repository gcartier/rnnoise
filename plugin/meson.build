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

cc = meson.get_compiler('c')
if (host_machine.system() == 'darwin')
    rnnoise_inc = include_directories('../rnnoise/mac/include')
    rnnoise_dep = cc.find_library('rnnoise.0',
                    dirs : [meson.current_source_dir() + '/../rnnoise/mac/lib'])
elif (host_machine.system() == 'windows')
    rnnoise_inc = include_directories('../rnnoise/windows/include')
    rnnoise_dep = cc.find_library('rnnoise-0',
                    dirs : [meson.current_source_dir() + '/../rnnoise/windows/lib'])
endif

rnnoise_sources = [
    'src/gstrnnoise.c',
    'src/rndenoise.c',
    'src/rndenoiser.c'
]

library('gstrnnoise',
    rnnoise_sources,
    c_args: plugin_c_args,
    dependencies : [gst_dep, gstaudio_dep, rnnoise_dep],
    include_directories : rnnoise_inc
)
