


## SDK log

### 230403 ptn101x_sdk_kara_v2520

- 增加宏： UART0_USB0_COEXIST   -- USB和串口0共存定义
  - 当使用PTN1012，如果电路上串口和USB0没有外部短路共用，定义为1，否则无需定义。
- 增加宏：SYS_LOG_PORT_SEL      -- 定义调试log输出口。
- 新增串口通用驱动，修改系统调试log的串口管理。


### 230327 ptn101x_sdk_kara_v2510

- 增加USPDIF, U盘播放功能(未完善)
- SDK相关说明：
  - 默认定义IC型号为1012：#define IC_MODEL                    IC_PTN1012
  - 如果要定义1011，可用如下语句重定义：
      #undef IC_MODEL  // redefine ic model to PTN1011
      #define IC_MODEL        IC_PTN1011

  USB功能相关宏定义修改：
      log：CONFIG_LOG_TO_UART1

  - SD卡：
      - 禁止插卡后自动切模式(默认打开)：CONFIG_SD_AUTO_MOD_SW_DIS
      - 禁止模式切后自动播放(默认打开)：CONFIG_SD_AUTO_PLAY_DIS

  - Udisk：
      - 禁止插盘后自动切模式(默认打开)： `udisk_mode_auto_sw_set(0)`
      - 禁止模式切后自动播放(默认打开)： `udisk_mode_auto_play_set(0)`
      - 特殊需求可在用户文件重定义U盘插入/移除回调函数：`usbh_udisk_init_cmp_callback() / usbh_udisk_lost_callback（）`
      - 判断U盘连接状态：`udisk_is_attached()`
          - 1:检测到U盘，并且驱动初始化完成。
          - 0:没检测到U盘，或检测到正在初始化，或检测到但初始化失败（后面要改为-1）。
      - enter_mode_wave_and_action中添加提示音ID

  - SPDIF：
      - 配置启用SPDIF：#define SPDIF_GPIO              GPIO12//GPIO11, GPIO12, GPIO14
      - 禁止检测后自动切模式(默认打开)： spdif_mode_auto_sw_set(0)
      - 禁止模式切后自动播放(默认打开)： spdif_mode_auto_play_set(0)
      - 判断SPDIF信号是否有效：`spdif_is_online()`
      - enter_mode_wave_and_action中添加 case & 提示音ID

### 230213 ptn101x_sdk_kara_v24

- Merge full_fun_v260C，AAC解码部分代码开放。

### 230212 ptn101x_sdk_kara_v23

- 音频流和DSP命令传输模式由 MailBox改为共享内存，降低MCU开销。

### 221224 ptn101x_sdk_kara_v21_221224

- DSP端增加独立的USB播放音频流。
- 蓝牙/SD的音频重采样挪到DSP端处理。
- SD卡驱动整理更新。

- 当前最新 git hash:e30d90c

### 221208 ptn101x_sdk_kara_v2_221208

- 【增加】用户BLE收发接口，服务ID：FFF0, 手机端接收特征ID:FFF1，发送特征ID：FFF2。
- 芯片端收发接口：[path:projects\app_full_fun\app\u_com.h]
  - void com_ble_send(uint8_t *buf, uint8_t size);
  - void com_ble_recv(uint8_t *buf, uint8_t size);


### ptn101_full_fun_karaok_221204_427f1f5_BLE.7z

MCU更新：
    - 内存分配：修改MCU内存81K->88K，动态内存：41K->40K。
        - 去掉了_ext_mem_xxx的6K内存，改用作主内存，后续和BK确认这块内存作用。
        - 修改了动态分配内存，由41K改为40K。
    - BLE代码加入编译。

BLE资源占用：（默认关闭SD卡）
    - SDK默认配置下，BLE代码空间占用约190KB，数据内存占用23KB。


##### 如何禁用BLE功能

1. 配置`host\config\config.h`中的宏 `BT_DUALMODE_RW` 为0.
2. 排除编译SDK下的`rw_ble`目录：在bstudio对应工程中右键选择`rw_ble/`目录 -> `C/C++ Build` -> 勾选`Exclude resouce from build`


### ptn101_full_fun_karaok_221201_45bf3da.7z

#### 更新说明

MCU:
    - 蓝牙异步时钟处理改为软件方式。
    - 加入SD卡音乐播放功能。
    - USB和BT/SD共存。
    - 修复：主频抬高后，部分芯片DSP无法启动的问题。
    - 增加SPI驱动。
    - 启用硬件FPU。
    - 蓝牙only sdc库更新。
DSP：
    - ADC/ANC写ringbuff可选中断方式。
    - 加密验证测试程序修改。
    - 音频ringbuff空间大小修改。

SDK：同步全功能V2.5.0.A版本，gitlab commit id: cfeddd8 [Date: Thu Sep 15 18:02:43 2022]


#### 操作说明


- 目前有效的宏：
  - CONFIG_DISK_SDCARD        -- 定义则启用SD卡功能
  - CONFIG_APP_PLAYER         -- 定义则使能播放器功能
  - CONFIG_SDCARD_DETECT      -- 定义则使能SD卡检测
  - CONFIG_SD_INSERT_MODE_SW  -- 插入SD卡便切到到SD卡模式
  - 蓝牙和USB共存的宏： BT_USB_OUT_COEXIST 【开了SD卡后需要一直打开】(删除@230219)
  - SDCARD_DETECT_IO			-- 定义SD卡拔插检测的GPIO，如:GPIO13
  - SDCARD_DETECT_LVL			-- 定义SD卡拔插检测的GPIO有效电平，一般为低：0

- SD相关函数接口：
  - 模式切换： system_work_mode_change_button
  - 播放，暂停：app_player_button_play_pause()
  - 上一曲： app_player_button_prev()
  - 下一曲： app_player_button_next()
  - 上一目录： app_player_button_prevDir()
  - 下一目录： app_player_button_nextDir()
  - 切换到SD卡模式： system_mode_shift_for_sd_change
  - 判断SD卡是否存在： sd_is_attached()， 返回: 1->存在，0->不存在
  - SD卡当前播放状态：player_get_play_status()， 1:正在播放， 0:没有播放。
  - 判断当前是否是SD卡模式： app_is_mp3_mode()， 返回: 1->是，0->不是

- 蓝牙相关函数接口：
  - 判断当前是否是蓝牙模式： app_is_bt_mode()， 返回: 1->是，0->不是
  - 蓝牙连接状态，当前是否播放音乐：
        app_handle_t sys_hdl = app_get_sys_handler();
        if(hci_get_acl_link_count(sys_hdl->unit))//如果是连接状态
        {
            if(a2dp_has_music())//有播放音乐
                app_set_led_event_action(LED_EVENT_BT_PLAY);
            else//没有播放音乐
                app_set_led_event_action(LED_EVENT_CONN);
        }


### ptn101_full_fun_karaok_220914_07668ca.7z

MCU+DSP:
    - DSP合并地址修改为600KB。
    - 支持单次发多条12byte的命令。
    - MCU对DSP内部的log控制功能。`dsp_log_ctrl()`
    - DSP的运行时间信息可在MCU端获取：`mcu2dsp_dsp_run_time_get()`
        - 返回32bit数，高16bit是效果器运行时间，低16bit是包含接口处理的音频帧处理总时间.

MCU:
    - 提示音分离到独立通道，提示音使用SBC软解。
    - 定时器改为1ms中断，不影响原有架构。
    - 关闭BLE功能。
    - USB audio播放和录音异步处理修复（限制约正负200ppm）。
    - 蓝牙音频流和USB音频流框架修改，统一在中断和DSP交互。


DSP:
    - 增加加密验证函数功能。
    - 蓝牙重采样部分挪到音频处理内部，运行时间计入打印。
    - I2S部分接口修改。
    


### 220804主要更新

DSP和MCU：
- 增加查询DSP接收准备接口：com_cmd_mcu2dsp_ready()
  - 【注意：用到了共享内存，没有互斥】
- 增加直接中断DSP立即处理命令接口
  - 可设置多条命令，形成命令list，DSP一次性更新参数。
  - 【串口测试发送过快也容易出问题，暂未细察】
- 增加另一种多条命令设置方式，DSP接收到fifo，循环中处理。
  - 【DSP端留了接口，MCU端暂未实现，需要MCU端设置共享内存，发送命令list，后续有需要再实现】

DSP：
- 音频链路接口更新。
  - 给算法提供所有单声道音频帧；
  - 所有输入/输出声道可映射任意接口的某输入/输出一声道。
- 所有音频buff定义到内部数据区。
- 内存配置为 128KB IRAM + 512KB DRAM。
- 增加DSP的log切换，可关闭DSP内部的log，或切换到硬件串口FIFO。
  - 【如果是串口B/C，需要先在MCU端初始化】

MCU：
- SarADC提供6个通道，每10ms刷新一个通道，实测误差正负50mV以内。

### 220714主要更新

```log
USB启用后打印口的切换功能，默认关闭USB。
USB功能模块，测试不带DSP，USB播放录音OK，HID回环OK，IDE可编译通过。【带DSP录音异常，看起来是声道数不匹配的问题】
ADC和DAC可单独初始化，所有MIC增益不配置。
配置DSP主频360M，Flash时钟4分频。

commit 3a3030827bed62b7d45097cc52ffaa43d116181e (HEAD -> main, origin/main, orig
in/HEAD)
Date:   Fri Jul 1 18:29:53 2022 +0800

    PTN101_Full_FUN_DesignKit _V2.5.0.9

    1、 修改写 flash 可能引起断线的问题
    2、 增加 RSSI 检测功能
    3、 修改连接 MP3 设备播放音乐无声问题
    4、 打开温度校准

```

### ptn101_full_fun_karaok_220524a_1ea2fe1

- 更新同步了原厂SDK版本PTN101_Full_FUN_DesignKit _V2.4.0.8

- 原厂SDK更新信息如下

```log
commit f17d70f6f2e54e77de6a80519b01ad3c870ccba9 (HEAD -> main, origin/main, orig
in/HEAD)
Date:   Thu May 19 16:40:52 2022 +0800

    PTN101_Full_FUN_DesignKit _V2.4.0.8

    更新：
    使能温度校准宏，温度变化较大时调整相关LDO及频偏；

commit 6de4d309dd6d138fa7a26f865b45fdd3843a88eb
Date:   Thu May 5 19:38:47 2022 +0800

    PTN101_Full_FUN_DesignKit _V2.4.0.7

    1、 修复提示音可能出现杂音的问题
    2、 修改 BLE 中断可能出现异常的情况
    3、 添加 A2DP HFP 单独断开的逻辑
    4、 修改打静电可能引起杂音问题
    5、 优化底噪问题
    6、 优化链接电脑可能出现的链接异常问题

```






### ptn101_full_fun_karaok_220524_5039641

- MCU
    - 增加同时支持GPIO按键和ADC按键
    - 增加MCU到DSP的命令接口示例(后续根据算法内部模块可能当前示例植入算法后会部分或全部无效)
        - void cmd_send_to_dsp(uint8_t mCmd, uint8_t sCmd, uint8_t* param, uint8_t size)
- DSP
    - 修复了2345通道MIC数据顺序混乱的问题



### ptn101_full_fun_karaok_220520_53928b8

- MCU修复提示音（重采样到48K传给DSP，合并在MCU到DSP的音乐通道）
- MCU增加支持ADC按键


- 220302: 

全功能工程: PTN101_Full_FUN_DesignKit_V2.2.0.4
支持CMAKE编译和 IDE 编译
