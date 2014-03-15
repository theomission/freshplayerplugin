/*
 * Copyright © 2013-2014  Rinat Ibragimov
 *
 * This file is part of FreshPlayerPlugin.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "ppb_audio.h"
#include <stdlib.h>
#include "trace.h"
#include "pp_resource.h"


PP_Resource
ppb_audio_create(PP_Instance instance, PP_Resource config,
                 PPB_Audio_Callback_1_0 audio_callback, void *user_data)
{
    PP_Resource audio = pp_resource_allocate(PP_RESOURCE_AUDIO, instance);
    struct pp_audio_s *a = pp_resource_acquire(audio, PP_RESOURCE_AUDIO);
    if (!a)
        return 0;

    struct pp_audio_config_s *ac = pp_resource_acquire(config, PP_RESOURCE_AUDIO_CONFIG);
    if (!ac)
        goto err;

    a->sample_rate = ac->sample_rate;
    a->sample_frame_count = ac->sample_frame_count;

    snd_pcm_hw_params_t *hw_params;
    snd_pcm_sw_params_t *sw_params;

#define ERR_CHECK(errcode, funcname, jumpoutstatement)                          \
    if (errcode < 0) {                                                          \
        trace_error("%s, " #funcname ", %s\n", __func__, snd_strerror(errcode));\
        jumpoutstatement;                                                       \
    }

    int res;
    res = snd_pcm_open(&a->ph, "default", SND_PCM_STREAM_PLAYBACK, 0);
    ERR_CHECK(res, snd_pcm_open, goto err);

    res = snd_pcm_hw_params_malloc(&hw_params);
    ERR_CHECK(res, snd_pcm_hw_params_malloc, goto err);

    res = snd_pcm_hw_params_any(a->ph, hw_params);
    ERR_CHECK(res, snd_pcm_hw_params_any, goto err);

    res = snd_pcm_hw_params_set_access(a->ph, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    ERR_CHECK(res, snd_pcm_hw_params_set_access, goto err);

    res = snd_pcm_hw_params_set_format(a->ph, hw_params, SND_PCM_FORMAT_S16_LE);
    ERR_CHECK(res, snd_pcm_hw_params_set_format, goto err);

    res = snd_pcm_hw_params_set_rate_near(a->ph, hw_params, &a->sample_rate, 0);
    ERR_CHECK(res, snd_pcm_hw_params_set_rate_near, goto err);

    res = snd_pcm_hw_params_set_channels(a->ph, hw_params, 2);
    ERR_CHECK(res, snd_pcm_hw_params_set_channels, goto err);

    res = snd_pcm_hw_params(a->ph, hw_params);
    ERR_CHECK(res, snd_pcm_hw_params, goto err);

    snd_pcm_hw_params_free(hw_params);

    res = snd_pcm_sw_params_malloc(&sw_params);
    ERR_CHECK(res, snd_pcm_sw_params_malloc, goto err);

    res = snd_pcm_sw_params_current(a->ph, sw_params);
    ERR_CHECK(res, snd_pcm_sw_params_current, goto err);

    res = snd_pcm_sw_params_set_avail_min(a->ph, sw_params, a->sample_frame_count);
    ERR_CHECK(res, snd_pcm_sw_params_set_avail_min, goto err);

    res = snd_pcm_sw_params_set_start_threshold(a->ph, sw_params, 0U);
    ERR_CHECK(res, snd_pcm_sw_params_set_start_threshold, goto err);

    res = snd_pcm_sw_params(a->ph, sw_params);
    ERR_CHECK(res, snd_pcm_sw_params, goto err);

    res = snd_pcm_prepare(a->ph);
    ERR_CHECK(res, snd_pcm_prepare, goto err);

    snd_pcm_sw_params_free(sw_params);

#undef ERR_CHECK

    pp_resource_release(audio);
    return audio;
err:
    pp_resource_release(audio);
    pp_resource_expunge(audio);
    return 0;
}

void
ppb_audio_destroy(void *p)
{
    struct pp_audio_s *a = p;
    if (!a)
        return;
    snd_pcm_close(a->ph);
}

PP_Bool
ppb_audio_is_audio(PP_Resource resource)
{
    return pp_resource_get_type(resource) == PP_RESOURCE_AUDIO;
}

PP_Resource
ppb_audio_get_current_config(PP_Resource audio)
{
    return 0;
}

PP_Bool
ppb_audio_start_playback(PP_Resource audio)
{
    return PP_FALSE;
}

PP_Bool
ppb_audio_stop_playback(PP_Resource audio)
{
    return PP_FALSE;
}


// trace wrappers
static
PP_Resource
trace_ppb_audio_create(PP_Instance instance, PP_Resource config,
                       PPB_Audio_Callback_1_0 audio_callback, void *user_data)
{
    trace_info("[PPB] {full} %s\n", __func__+6);
    return ppb_audio_create(instance, config, audio_callback, user_data);
}

static
PP_Bool
trace_ppb_audio_is_audio(PP_Resource resource)
{
    trace_info("[PPB] {full} %s\n", __func__+6);
    return ppb_audio_is_audio(resource);
}

static
PP_Resource
trace_ppb_audio_get_current_config(PP_Resource audio)
{
    trace_info("[PPB] {zilch} %s\n", __func__+6);
    return ppb_audio_get_current_config(audio);
}

static
PP_Bool
trace_ppb_audio_start_playback(PP_Resource audio)
{
    trace_info("[PPB] {zilch} %s\n", __func__+6);
    return ppb_audio_start_playback(audio);
}

static
PP_Bool
trace_ppb_audio_stop_playback(PP_Resource audio)
{
    trace_info("[PPB] {zilch} %s\n", __func__+6);
    return ppb_audio_stop_playback(audio);
}

const struct PPB_Audio_1_0 ppb_audio_interface_1_0 = {
    .Create =           trace_ppb_audio_create,
    .IsAudio =          trace_ppb_audio_is_audio,
    .GetCurrentConfig = trace_ppb_audio_get_current_config,
    .StartPlayback =    trace_ppb_audio_start_playback,
    .StopPlayback =     trace_ppb_audio_stop_playback,
};
