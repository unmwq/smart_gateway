

#include "JQ6500.h"
#include "usart.h"
#include "delay.h"
#include "tick.h"
#include <string.h>


//语音存储顺序
//01  28 欢迎使用.wav
//02  01 学习遥控器.wav
//03  02 学习门磁窗磁.wav
//04  03 学习红外窗幕.wav
//05  04 学习其它传感器.wav
//06  05 学习成功.wav
//07  06 学习失败.wav
//08  07 已经学习.wav
//09  08 退出学习.wav
//10  09 存储区满，请删除后再次学习.wav
//11  10 删除遥控器.wav
//12  11 删除门磁窗磁.wav
//13  12 删除红外窗幕.wav
//14  13 删除其它传感器.wav
//15  14 删除所有传感器.wav
//16  15 删除成功.wav
//17  16 退出删除.wav
//18  17 布防.wav
//19  18 撤防.wav
//20  19 门磁窗磁报警.wav
//21  20 红外窗幕报警.wav
//22  21 其它传感器报警.wav
//23  22 有人.wav
//24  23 无人.wav
//25  24 已经学习到门磁.wav
//26  25 已经学习到红外窗幕.wav 
//27  26 已经学习到其它传感器.wav
//28  27 已经学习到遥控器.wav
//29  28 空白.wav
//30  



void ctl_vol_init()
{
  //语音输入busy检测
  GPIO_InitTypeDef GPIO_InitStructure;
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOC,ENABLE);//使能PORTA,PORTC时钟
  GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_8;//PA8
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; //设置成上拉输出
  GPIO_Init(GPIOA, &GPIO_InitStructure);
}
void ctl_vol()			
{
  GPIO_SetBits(GPIOA,GPIO_Pin_8);
}
void Rectl_vol()		
{	
  GPIO_ResetBits(GPIOA,GPIO_Pin_8);
}



void Slpee_play();
/*********************************************************************************************
//函数名:Check_stats()
//
//功能说明:查询播放状态,在任何时候都可以查询当前的播放状态
//
//入口参数:
//
//出口参数:AA 01 01 播放状态 SM
***********************************************************************************************/
void Check_stats(void)
{  
	u8 Check_Cmd[]={0xAA,0X01,0X00,0XAB};
	Usart2_SendData(Check_Cmd,4);
	
	/*USART_SendData(UART5,(u16)0x7E); 
	USART_SendData(UART5,(u16)0x02);	
	USART_SendData(UART5,(u16)0x01);
	USART_SendData(UART5,(u16)0xEF);*/
}

/*********************************************************************************************
//函数名:Play_now()
//
//功能说明:在任何时候发此命令都会从头开始播放当前曲目
//
//入口参数:
//
//出口参数:NO
***********************************************************************************************/
void Play_now(void)
{
	u8 Play_Cmd[]={0xAA,0X02,0X00,0XAC};

	Usart2_SendData(Play_Cmd,4);
}

/*********************************************************************************************
//函数名:Pause()
//
//功能说明:暂停
//
//入口参数:
//
//出口参数:
***********************************************************************************************/
void Pause(void)
{  
	u8 Pause_Cmd[]={0xAA,0X03,0X00,0XAD};
	Usart2_SendData(Pause_Cmd,4);
	
	/*USART_SendData(UART5,(u16)0x7E); 
	USART_SendData(UART5,(u16)0x02);	
	USART_SendData(UART5,(u16)0x01);
	USART_SendData(UART5,(u16)0xEF);*/
}

/*********************************************************************************************
//函数名:Play_now()
//
//功能说明:在任何时候发此命令都会从头开始播放当前曲目
//
//入口参数:
//
//出口参数:NO
***********************************************************************************************/
void Stop(void)
{
	u8 Stop_Cmd[]={0xAA,0X04,0X00,0XAE};

	Usart2_SendData(Stop_Cmd,4);


}

/*********************************************************************************************
//函数名:Next_Play()
//
//功能说明:设定音量
//
//入口参数:
//
//出口参数:NO
***********************************************************************************************/
void Next_Play(void)

{  
	u8 Next_Cmd[]={0xAA,0X06,0X00,0XB0};
	//u8 Next_Cmd[]={0x7E,0X02,0X01,0XEF};
	Usart2_SendData(Next_Cmd,4);
	
	/*USART_SendData(UART5,(u16)0x7E); 
	USART_SendData(UART5,(u16)0x02);	
	USART_SendData(UART5,(u16)0x01);
	USART_SendData(UART5,(u16)0xEF);*/
}

/*********************************************************************************************
//函数名:Last_Play()
//
//功能说明:
//
//入口参数:
//
//出口参数:NO
***********************************************************************************************/
void Last_Play(void)
{
	//u8 Last_Cmd[]={0x7E,0X02,0X02,0XEF};
	u8 Last_Cmd[]={0xAA,0X05,0X00,0XAF};

	Usart2_SendData(Last_Cmd,4);


}

void Volume_path()
{
  u8 i=0;
  u8 sum=0;
	
	u8 Volume_path[]={0xAA, 0x08, 0x08, 0x02, 0x2F,0x5A,0x48, 0x2A, 0x4D, 0x50, 0x33,0x00};//
	  for(i=0;i<12;i++){
        sum=sum+Volume_path[i];
         }
          Volume_path[11]=sum;
         Usart2_SendData(Volume_path,12);//指定曲目播放
      }

   
/*********************************************************************************************
//函数名:Specify_Musi_Play()
//
//功能说明:指定曲目播放
//
//入口参数:
//
//出口参数:NO
***********************************************************************************************/
void Specify_musi_play(u8 num)
{
 // static u32 last_play_time;
  u8 sum=0;
  u8 i;
//  u8 Specify_Musi_Play_Cmd[]={0xAA, 0x08, 0x09, 0x02, 0x2F,0x5A,0x48,0x00,0x2A,0x4D,0x50,0x33,0x00};
  u8 Specify_Musi_Play_Cmd[]={0xAA,0X07,0X02,0x00,0x00,0X00};
  Specify_Musi_Play_Cmd[4]=num;
  for(i=0;i<6;i++){
sum=sum+Specify_Musi_Play_Cmd[i];
  }
  Specify_Musi_Play_Cmd[5]=sum;
  Usart2_SendData(Specify_Musi_Play_Cmd,6);//指定曲目播放
  }





u16 Get_Musi_Status(void)
{ u16 i[4];
u8 j;

u8 Get_Musi_Status_Cmd[]={0x7E,0X02,0X42,0XEF};
//delay_ms(400);
  Usart2_SendData(Get_Musi_Status_Cmd,4);
//  delay_ms(10);
//   
  do
  {
for(j=0;j<4;j++)
{
 // while (USART_GetFlagStatus(UART5, USART_FLAG_RXNE) == RESET); 
  i[j]=USART_ReceiveData(UART5);
}

 UART4_printf("i[0]=0x%2x,i[1]=0x%2x,i[2]=0x%2x,i[3]=0x%2x\r\n",i[0],i[1],i[2],i[3]);
 if(i[3]==0x54 || i[3]==0x53) return i[3];
  }while(i[3] != 0x30);

return	i[3];
}

/*********************************************************************************************
//函数名:Volume_Add()
//
//功能说明:音量加
//
//入口参数:
//
//出口参数:NO
***********************************************************************************************/
void Volume_Add()
{
	// u8 Volume_Up[]={0x7E,0X02,0X04,0XEF};
	u8 Volume_Up[]={0xAA,0x14,0x00,0xBE};
	Usart2_SendData(Volume_Up,4);

}
/*********************************************************************************************
//函数名:Volume_Dec()
//
//功能说明:音量减
//
//入口参数:
//
//出口参数:NO
***********************************************************************************************/
void Volume_Dec()
{ 
	//u8 Volume_Dec[]={0x7E,0X02,0X05,0XEF};
	u8 Volume_Dec[]={0xAA,0X15,0X00,0XBF};
	Usart2_SendData(Volume_Dec,4);
}
/*********************************************************************************************
//函数名:Specify_Volume()
//
//功能说明:指定音量
//
//入口参数:00-1E
//
//出口参数:NO
***********************************************************************************************/
void Specify_Volume(u8 num)
{
u8 sum=0;
u8 i;
	//u8 Specify_Volume[]={0x7E,0X03,0X06,0x00,0XEF};

	u8 Specify_Volume[]={0xAA,0X13,0X01,0x00,0X00};
        IWDG_Feed();
	Specify_Volume[3]=num;
for(i=0;i<5;i++){
sum=sum+Specify_Volume[i];
}
	Specify_Volume[4]=sum;

	Usart2_SendData(Specify_Volume,5);
	
	
}


/*********************************************************************************************
//函数名:Music_play()
//
//功能说明:播放
//
//入口参数:
//
//出口参数:NO
***********************************************************************************************/
void Music_play(void)
{
	u8 Music_play[]={0x7E,0X02,0X0D,0XEF};
	Usart2_SendData(Music_play,4);
	
}
/*********************************************************************************************
//函数名:Music_pause()
//
//功能说明:暂停
//
//入口参数:
//
//出口参数:NO
***********************************************************************************************/
void Music_pause(void)
{
	u8 Music_pause[]={0x7E,0X02,0X0E,0XEF};
	Usart2_SendData(Music_pause,4);
}

/*********************************************************************************************
//函数名:Reset_Device()
//
//功能说明:复位
//
//入口参数:
//
//出口参数:NO
***********************************************************************************************/
void Reset_Device()
{
	u8 Music_stop[]={0x7E,0X02,0X0C,0XEF};
	Usart2_SendData(Music_stop,4);
}
/******************************************************************************
***************
//函数名:Slpee_play()
//
//功能说明:1:全部循环 0:单曲循环
//
//入口参数:
//
//出口参数:NO
*******************************************************************************
****************/
void Slpee_play()
{
	u8 Slpee_play[]={0x7E,0x02,0x0A,0xEF};
	//Cycle_play[3]=num;
	Usart2_SendData(Slpee_play,4);
	
}
/*********************************************************************************************
//函数名:Specify_play_Mode()
//
//功能说明:Normal/Pop/Rock/Jazz/Classic/Base(0/1/2/3/4/5)切换
//
//入口参数:num(0-5)
//
//出口参数:NO
***********************************************************************************************/
void Specify_play_Mode(u8 num)
{
	u8 Specify_play_Mode[]={0x7E,0X03,0X07,0X00,0XEF};
	Specify_play_Mode[3]=num;
	Usart2_SendData(Specify_play_Mode,5);
	
}

/*********************************************************************************************
//函数名:Switch_play_Device()
//
//功能说明:U/TF/AUX/SLEEP/FLASH(0/1/2/3/4)切换
//
//入口参数:num(0-4)
//
//出口参数:NO
***********************************************************************************************/
void Switch_play_Device(u8 num)
{
	
	u8 Specify_play_Mode[]={0x7E,0X03,0X09,0X00,0XEF};
	Specify_play_Mode[3]=num;
	Usart2_SendData(Specify_play_Mode,5);


}
/*********************************************************************************************
//函数名:Cycle_play()
//
//功能说明:1:全部循环 0:单曲循环
//
//入口参数:
//
//出口参数:NO
***********************************************************************************************/
void Cycle_play(u8 num)
{
	u8 Cycle_play[]={0x7E,0X03,0X11,0X00,0XEF};
	Cycle_play[3]=num;
	Usart2_SendData(Cycle_play,5);
	
}

