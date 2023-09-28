#ifndef _app_h_
#define _app_h_

#include "temp_cal.h"
#include "amg8802.h"
#include "amg8802_algo.h"
#include "amg8802_app.h"
#include "algorithm.h"

#define soft_vol 6

#define hard_vol 2

#define SN (unsigned long)0x1235782e2f135da2

typedef struct APP_FLAG
{
    unsigned char DFEWRITE:1;
    unsigned char FLASHOP:1;
    unsigned char FLAG1WRITE:1;
    unsigned char FLAG2WRITE:1;
    unsigned char FLAG3WRITE:1;
    unsigned char FLAG4WRITE:1;
}APP_FLAG;

extern void APP_Init(void);
extern void rec_config_to_write(void);

extern void convert_cellsdata_to_bmsdata(void);
extern void find_max_min_cell(void);
extern void AMG8802_Collection(void);
extern void Loop_main(void);
void battery_status(int num);

#endif
