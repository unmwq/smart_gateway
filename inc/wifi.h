#ifndef _WIFI_H
#define _WIFI_H
#include "usart.h"
#include "rtc.h"
#include "timer.h"
#include "stdlib.h"
#include <stdio.h>
#include "string.h"
#include "delay.h"
#include "detector.h"
#include "JQ6500.h"

#include "Tick.h"
#include "GenericTypeDefs.h"


#define         Set_Photo_PP    	0x10	//����ͼƬ����
#define         Make_Photo_flash  	0x11	//���մ汾��
#define         Camera_Detect_photo	0x12	//�ƶ����
#define         Get_Photo_Flash  	0x13	//��ȡflash�е���Ƭ
#define         delete_one_photo	0x14	//ɾ��һ����Ƭ
#define         delete_all_photo	0x15	//ɾ��������Ƭ
#define 		Set_Photo_Size		0x16	//����ͼƬ�ֱ���
#define 		upload_photoerror	0x00
#define         Put_Photo_info      0x18    //�ύ��Ƭ��Ϣ
#define         photo_send_ok       0x17   //�ش�ĳ֡
#define         Get_One_Page        0x19   //�ش�ĳ֡

#define			Battery_Power		0x70	//��ȡ��ص���
#define			WIFI_apr			0x71	//��ȡwifi�ź�

u8 handly_wifi_megs();
void  hand_wifi_mesg(void);
int wifi_sendchar(BYTE *buffer, int count);
int wifi_sendcmd(BYTE *buffer, int count,u8 cmd,u8 svrProfileId);

void check_wifi_mesg();
void Esp8266_Init();
void wif_pw_ctl();
void wifi_on();
void wifi_off();
void Wifi_udp_connect();
int handly_get_value(u16 timeout,u8 num);
int search(const char *s,const char b,u8 lens);
u8 check_IP_stat(u8 num,u8 timeout);
void  hand_wifi_err();
void check_wifi_apr();
int smart_link();
void Get_Battery_Wifi();
void handly_ctl_mode(u8 data,u8 info);
u32 wifi_Message_Apr(char *temp);
#endif