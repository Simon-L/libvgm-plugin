/*
 * ImGui plugin example
 * Copyright (C) 2021 Jean Pierre Cimalando <jp-dev@inbox.ru>
 * Copyright (C) 2021-2023 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: ISC
 */

#include "DistrhoPlugin.hpp"
#include "extra/ValueSmoother.hpp"

#include "DistrhoPluginUtils.hpp"

#include <string>
#include <list>
#include <iostream>
#include <filesystem>
#include <json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

START_NAMESPACE_DISTRHO


// --------------------------------------------------------------------------------------------------------------------

static constexpr const float CLAMP(float v, float min, float max)
{
    return std::min(max, std::max(min, v));
}

static constexpr const float DB_CO(float g)
{
    return g > -90.f ? std::pow(10.f, g * 0.05f) : 0.f;
}

// --------------------------------------------------------------------------------------------------------------------

class ImGuiPluginDSP : public Plugin
{
    enum Parameters {
        kParamGain = 0,
        kParamVoice,
        kParamCount
    };

    float fGainDB = 0.0f;
    int fVoice = 0;
    ExponentialValueSmoother fSmoothGain;

public:
   /**
      Plugin class constructor.@n
      You must set all parameter values to their defaults, matching ParameterRanges::def.
    */
    ImGuiPluginDSP()
        : Plugin(kParamCount, 0, 1) // parameters, programs, states
    {
        fSmoothGain.setSampleRate(getSampleRate());
        fSmoothGain.setTargetValue(DB_CO(0.f));
        fSmoothGain.setTimeConstant(0.020f); // 20ms

        // res = fs::path(getBinaryFilename()).parent_path().parent_path();
    }
    

protected:
    // ----------------------------------------------------------------------------------------------------------------
    // Information
    const char* getLabel() const noexcept override { return "__DPFLABEL__";}
    const char* getDescription() const override { return "__DPFDESCRIPTION__";}
    const char* getLicense() const noexcept override { return "ISC";}
    uint32_t getVersion() const noexcept override { return d_version(1, 0, 0);}
    int64_t getUniqueId() const noexcept override { return d_cconst('a', 'b', 'c', 'd');}
    const char* getMaker() const noexcept override { return "__YOURNAME__, Jean Pierre Cimalando, falkTX";}
    
    // ----------------------------------------------------------------------------------------------------------------
    // Init

    void initParameter(uint32_t index, Parameter& parameter) override
    {
        switch (index) {
          case 0:
            parameter.ranges.min = -90.0f;
            parameter.ranges.max = 30.0f;
            parameter.ranges.def = 0.0f;
            parameter.hints = kParameterIsAutomatable;
            parameter.name = "Gain";
            parameter.shortName = "Gain";
            parameter.symbol = "gain";
            parameter.unit = "dB";
            break;
          case 1:
            parameter.ranges.min = 0;
            parameter.ranges.max = 128;
            parameter.ranges.def = 0;
            parameter.hints = kParameterIsAutomatable|kParameterIsInteger;
            parameter.name = "Voice";
            parameter.shortName = "Voice";
            parameter.symbol = "voice";
            break;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Internal data
    
    /**
       Initialize the state @a index.@n
       This function will be called once, shortly after the plugin is created.@n
       Must be implemented by your plugin class only if DISTRHO_PLUGIN_WANT_STATE is enabled.
     */
    void initState(uint32_t index, State& state) override
    {
      // std::cout << "initState " << index << '\n';
      if (index == 0)
      {
        state.key = "file";
        state.defaultValue = "";
        state.hints = kStateIsFilenamePath;
      }
    }
    
    void setState(const char* key, const char* value) override
    {
      if (std::strcmp(key, "file") == 0)
      {
      }
    }
   /**
      Get the current value of a parameter.@n
      The host may call this function from any context, including realtime processing.
    */
    float getParameterValue(uint32_t index) const override
    {
        switch (index) {
          case 0:
            return fGainDB;
            break;
          case 1:
            return fVoice;
            break;
        }
    }

   /**
      Change a parameter value.@n
      The host may call this function from any context, including realtime processing.@n
      When a parameter is marked as automatable, you must ensure no non-realtime operations are performed.
      @note This function will only be called for parameter inputs.
    */
    void setParameterValue(uint32_t index, float value) override
    {
        switch (index) {
          case 0:
            fGainDB = value;
            fSmoothGain.setTargetValue(DB_CO(CLAMP(value, -90.0, 30.0)));
            break;
          case 1:
            fVoice = int(value);
            break;
        }

    }
    
    // ----------------------------------------------------------------------------------------------------------------
    // Audio/MIDI Processing

   /**
      Activate this plugin.
    */
    void activate() override
    {
        fSmoothGain.clearToTargetValue();
          
    }
    
#define EVENT_NOTEON 0x90
#define EVENT_NOTEOFF 0x80
#define EVENT_PITCHBEND 0xE0
#define EVENT_PGMCHANGE 0xC0
#define EVENT_CONTROLLER 0xB0
    
    void handleMidi(const MidiEvent* event)
    {   
      uint8_t b0 = event->data[0]; // status + channel
      uint8_t b0_status = b0 & 0xF0;
      uint8_t b0_channel = b0 & 0x0F;
      uint8_t b1 = event->data[1]; // note
      uint8_t b2 = event->data[2]; // velocity
      d_stdout("MIDI in 0x%x (status: 0x%x, channel: 0x%x) %d %d", b0, b0_status, b0_channel, b1, b2);
      
      switch (b0_status) {
        case EVENT_NOTEON:
          break;
        case EVENT_NOTEOFF:
          break;
        case EVENT_PITCHBEND:
          break;
        case EVENT_PGMCHANGE:
          break;
        case EVENT_CONTROLLER:
          break;
      }
    }
   /**
      Run/process function for plugins without MIDI input.
      @note Some parameters might be null if there are no audio inputs or outputs.
    */
    void run(const float** inputs, float** outputs, uint32_t frames, const MidiEvent* midiEvents, uint32_t midiEventCount) override
    {
        for (size_t i = 0; i < midiEventCount; i++) {
          handleMidi(&midiEvents[i]);
        }
        // get the left and right audio outputs
        float* const outL = outputs[0];
        float* const outR = outputs[1];
        
        // apply gain against all samples
        for (uint32_t i=0; i < frames; ++i)
        {
            const float gain = fSmoothGain.next();
            outL[i] = (0) * gain;
            outR[i] = (0) * gain;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Callbacks (optional)

   /**
      Optional callback to inform the plugin about a sample rate change.@n
      This function will only be called when the plugin is deactivated.
      @see getSampleRate()
    */
    void sampleRateChanged(double newSampleRate) override
    {
        fSmoothGain.setSampleRate(newSampleRate);
        std::cout << "SR changed to " << newSampleRate << '\n';
    }

    // ----------------------------------------------------------------------------------------------------------------

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImGuiPluginDSP)
};

// --------------------------------------------------------------------------------------------------------------------

Plugin* createPlugin()
{
    return new ImGuiPluginDSP();
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
