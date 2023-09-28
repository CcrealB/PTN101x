#ifndef _PARTHUS_USLC_TESTMODE_R2P_
#define _PARTHUS_USLC_TESTMODE_R2P_
void USLCtestmode_r2p_Initialise(void);
t_error USLCtestmode_r2p_Request(t_deviceIndex device_index, t_slots timeout, t_clock instance);
t_error USLCtestmode_r2p_Cancel(void);
void USLCtestmode_r2p_State_Dispatcher(BOOL dummy);
#endif
