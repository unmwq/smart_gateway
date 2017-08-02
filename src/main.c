/**
******************************************************************************

******************************************************************************
*/ 

/****************************************Copyright (c)**************************************************
**�������� ����Ҫʵ�ֹ��ܵĸ�����
********************************************************************************************************/
/******************************************************************************************************/
//#include "Core_cm3.h"
#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


u32 ID;

extern u8 up_stats_time;
extern int zk;

extern Alarm_Flags AlarmFlags;
extern WIFI_INFO wifi_info;
extern char rpdata[100];
extern u8 SIM_flags;
extern SEND_433_DATA   SendDat;

extern IR_Data  ir_data;  

extern Ctl_Relation    C_Relation;
extern Scenario_mode   S_MODE;
extern PA_data    PA_DATA;

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

//char addr[35]="yellowpet.xicp.net";
//uint16 port=8002;

void CPU_IntDis (void)
{
  __ASM("CPSID I");
  //__ASM("BX LR");
}


void CPU_IntEn (void)
{                                                                                       
  __ASM("CPSIE I");
  //__ASM("BX LR");
}


void cpuinit(void)
{
  
  GPIO_InitTypeDef GPIO_InitStr;
  //SPI_Cmd(SPI2, DISABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB, ENABLE);
  
  GPIO_InitStr.GPIO_Pin = GPIO_Pin_8;
  GPIO_InitStr.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStr.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOA, &GPIO_InitStr);
  GPIO_ResetBits(GPIOA,GPIO_Pin_8);
  
  //GPIO_InitStr.GPIO_Pin = GPIO_Pin_6;
  //GPIO_Init(GPIOB, &GPIO_InitStr);
  //  GPIO_ResetBits(GPIOB,GPIO_Pin_6);
  
  GPIO_InitStr.GPIO_Pin = GPIO_Pin_11;
  GPIO_InitStr.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_Init(GPIOA, &GPIO_InitStr);
}	

void put_devinfo()
{
  
  
  u8 temp[10]={0};
  printf("ID=0x%x\r\n",ID);
  
  memset(&temp,0,sizeof(temp));
  memcpy(&temp,&Deviceinfo.DeviceName,sizeof(Deviceinfo.DeviceName));
  
  printf("DeviceName:%s\r\n",temp);
  
  memset(&temp,0,sizeof(temp));
  memcpy(&temp,&Deviceinfo.FirmwareVer,sizeof(Deviceinfo.FirmwareVer));
  printf("FirmwareVer:%s\r\n",temp);
  
  memset(&temp,0,sizeof(temp));
  memcpy(&temp,&Deviceinfo.SerialNO,sizeof(Deviceinfo.SerialNO));
  printf("SerialNO:%s|\r\n",temp);
  
  
} 

void Get_ID_formflash()
{
  char getid[9]={0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39};
  if(strstr(rpdata,getid)!=NULL)
  {
    put_devinfo();
    zk=0;
    memset(rpdata,0,sizeof(rpdata));
  }
}



int main (void)
{ 
  u16 th;
  u16 sum;
  
  //����������Ҫ���͵����ݣ��ɷ���ȫ����
  // u8 sztmp[50];
  SystemInit();
  
  CPU_IntDis();
  CPU_IntEn();
  UARTBUF_Init();
  RCC_Configuration(); 
  TickInit();
  
  AllUart_Init();
  TIM1_Config();      //PWM 38Khz�ز�                           //((1+TIM_Prescaler )/72M)*(1+TIM_Period )=((1+651)/72M)*(1+5000)=0.0452s=45.2ms
  TIM2_Int_Init(9999,7199);   //һ�����һ�Σ���Ϊϵͳʱ��Ŀɿ����ݣ����RTC��ʱ��
  TIM3_Int_Init(5000,651);    //for 433 study ÿ�ۼ�һ�Σ�9.04us,5000�Σ�45.20ms 651                        //((1+TIM_Prescaler )/72M)*(1+TIM_Period )=((1+651)/72M)*(1+5000)=0.0452s=45.2ms
  TIM4_Int_Init(5000,651);   //for study ң����
  TIM5_Int_Init(50000,72);  //�������� ����
  //Temp_Humi_Init();
  
  Adc_Init();
  
  InitStuDat( &StudyDat );
  InitDevInfo( &Deviceinfo );
  InitSendDat( &SendDat );
  InitWifInfo(&wifi_info);
  
  InitCtl_Relation( &C_Relation );
  InitScenario_mode( &S_MODE);
  InitIRDat(&ir_data);
  
  InitPA_data(&PA_DATA);
  
  ID=(Deviceinfo.DevcieID[0]<<24) |  Deviceinfo.DevcieID[1]<<16  |  (Deviceinfo.DevcieID[2]<<8) | Deviceinfo.DevcieID[3] ;
  put_devinfo();
  wif_pw_ctl();
  
  Volume_path();
  delay_ms(1000);
  Specify_Volume(0x1a);//ָ���������
  delay_ms(500);
  Specify_musi_play(0x01);//��ӭʹ�����Բ���
  
  KEY_LED_Init();
  
  CpuInit();
  Ask_gpio_init();
  
  Set_Ask_RxMode();
  
  Esp8266_Init();
  Detector_Init();
  TIME_Set(2016,6,1,0,0,0);
  delay_ms(500);
  // Specify_musi_play(33);//�������
  //delay_ms(100);
  zk=0;
  memset(rpdata,0,sizeof(rpdata));
  
  IWDG_Init(6,625*3);//���Ź�ʱ�� 1S
  
  
  
  while(1)
  { 
    
    IWDG_Feed();
    Get_ID_formflash();
    handly_wifi_megs();
    KeyProcess();
    Changgui();
    hand_IR_data();
    // ir_in_handler();
    check_wifi_mesg();
    LED_delay();
    // th=Gettemp();
    // printf("th=%d\r\n",th);
    // temp_humi();
    //handly_mode_time();
  }  
}



/*********************************************************************************************************
End Of File
**********************************************************************************************************/

