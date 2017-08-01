#ifndef __DETECTOR_H
#define __DETECTOR_H
#include "GenericTypeDefs.h"
#include "stm32f10x.h"
#include <string.h>
#include "main.h"


#define DATA_LOW  GPIO_ResetBits(GPIOB, GPIO_Pin_9)  //433 out
#define DATA_HIGH GPIO_SetBits(GPIOB, GPIO_Pin_9)

//#define CSIO_DATA  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_7)//433 in
//#define CSIO_DATA_1 GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_6)//433 in
#define CSIO_DATA  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_8)//433 in
#define IR_BODY    (GPIO_Pin_1)
//#define IR_DATA  GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_1)//��ȡ��������

#define MW_BODY    (GPIO_Pin_2)
#define MW_DATA  GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_2)//��ȡ΢������


//������ PA11--����ߵ�ƽ����ģ�鹤��
//       PA12--����ߵ�ƽ����ʱ�䣬�㳤��

#define Udata_cnt 7 //�������˲�����


enum leix//ѧϰģʽ������ģʽ
{
  study=1, changgui
};

enum  Sensor//ѧϰ����������
{
  remote=1,Menci,Infrared,smoke,gas,arlrm_calling,
};



void hand_pa_send(u8 i);


void handsave_guanlin(u8 *data);

void save_Scenar(u8 *data);


void send_433data(u8 *data,u8 times);

void get_back_study();
void Save_433_flash(u8 memu1,u8 memu2,u8 StudyCntAddr,u8 sensor,u8 typeinfo);

void Detector_Init(void);
void Jiema(uint8 leixing);
void Study_Remote (void);//ң����
void Study_Menci (void);//�Ŵ�
void Study_Infrared (void);//����
void Study_Other (void);//����
void Study(u8 sensor,u8 typeinfo);
void Delete_Sensor(u8 sensor);
void Delete_OneSensor(void);  
void Delete_AllSensor(void);
int Changgui(void);
u16 GenerateRandomU16(void);//����adc�������������
void MakeDeviceID(u8 *pdeviceID);//����ÿ��cpu id��Ψһ�ԣ�����ÿ���豸��ID
int wxkg_study();


BOOL CheckStuDatExist(void);
BOOL CheckDevInfoExist(void);

uint8 body_detect(void);
void bubble_sort(u16 * pData,u8 n);
u16 ultrasonic_detect(u8 detectcnt);
u16 Get_Adc_Average(u8 times);
u16 filter16(u16 meter_data,u16 *ptr);
void Alarm_Handler(void);
void net_sendstats(u8 *ptemp,u8 num);

#endif
