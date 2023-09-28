#ifndef _DRIVER_I2S_H
#define _DRIVER_I2S_H
#include <stdint.h>
#include <config/config.h>

/**
 * @brief I2S definition
 */
typedef enum
{
    I2S0,
    I2S1,
    I2S2,
}I2S;

/**
 * @brief I2S role definition
 */
typedef enum I2S_ROLE
{
    I2S_ROLE_MASTER,
    I2S_ROLE_SLAVE,
}I2S_ROLE_t;

/**
 * @brief I2S mode definition
 */
enum I2S_MODE
{
    I2S_MODE_PHILIP = 0,
    I2S_MODE_LEFT_JUSTIFIED,
    I2S_MODE_RIGHT_JUSTIFIED,
    I2S_MODE_SHORT_FRAME_SYNC,
    I2S_MODE_LONG_FRAME_SYNC,
    I2S_MODE_NORMAL_2B_D,
    I2S_MODE_DELAY_2B_D,
};

/**
 * @brief I2S error code definition
 */
enum I2S_ERROR_CODE
{
    I2S_ERROR_CODE_OK = 0,
    I2S_ERROR_CODE_INVALID_PARAMETER,
};

/**
 * @brief I2S control command definition
 */
enum I2S_CTRL_CMD
{
    I2S_CTRL_CMD_SET_SMP_RATIO,
    I2S_CTRL_CMD_SET_DLEN,
    I2S_CTRL_CMD_SET_BIT_WIDTH,
    I2S_CTRL_CMD_SET_SLEN,
    I2S_CTRL_CMD_SET_LSB_FIRST,
    I2S_CTRL_CMD_SET_BIT_CLK_INV,
    I2S_CTRL_CMD_SET_LR_CLK_INV,
    I2S_CTRL_CMD_SET_MODE,
    I2S_CTRL_CMD_CLR_RX_FIFO,
    I2S_CTRL_CMD_CLR_TX_FIFO,
    I2S_CTRL_CMD_SET_RX_THRESHOLD,
    I2S_CTRL_CMD_SET_TX_THRESHOLD,
    I2S_CTRL_CMD_SET_DOWN_SAMPLE_RATIO,
    I2S_CTRL_CMD_SET_BIT_SEQ_MODE,
    I2S_CTRL_CMD_SET_TX_FIFO_MODE,
	I2S_CTRL_CMD_RX_TRIG,
	I2S_CTRL_CMD_LR_COM_STORE,
	I2S_CTRL_CMD_PARALLEL_EN,
};

typedef struct _I2S_CFG_t{
    I2S I2Sx; //0,1,2
    I2S_ROLE_t role;//master or slave
    uint32_t sample_rate;
    uint8_t datawith;//16 or 24
    uint8_t clk_gate_dis;
    uint8_t lrck_inv_en;
    uint8_t bck_valid;
    uint8_t lrck_valid;
    uint8_t din_valid;
    uint8_t dout_valid;
    uint8_t dout2_valid;//only for i2s0
    uint8_t dout3_valid;//only for i2s0
}I2S_CFG_t;

#if(CONFIG_DRIVER_I2S)
#define I2S_INT_CODE_EN     0
/**
 * @brief I2S initialization
 * @param i I2S index
 * @param mode I2S mode, @see I2S_ROLE
 * @param sample_rate sampel rate
 * @param sample_width sample width
 * @param sample_ratio sample ratio
 * @return error code
 */
int32_t i2s_init(I2S i, uint32_t role, uint32_t sample_rate, uint32_t sample_width, uint32_t sample_ratio);

#define i2s_master_init(i, sample_rate, sample_width, sample_ratio)  i2s_init(i, I2S_ROLE_MASTER, sample_rate, sample_width, sample_ratio)
#define i2s_slaver_init(i, sample_rate, sample_width, sample_ratio)  i2s_init(i, I2S_ROLE_SLAVE,  sample_rate, sample_width, sample_ratio)

/**
 * @brief I2S control
 * @param i I2S index
 * @param cmd control command, @see I2S_CTRL_CMD
 * @param arg control argument
 * @return error code
 */
int32_t i2s_ctrl(I2S i, uint32_t cmd, uint32_t arg);

/**
 * @brief I2S enable
 * @param i I2S index
 * @param enable 0:disable, 1:enable
 * @return NULL
 */
void i2s_enable(I2S i, uint32_t enable);

uint8_t i2s_is_enable(void);

#if I2S_INT_CODE_EN
/**
 * @brief I2S rx interrupt enable
 * @param i I2S index
 * @param enable 0:disable, 1:enable
 * @return NULL
 */
void i2s_rx_int_enbale(I2S i, uint32_t enable);

/**
 * @brief I2S tx interrupt enable
 * @param i I2S index
 * @param enable 0:disable, 1:enable
 * @return NULL
 */
void i2s_tx_int_enbale(I2S i, uint32_t enable);

void i2s0_isr(void);
void i2s1_isr(void);
void i2s2_isr(void);
#endif //I2S_INT_CODE_EN

#endif
#endif
