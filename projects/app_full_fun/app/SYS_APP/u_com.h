
#ifndef _U_COM_H_
#define _U_COM_H_


void com_cmd_send_proc(uint8_t *buf,uint8_t len);


void com_ble_send(uint8_t *buf, uint8_t size);
void com_ble_recv(uint8_t *buf, uint8_t size);

#endif /* _U_COM_H_ */
