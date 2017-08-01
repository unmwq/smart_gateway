#ifndef __STORE_H
#define __STORE_H	 
#include "main.h"

//RC 256K, PAGE=2K      ALL 128 PAGE
#define FLASH_SAVE_ADDRESS     0x0803F800//STM32F103RC

#define FLASH_STUDAT_ADDRESS     0x0803F800//STM32F103RC  

#define FLASH_DEVINFO_ADDRESS     0x0803FB10//FLASH_STUDAT_ADDRESS+240 ע�⣬���Ķ� NODE_CNT ʱ����Ҫ�Ķ��õ�ַ

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

#define  NODE_CNT 35//�洢�ڵ�����



#define IR_DATA  GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_1)//��ȡ΢������

typedef struct
{
 // unsigned char : 1;
 // unsigned char bIsCD_BodyIn : 1;    //=1:����������, =0:����������
 // unsigned char bIsIR_BodyIn : 1;       //=1:��������, =0:��������
//  unsigned char bIsMW_BodyIn : 1;       //=1:΢������, =0 ΢������
  unsigned char bIsRmt_Deploy ;      //=1:����, =0:����
 // unsigned char bIsMnc_ALAM : 1;        //�Ŵű���
 // unsigned char bIsIrf_ALARM : 1;       //���ⱨ��
 // unsigned char bIsOth_ALAM : 1;        //��������
 
}Alarm_Flags; 

//16 BYTE
__packed typedef  struct  
{
  u8  addr_433[2];// 2�ֽڵ�ַ
  u8  data_433[4][3];   // ң������1-4�ֽ����ݣ�ң��������ֵ; ������������һ���ֽ�����
  u8  Dev_Type;//�豸����:01 ң����  02 �Ŵ� 03 ���� 04 ����
  u8  Area;//��������ǰ�������ķ���
} RX_Data_433;//433ѧϰ��


/*
__packed typedef  struct  
{
  u8  Dev_Type;//�豸����:01 ң����  02 �Ŵ� 03 ���� 04 ����
  u8  addr_433[2];// 2�ֽڵ�ַ
  u8  data_433[3];   // ң������1-4�ֽ����ݣ�ң��������ֵ; ������������һ���ֽ�����
 
  //u8  ctl;//1Ϊ������0Ϊ������
} Rec_Data_433;//433ѧϰ��
*/

__packed typedef  struct 
{
  u8 area[5];
  u8 rec_data[5][5];
  u8 Send_433[5][60];//���5�������������ݣ�һ����������������5���豸
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
  u16 First_LTime;//��͵�ƽ����λ�á�ʱ��(15-13:�ֽ�λ�ã�12-10:λ λ��;9-10:ʱ��)
  u8  Low_Time;   // խ���� ʱ��
  u8  High_Time;  //������ ʱ��
} TX_Time_433;//433ѧϰ��

// 1+NODE_CNT*16+NODE_CNT*6=661 BYTE
__packed typedef  struct 
{
  u8 Count_433;//����ͳ���Ѿ��洢�˶���������
  
  RX_Data_433  addr_data_433[NODE_CNT];//������ѧϰ��ַ�롢����  3�ֽڣ�
  TX_Time_433  ad_time_433[NODE_CNT];//ѧϰ����433���ͬ��ͷʱ�䡢�����塢խ����ʱ�䣬3�ֽ�
} STOR_433_DATA;



//wifi��Ϣ 123Byte
__packed typedef  struct
{	
  u8 voice_flags;
  char Address[20]; //IP��ַ
  u16 Port;	// �˿�
  char ssid[50]; //�绰����־
  char password[50];
 
} WIFI_INFO;


//�豸��Ϣ,23BYTE
__packed typedef  struct  
{
  u8 DeviceName[9];//�豸���� Escout-4u 4c
  u8 DevcieID[4];//�豸ID
  u8 FirmwareVer[4];//�̼��汾 
  u8 SerialNO[8];//���к�
}DEVICE_INFO;


__packed typedef  struct 
{
  u8 Count_433;//����ͳ���Ѿ��洢�˶���������
  uint8 send_data[15][6];
 // RX_Data_433  addr_data_433[SEND_CNT];//������ѧϰ��ַ�롢����  3�ֽڣ�
 // TX_Time_433  ad_time_433[SEND_CNT];//ѧϰ����433���ͬ��ͷʱ�䡢�����塢խ����ʱ�䣬3�ֽ�
} SEND_433_DATA;

__packed typedef  struct 
{
  u8 en_disable;
  u8 pa_data[4][12];
} PA_data;


__packed typedef  struct 
{
  //u8  index;
  u16 s_l;//ͬ��ͷ�ĵ͵�ƽʱ��
  u16 s_h;//ͬ��ͷ�ĸߵ�ƽʱ��
  u8  num;//��ֵ����
  u16 l_h;//��������
  u16 l_l;//խ������
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