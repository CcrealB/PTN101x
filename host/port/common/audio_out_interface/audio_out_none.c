#include <audio_out_interface.h>
#include "driver_beken_includes.h"
#include "app_beken_includes.h"

result_t audio_out_open(int32_t rate,
                            int32_t channels,
                            int32_t bits_per_sample,
                            audio_out_ctx_h *audio_out_ctx)
{
    *audio_out_ctx = (void *)((uint8_t *)NULL + 1);

    return UWE_OK;
}

void audio_out_close(audio_out_ctx_h *audio_out_ctx)
{
    *audio_out_ctx = NULL;
}

result_t audio_out_write(audio_out_ctx_h audio_out_ctx, uint8_t *buffer,
    j_size_t size)
{
    aud_dac_fill_buffer(buffer, size);
    aud_dac_open();
    return UWE_OK;
}
// EOF
