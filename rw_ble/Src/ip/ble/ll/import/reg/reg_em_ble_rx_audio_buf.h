#ifndef _REG_EM_BLE_RX_AUDIO_BUF_H_
#define _REG_EM_BLE_RX_AUDIO_BUF_H_

#include <stdint.h>
#include "_reg_em_ble_rx_audio_buf.h"
#include "compiler.h"
#include "arch.h"
#include "em_map.h"
#include "reg_access.h"

#define REG_EM_BLE_RX_AUDIO_BUF_COUNT 1

#define REG_EM_BLE_RX_AUDIO_BUF_DECODING_MASK 0x00000000

#define REG_EM_BLE_RX_AUDIO_BUF_ADDR_GET(idx) (EM_BLE_RX_AUDIO_BUFFER_OFFSET + (idx) * REG_EM_BLE_RX_AUDIO_BUF_SIZE)

/**
 * @brief RXAUDIOBUF register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:00           RXAUDIOBUF   0x0
 * </pre>
 */
#define EM_BLE_RXAUDIOBUF_ADDR   (0x01110000 + EM_BLE_RX_AUDIO_BUFFER_OFFSET)
#define EM_BLE_RXAUDIOBUF_INDEX  0x00000000
#define EM_BLE_RXAUDIOBUF_RESET  0x00000000
#define EM_BLE_RXAUDIOBUF_COUNT  32

__INLINE uint16_t em_ble_rxaudiobuf_get(int elt_idx, int reg_idx)
{
    ASSERT_ERR(reg_idx <= 31);
    return EM_BLE_RD(EM_BLE_RXAUDIOBUF_ADDR + elt_idx * REG_EM_BLE_RX_AUDIO_BUF_SIZE + reg_idx * 2);
}

__INLINE void em_ble_rxaudiobuf_set(int elt_idx, int reg_idx, uint16_t value)
{
    ASSERT_ERR(reg_idx <= 31);
    EM_BLE_WR(EM_BLE_RXAUDIOBUF_ADDR + elt_idx * REG_EM_BLE_RX_AUDIO_BUF_SIZE + reg_idx * 2, value);
}

// field definitions
#define EM_BLE_RXAUDIOBUF_MASK   ((uint16_t)0x0000FFFF)
#define EM_BLE_RXAUDIOBUF_LSB    0
#define EM_BLE_RXAUDIOBUF_WIDTH  ((uint16_t)0x00000010)

#define EM_BLE_RXAUDIOBUF_RST    0x0

__INLINE uint16_t em_ble_rxaudiobuf_getf(int elt_idx, int reg_idx)
{
    ASSERT_ERR(reg_idx <= 31);
    uint16_t localVal = EM_BLE_RD(EM_BLE_RXAUDIOBUF_ADDR + elt_idx * REG_EM_BLE_RX_AUDIO_BUF_SIZE + reg_idx * 2);
    ASSERT_ERR((localVal & ~((uint16_t)0x0000FFFF)) == 0);
    return (localVal >> 0);
}

__INLINE void em_ble_rxaudiobuf_setf(int elt_idx, int reg_idx, uint16_t rxaudiobuf)
{
    ASSERT_ERR(reg_idx <= 31);
    ASSERT_ERR((((uint16_t)rxaudiobuf << 0) & ~((uint16_t)0x0000FFFF)) == 0);
    EM_BLE_WR(EM_BLE_RXAUDIOBUF_ADDR + elt_idx * REG_EM_BLE_RX_AUDIO_BUF_SIZE + reg_idx * 2, (uint16_t)rxaudiobuf << 0);
}


#endif // _REG_EM_BLE_RX_AUDIO_BUF_H_

