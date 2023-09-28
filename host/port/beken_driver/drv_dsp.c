#include <stdint.h>
#include "bkreg.h"
#include "drv_system.h"
#include "port_generic.h"

void dsp_code_data_copy(uint32_t type, uint32_t* dst, uint32_t* src, uint32_t len)
{
    uint32_t i;

    if(type == 0)
    {
        for(i = 0; i < len / 4; i++)
        {
            *dst++ = src[2];
            *dst++ = src[3];
            *dst++ = src[0];
            *dst++ = src[1];
            src += 4;
        }
    }
    else
    {
        for(i = 0; i < len; i++)
        {
            *dst++ = *src++;
        }
    }
}

void dsp_boot(void)
{
    #define DSP_CODE_START_ADDR         0x03200000
    #define DSP_DAT0_START_ADDR         0x03000000
    #define DSP_DAT1_START_ADDR         (0x01200000 + 96*1024)
    #define DSP_INTTBL_SIZE             0x300
    #define DSP_CODE_DATA_LOAD_OFFSET   (*((volatile unsigned int *)0x00000004))
    #define DSP_CODE_DATA_MAGIC_VALUE   (0x41564543)

    #define SWAP32(x)   (uint32_t)(((x) >> 24) | (((uint32_t)(x) & 0xff0000) >> 8) | (((uint32_t)(x) & 0xff00) << 8) | ((uint32_t)(x) << 24))

    //[C]: Cache, used for flash code
    //[T]: PTCM,  used for ram   code
    //[D]: DTCM,  used for ram   data
    //e.g. C128 means 128KB Cache
    enum DSP_MSS_MODE
    {
        DSP_MSS_MODE_C128_D512,
        DSP_MSS_MODE_C128_T128_D384,
        DSP_MSS_MODE_T128_D512,
        DSP_MSS_MODE_T256_D384,
    };

    uint32_t mode = DSP_MSS_MODE_C128_T128_D384;

    uint32_t* p = (uint32_t*)(DSP_CODE_DATA_LOAD_OFFSET + DSP_INTTBL_SIZE);
    uint32_t  dsp_boot_addr  = p[4];
    uint32_t  dsp_code1_addr = DSP_CODE_DATA_LOAD_OFFSET;
    uint32_t  dsp_code0_addr = dsp_code1_addr + p[8];
    uint32_t  dsp_data0_addr = dsp_code0_addr + p[9];
    uint32_t  dsp_data1_addr = dsp_data0_addr + p[10];
    uint32_t  dsp_code1_size = p[8];
    uint32_t  dsp_code0_size = p[9];
    uint32_t  dsp_data0_size = p[10];
    uint32_t  dsp_data1_size = p[11];
    uint32_t  dsp_ptcm_size  = dsp_code0_size;
    uint32_t  dsp_dtcm_size  = p[3];
    uint32_t  dsp_flsh_size  = p[12] - p[13];

    if(p[0] != DSP_CODE_DATA_MAGIC_VALUE)
    {
        LOG_I(BOOT, "No DSP code found @ 0x%08X\r\n", DSP_CODE_DATA_LOAD_OFFSET);
        return;
    }

    if(dsp_boot_addr != 0 && dsp_boot_addr != dsp_code1_addr)
    {
        LOG_I(BOOT, "DSP flash code at 0x%08X is not located @ the right address 0x%08X\r\n", dsp_code1_addr, dsp_boot_addr);
        return;
    }

    dsp_dtcm_size = SWAP32(dsp_dtcm_size);
    dsp_flsh_size = SWAP32(dsp_flsh_size);

    dsp_ptcm_size += 128 * 1024 - 1;
    dsp_ptcm_size /= 128 * 1024;
    dsp_ptcm_size *= 128;
    dsp_dtcm_size += 128 * 1024 - 1;
    dsp_dtcm_size /= 128 * 1024;
    dsp_dtcm_size *= 128;
    dsp_flsh_size  = dsp_code1_size > (dsp_flsh_size + 16) ? dsp_code1_size - dsp_flsh_size : 0;

    #if 0
    LOG_I(BOOT, "DSP addr(CODE0)  = 0x%08X, sizeof(CODE0) = %d\r\n", dsp_code0_addr, dsp_code0_size);
    LOG_I(BOOT, "DSP addr(CODE1)  = 0x%08X, sizeof(CODE1) = %d\r\n", dsp_code1_addr, dsp_code1_size);
    LOG_I(BOOT, "DSP addr(DATA0)  = 0x%08X, sizeof(DATA0) = %d\r\n", dsp_data0_addr, dsp_data0_size);
    LOG_I(BOOT, "DSP addr(DATA1)  = 0x%08X, sizeof(DATA1) = %d\r\n", dsp_data1_addr, dsp_data1_size);
    LOG_I(BOOT, "DSP sizeof(PTCM) = %d KB\r\n",    dsp_ptcm_size);
    LOG_I(BOOT, "DSP sizeof(DTCM) = %d KB\r\n",    dsp_dtcm_size);
    LOG_I(BOOT, "DSP sizeof(FLSH) = %d Bytes\r\n", dsp_flsh_size);
    #endif

    if(dsp_ptcm_size + dsp_dtcm_size > 640)
    {
        LOG_I(BOOT, "DSP PTCM plus DTCM overflow!!!\r\n");
        return;
    }

    if(dsp_dtcm_size > 384) mode &= 2;
    if(dsp_ptcm_size > 128) mode |= 2;
    if(dsp_ptcm_size && (mode == 0)) mode |= 2;

    LOG_I(BOOT, "DSP PMSS mode set as %d\r\n", mode);

    REG_SYSTEM_0x10  = 0;
    REG_SYSTEM_0x15  = 0;
    REG_SYSTEM_0x1D &= ~MSK_SYSTEM_0x1D_DSP_MSS_CFG;
    REG_SYSTEM_0x1D |= (mode << SFT_SYSTEM_0x1D_DSP_MSS_CFG);
    REG_SYSTEM_0x1F  = dsp_boot_addr;
    REG_SYSTEM_0x1D |= MSK_SYSTEM_0x1D_DSP_BOOT | MSK_SYSTEM_0x1D_DSP_CACHE_INV | MSK_SYSTEM_0x1D_DSP_EXT_WAIT;
    REG_SYSTEM_0x1D |= MSK_SYSTEM_0x1D_DSP_GLOBAL_RSTN | MSK_SYSTEM_0x1D_DSP_SYS_RSTN | MSK_SYSTEM_0x1D_DSP_OCEM_RSTN;

    //load code and data for DSP
    dsp_code_data_copy(0, (uint32_t*)DSP_CODE_START_ADDR, (uint32_t*)dsp_code0_addr, dsp_code0_size / 4);
    dsp_code_data_copy(1, (uint32_t*)DSP_DAT0_START_ADDR, (uint32_t*)dsp_data0_addr, dsp_data0_size / 4);
    dsp_code_data_copy(1, (uint32_t*)DSP_DAT1_START_ADDR, (uint32_t*)dsp_data1_addr, dsp_data1_size / 4);

    sys_delay_ms(1);

    REG_SYSTEM_0x1D |=  (MSK_SYSTEM_0x1D_DSP_CORE_RSTN);
    REG_SYSTEM_0x1D &= ~(MSK_SYSTEM_0x1D_DSP_BOOT | MSK_SYSTEM_0x1D_DSP_CACHE_INV | MSK_SYSTEM_0x1D_DSP_EXT_WAIT);
}

void dsp_shutdown(void)
{
    REG_SYSTEM_0x1D &= ~(0x3FFF << 4);  //Clear DSP Cfg
    REG_SYSTEM_0x15 |= MSK_SYSTEM_0x15_DSP_PWDN; // power down dsp
    REG_SYSTEM_0x10 |= MSK_SYSTEM_0x10_DSP_HALT; // close dsp clk
}

uint8_t dsp_is_working(void)
{
    return (REG_SYSTEM_0x15 == 0);
}