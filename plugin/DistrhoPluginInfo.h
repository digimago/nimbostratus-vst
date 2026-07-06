// Plugin metadata for the DPF wrapper around the Mutable Instruments
// Clouds DSP code (MIT licensed, by Emilie Gillet).

#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

// Display name only; internal ids keep the original "clouds" values so
// existing host sessions keep resolving to the same plugin.
#define DISTRHO_PLUGIN_BRAND    "DigiMago"
#define DISTRHO_PLUGIN_NAME     "Nimbostratus"
#define DISTRHO_PLUGIN_URI      "https://github.com/digimago/nimbostratus-vst"
#define DISTRHO_PLUGIN_CLAP_ID  "nl.digimago.clouds"

#define DISTRHO_PLUGIN_BRAND_ID  Dgmg
#define DISTRHO_PLUGIN_UNIQUE_ID dgCl

#define DISTRHO_PLUGIN_HAS_UI        1
#define DISTRHO_PLUGIN_IS_RT_SAFE    1
#define DISTRHO_PLUGIN_IS_SYNTH      0
#define DISTRHO_PLUGIN_NUM_INPUTS    2
#define DISTRHO_PLUGIN_NUM_OUTPUTS   2
#define DISTRHO_PLUGIN_WANT_LATENCY  1

#define DISTRHO_UI_USE_CUSTOM        1
#define DISTRHO_UI_CUSTOM_INCLUDE_PATH "DearImGui.hpp"
#define DISTRHO_UI_CUSTOM_WIDGET_TYPE  DGL_NAMESPACE::ImGuiTopLevelWidget
#define DISTRHO_UI_DEFAULT_WIDTH     820
#define DISTRHO_UI_DEFAULT_HEIGHT    380
#define DISTRHO_UI_USER_RESIZABLE    1

#endif // DISTRHO_PLUGIN_INFO_H_INCLUDED
