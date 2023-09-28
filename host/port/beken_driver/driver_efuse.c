
#include "bkreg.h"
#include "driver_efuse.h"
#include "drv_system.h"


#if EFUSE_EN

void eFuse_enable(uint8_t enable)
{
    if(enable)
    {
        system_peri_clk_enable(SYS_PERI_CLK_EFUSE);
        system_peri_clk_gating_disable(SYS_PERI_CLK_EFUSE);
        REG_SYSTEM_0x5B |= (1<<30);
        
    }
    else
    {
        system_peri_clk_disable(SYS_PERI_CLK_EFUSE);
        system_peri_clk_gating_enable(SYS_PERI_CLK_EFUSE);
        REG_SYSTEM_0x5B &= ~(1<<30);
    }
}
uint8_t eFuse_write(uint8_t* data,uint8_t addr,uint8_t len)
{
    uint8_t i;
    uint32_t reg=0;
    uint32_t delay = 0xffffff;
    if(addr+len>32)
        return 0;
    for(i=0;i<len;i++)
    {
        reg = (addr&0x1f) << SFT_EFUSE_0x00_OPRT_ADDR;
        reg |= (data[i]&0xff)<<SFT_EFUSE_0x00_OPRT_WDATA;
        reg |= MSK_EFUSE_0x00_VDD25_EN;
        reg |= MSK_EFUSE_0x00_OPRT_DIR;  // write
        REG_EFUSE_0x00 = reg;
        reg |= 1; // write enable
        REG_EFUSE_0x00 = reg;

        while(REG_EFUSE_0x00&MSK_EFUSE_0x00_OPRT_EN)  //!(BK3000_EFUSE_READ_REG&0x100)
        {
            delay--;
            if(delay==1)
                break;
        }
        if(REG_EFUSE_0x00&MSK_EFUSE_0x00_OPRT_EN)
        {
            return 0;
        }
        delay = 0xffffff;            
        addr++;
    }
    return 1;
}

uint8_t eFuse_read(uint8_t* data,uint8_t addr,uint8_t len) 
{
    uint8_t i;
    uint32_t reg=0;
    uint32_t delay = 0xffffff;
    if(addr+len>32)
        return 0;
    for(i=0;i<len;i++)
    {
        data[i]= 0xff;
        reg = (addr&0x1f) << SFT_EFUSE_0x00_OPRT_ADDR;
        reg |= MSK_EFUSE_0x00_OPRT_EN; // read enable
        REG_EFUSE_0x00 = reg;
        delay = 0xffffff;
        while((REG_EFUSE_0x00&MSK_EFUSE_0x00_OPRT_EN))
        {
            delay--;
            if(delay==1)
                break;
        }        
        if(REG_EFUSE_0x01&MSK_EFUSE_0x01_READ_DATA_VALID)
        {
            data[i] = REG_EFUSE_0x01&MSK_EFUSE_0x01_READ_DATA;
            //bk_printf("read efuse %d, value:0x%x\r\n",addr,data[i]);
        }
        else
        {
            //bk_printf("read efuse %d ERROR\r\n",addr);
            return 0;
        }
        addr++;
    }
    return 1;
}


#endif

