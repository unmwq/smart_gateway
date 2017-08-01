#ifndef __STORE_H
#define __STORE_H	 
#include "main.h"

//RC 256K, PAGE=2K      ALL 128 PAGE
#define FLASH_SAVE_ADDRESS     0x0803F800//STM32F103RC

#define FLASH_STUDAT_ADDRESS     0x0803F800//STM32F103RC  

#define FLASH_DEVINFO_ADDRESS     0x0803FB10//FLASH_STUDAT_ADDRESS+240 注意，当改动 NODE_CNT 时，需要改动该地址

#define FLASH_DEVINFO_OFFSET  0x310//

#define FLASH_STUDAT_PAGE    127




#define FLASH_WIFINFO_OFFSET  0x310+0x60

#define FLASH_WIFINFO_ADDRESS    0x0803FB70//

#define FLASH_IR_PAGE    125
#define FLASH_IR_ADDRESS     0x0803E800//STM32F103RC
#define FLASH_IR_OFFSET  0x1F4//


#define FLASH_SEND_PAGE    126


#define FLASH_SEND_ADDRESS     0x0803F000//STM32F103RC

#define FLASH_Ctl_OFFSET  0x64//

#define FLASH_Ctl_ADDRESS     0x0803F064//STM32F103RC

#define FLASH_Scenar_OFFSET  0x64+0x150//

#define FLASH_Scenar_ADDRESS     (0x0803F064+0x150)//STM32F103RC
#define FLASH_PAdata_OFFSET   0x1B4+0x160

#define FLASH_PAdata_ADDRESS   0x0803F314

#define  NODE_CNT 35//存储节点总数



#define IR_DATA  GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_1)//读取微波输入

typedef struct
{
 // unsigned char : 1;
 // unsigned char bIsCD_BodyIn : 1;    //=1:超声波有人, =0:超声波无人
 // unsigned char bIsIR_BodyIn : 1;       //=1:红外有人, =0:红外无人
//  unsigned char bIsMW_BodyIn : 1;       //=1:微波有人, =0 微波无人
  unsigned char bIsRmt_Deploy ;      //=1:布防, =0:撤防
 // unsigned char bIsMnc_ALAM : 1;        //门磁报警
 // unsigned char bIsIrf_ALARM : 1;       //红外报警
 // unsigned char bIsOth_ALAM : 1;        //其它报警
 
}Alarm_Flags; 

//16 BYTE
__packed typedef  struct  
{
  u8  addr_433[2];// 2字节地址
  u8  data_433[4][3];   // 遥控器，1-4字节数据，遥控器按键值; 其它传感器，一个字节数据
  u8  Dev_Type;//设备类型:01 遥控器  02 门磁 03 红外 04 其它
  u8  Area;//防区，当前传感器的防区
} RX_Data_433;//433学习码


/*
__packed typedef  struct  
{
  u8  Dev_Type;//设备类型:01 遥控器  02 门磁 03 红外 04 其它
  u8  addr_433[2];// 2字节地址
  u8  data_433[3];   // 遥控器，1-4字节数据，遥控器按键值; 其它传感器，一个字节数据
 
  //u8  ctl;//1为报警，0为不报警
} Rec_Data_433;//433学习码
*/

__packed typedef  struct 
{
  u8 area[5];
  u8 rec_data[5][5];
  u8 Send_433[5][60];//最多5条关联控制数据，一个关联数据最多关联5个设备
}Ctl_Relation;

__packed typedef  struct 
{
  u8 enable_num;
  u8 area[5][6]; 
  u8 Send_433[5][60];
}Scenario_mode;

// 6BYTE
__packed typedef  struct  
{
  u16  Syno_Time;// Synchronous head time
  u16 First_LTime;//宽低电平出现位置、时长(15-13:字节位置；12-10:位 位置;9-10:时长)
  u8  Low_Time;   // 窄脉冲 时长
  u8  High_Time;  //宽脉冲 时长
} TX_Time_433;//433学习码

// 1+NODE_CNT*16+NODE_CNT*6=661 BYTE
__packed typedef  struct 
{
  u8 Count_433;//用于统计已经存储了多少组数据
  
  RX_Data_433  addr_data_433[NODE_CNT];//红外自学习地址码、数据  3字节；
  TX_Time_433  ad_time_433[NODE_CNT];//学习到的433码的同步头时间、宽脉冲、窄脉冲时间，3字节
} STOR_433_DATA;



//wifi信息 123Byte
__packed typedef  struct
{	
  u8 voice_flags;
  char Address[20]; //IP地址
  u16 Port;	// 端口
  char ssid[50]; //电话薄标志
  char password[50];
 
} WIFI_INFO;


//设备信息,23BYTE
__packed typedef  struct  
{
  u8 DeviceName[9];//设备名称 Escout-4u 4c
  u8 DevcieID[4];//设备ID
  u8 FirmwareVer[4];//固件版本 
  u8 SerialNO[8];//序列号
}DEVICE_INFO;


__packed typedef  struct 
{
  u8 Count_433;//用于统计已经存储了多少组数据
  uint8 send_data[15][6];
 // RX_Data_433  addr_data_433[SEND_CNT];//红外自学习地址码、数据  3字节；
 // TX_Time_433  ad_time_433[SEND_CNT];//学习到的433码的同步头时间、宽脉冲、窄脉冲时间，3字节
} SEND_433_DATA;

__packed typedef  struct 
{
  u8 en_disable;
  u8 pa_data[4][12];
} PA_data;


__packed typedef  struct 
{
  //u8  index;
  u16 s_l;//同步头的低电平时间
  u16 s_h;//同步头的高电平时间
  u8  num;//码值个数
  u16 l_h;//宽脉冲宽带
  u16 l_l;//窄脉冲宽带
  u8  data[10];
} IR_info;


__packed typedef  struct 
{
u8  IR_info[5][100];
} IR_Data;


void InitAlarmFlags(void);
BOOL Save433StudyDat(STOR_433_DATA *pStudyDat);
BOOL SaveDevInfo(DEVICE_INFO *pDevInfo);
BOOL Load433StudyDat(STOR_433_DATA *pStudyDat);
BOOL LoadDevInfo(DEVICE_INFO *pDevInfo);
BOOL SaveScenario_mode(Scenario_mode *pS_MODE);

u8 ReadStuDatCount(STOR_433_DATA *pStudyDat);

void InitStuDat(STOR_433_DATA *pStudyDat);
void InitDevInfo(DEVICE_INFO *pDevInfo);

void InitStudatDefault(STOR_433_DATA *pStudyDat);
void InitDevInfoDefault(DEVICE_INFO *pDevInfo);

BOOL SaveCtl_Relation(Ctl_Relation *pCtl_R);
extern STOR_433_DATA StudyDat;
extern DEVICE_INFO Deviceinfo;

int check_sensor_remote(u8 *data);
void InitStuDat(STOR_433_DATA *pStudyDat);
void InitDevInfo(DEVICE_INFO *pDevInfo);

void InitDevInfoDefault(DEVICE_INFO *pDevInfo);
void InitStudatDefault(STOR_433_DATA *pStudyDat);

BOOL Save433StudyDat(STOR_433_DATA *pStudyDat);
BOOL SaveDevInfo(DEVICE_INFO *pDevInfo);
BOOL SaveWifiInfo(WIFI_INFO *pWifi_info);
BOOL Load433StudyDat(STOR_433_DATA *pStudyDat);
void InitWifInfo(WIFI_INFO *wifi_info);


void InitIRdatDefault(IR_Data *pIR_data);
BOOL Save_ir_data(IR_Data *pIr_data);
BOOL LoadIRDat(IR_Data *pIR_data);

BOOL Load433SendDat(SEND_433_DATA *pSendDat);
BOOL CheckSendDatExist(void);
BOOL Save433SendDat(SEND_433_DATA *pSendDat);







#endif 