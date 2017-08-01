//本程序不包括配置时钟、IO口等配置函数，用户只需根据自己的需要配置好IO口，并改掉GPIO_Pin_10、GPIO_Pin_11这两个引脚即可。



#include "stm32f10x.h"
 


//*********************第二部分DHT90设置   START**************************************** 
#define SCK  GPIO_Pin_8     //定义通讯时钟端口
#define DATA GPIO_Pin_9     //定义通讯数据端口
#define uchar unsigned char
#define uint unsigned int

typedef union  
{ u16 i;      //定义了两个共用体
  float f; 
} value; 

//enum {TEMP,HUMI};      //TEMP=0,HUMI=1
 
#define noACK 0             //结束数据传输
#define ACK   1             //不结束数据传输
                            //adr  command  r/w 
#define STATUS_REG_W 0x06   //000   0011    0 
#define STATUS_REG_R 0x07   //000   0011    1 
#define MEASURE_TEMP 0x03   //000   0001    1 
#define MEASURE_HUMI 0x05   //000   0010    1 
#define RESET        0x1e   //000   1111    0 

/****************声明函数****************/
void s_transstart(void);               //启动传输函数
void s_connectionreset(void);          //连接复位函数
char s_write_byte(unsigned char value);//SHT10写函数
char s_read_byte(unsigned char ack);   //SHT100读函数
char s_measure(unsigned char *p_value, unsigned char *p_checksum, unsigned char mode);//测量温湿度函数
void calc_sht10(float *p_humidity ,float *p_temperature);//温湿度数据转换
extern void delay_ms(u32 nCount);
void Temperatuer(float *wendu,float *shidu) ;


extern  u32 *wendu,*shidu;


void Temp_Humi_Init(void)
{
  
  GPIO_InitTypeDef GPIO_InitStructure;
  
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);//使腜ORTB时钟
  
  GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_9 ;//PB8、PB9
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD; 
//   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
   GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_8 ;//PB8、PB9
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz
  //GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD; 
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
} 


//u8 a,b,c,d;
/*-------------------------------------- 
;模块名称:s_transstart(); 
;功    能:启动传输函数
;占用资源:--
;参数说明:--
;创建日期:2008.08.15 
;版    本:FV1.0(函数版本Function Version)
;修改日期:--
;修改说明:--
;-------------------------------------*/  
void s_transstart(void) 
// generates a transmission start  
//       _____         ________ 
// DATA:      |_______| 
//           ___     ___ 
// SCK : ___|   |___|   |______ 
{   
   GPIO_SetBits(GPIOB, DATA);	//DATA=1,SCK=0
   GPIO_ResetBits(GPIOB, SCK);
                       //Initial state 
   delay_ms(10); 
   GPIO_SetBits(GPIOB, SCK);// SCK=1; 
   delay_ms(10);  
   GPIO_ResetBits(GPIOB, DATA);// DATA=0; 
   delay_ms(10);  
   GPIO_ResetBits(GPIOB, SCK);//SCK=0;   
   delay_ms(30); //_nop_();_nop_();_nop_(); 
   GPIO_SetBits(GPIOB, SCK);//SCK=1; 
   delay_ms(10);  
   GPIO_SetBits(GPIOB,DATA);//DATA=1;        
   delay_ms(10); 
   GPIO_ResetBits(GPIOB,SCK);//SCK=0;   
   delay_ms(10);     
} 

/*-------------------------------------- 
;模块名称:s_connectionreset(); 
;功    能:连接复位函数
;占用资源:--
;参数说明:--
;创建日期:2008.08.15 
;版    本:FV1.0(函数版本Function Version)
;修改日期:--
;修改说明:--
;-------------------------------------*/ 
void s_connectionreset(void) 
// communication reset: DATA-line=1 and at least 9 SCK cycles followed by transstart 
//       _____________________________________________________         ________ 
// DATA:                                                      |_______| 
//          _    _    _    _    _    _    _    _    _        ___     ___ 
// SCK : __| |__| |__| |__| |__| |__| |__| |__| |__| |______|   |___|   |______ 
{   
  unsigned char i;  
  GPIO_SetBits(GPIOB,DATA);	 //DATA=1;
  GPIO_ResetBits(GPIOB,SCK); // SCK=0;                   
  for(i=0;i<9;i++)           //9 SCK cycles 
  { 
   GPIO_SetBits(GPIOB, SCK); //SCK=1;
   delay_ms(10);
   GPIO_ResetBits(GPIOB, SCK);//SCK=0; 
   delay_ms(10);
  } 
  s_transstart();            //transmission start 
} 


/*-------------------------------------- 
;模块名称:s_write_byte(); 
;功    能:DHT90写函数
;占用资源:--
;参数说明:--
;创建日期:2008.08.15 
;版    本:FV1.0(函数版本Function Version)
;修改日期:--
;修改说明:--
;-------------------------------------*/ 
char s_write_byte(unsigned char value) 
//---------------------------------------------------------------------------------- 
// writes a byte on the Sensibus and checks the acknowledge  
{  
  
  unsigned char i, wrong=0;   
  for (i=0x80;i>0;i/=2)                         //shift bit for masking 
  {  
    if (i & value) GPIO_SetBits(GPIOB, DATA);     //写数据 
    else GPIO_ResetBits(GPIOB, DATA); 
	                      
    GPIO_SetBits(GPIOB, SCK);                    //clk for SENSI-BUS 
    delay_ms(30);     
    GPIO_ResetBits(GPIOB, SCK);
	
   
  } 
  GPIO_SetBits(GPIOB, DATA); 
  delay_ms(20);
  
  GPIO_SetBits(GPIOB, SCK);	                    //第九个上升沿	   第9个上升沿后将数据线拉低表示写数据成功
  delay_ms(20);

  wrong=GPIO_ReadInputDataBit(GPIOB, DATA);
  delay_ms(20);

  GPIO_ResetBits(GPIOB, SCK);                  //第九个下降沿	   第9个下降沿后自动释放数据线  
  delay_ms(20);
  
  GPIO_SetBits(GPIOB, DATA); 				   //释放数据线
  return wrong;                                //error=1 in case of no acknowledge //返回：0成功，1失败
 	
} 
 

/*-------------------------------------- 
;模块名称:s_read_byte(); 
;功    能:DHT90读函数
;占用资源:--
;参数说明:--
;创建日期:2008.08.15 
;版    本:FV1.0(函数版本Function Version)
;修改日期:--
;修改说明:--
;-------------------------------------*/ 
char s_read_byte(unsigned char ack)  
// reads a byte form the Sensibus and gives an acknowledge in case of "ack=1"  
{  

  unsigned char i,val=0; 
  GPIO_SetBits(GPIOB, DATA);                                       //release DATA-line 
  delay_ms(5);
  for (i=0x80;i>0;i/=2)             
  { 
    GPIO_SetBits(GPIOB, SCK);                                      
    delay_ms(5);                        							  //稍微延时等待数据线稳定再读取
    if (GPIO_ReadInputDataBit(GPIOB, DATA)) val=(val | i);        //读数据  
	     
	delay_ms(30);                                                    //脉冲宽度 3 us
    GPIO_ResetBits(GPIOB, SCK);              
  } 
  delay_ms(5);													  //第8个下降沿后稍微延时
  if(ack==1) GPIO_ResetBits(GPIOB, DATA);                     //in case of "ack==1" pull down DATA-Line 
  else  GPIO_SetBits(GPIOB, DATA);                            //如果是校验(ack==0)，读取完后结束通讯
  delay_ms(30);                                                  //pulswith approx 3 us 
  GPIO_SetBits(GPIOB, SCK);                                   //第9个上升沿
  delay_ms(30);                                                  //pulswith approx 3 us  
  GPIO_ResetBits(GPIOB, SCK);                 				  //第9个下降沿
  delay_ms(30);         
  GPIO_SetBits(GPIOB, DATA);                                   //release DATA-line 
  return val; 
} 
 

 

/*-------------------------------------- 
;模块名称:s_measure(); 
;功    能:测量温湿度函数
;占用资源:--
;参数说明:--
;创建日期:2008.08.15 
;版    本:FV1.0(函数版本Function Version)
;修改日期:--
;修改说明:--
;-------------------------------------*/ 
char s_measure(unsigned char *p_value, unsigned char *p_checksum, unsigned char mode) 
// makes a measurement (humidity/temperature) with checksum 
{  
  unsigned char wrong=0; 
  unsigned int i; 
  
  s_transstart();                                                  //transmission start 
  delay_ms(30);
  switch(mode)
   {                                                              //send command to sensor 
   case 0  : wrong+=s_write_byte(MEASURE_TEMP); break; 
   case 1  : wrong+=s_write_byte(MEASURE_HUMI); break; 
   default     : break;    
   } 
  for (i=0;i<500;i++) 
   {
      delay_ms(20);
     //delay_ms(20); 
   if(GPIO_ReadInputDataBit(GPIOB, DATA)==0) break;		          // 等待传感器测量
   } 
  //delay_ms(0x8fffff);
  delay_ms(200);
  if(GPIO_ReadInputDataBit(GPIOB, DATA)) wrong+=1;                // or timeout (~2 sec.) is reached 
  
  *(p_value+1)=s_read_byte(ACK);	//读高八位 
  *(p_value)  =s_read_byte(ACK);    //读低八位                                  
  *p_checksum =s_read_byte(noACK);  //read checksum 
  return wrong; 
} 
 

/*-------------------------------------- 
;模块名称:calc_dht90(); 
;功    能:温湿度补偿函数
;占用资源:--
;参数说明:--
;创建日期:2008.08.15 
;版    本:FV1.0(函数版本Function Version)
;修改日期:--
;修改说明:--
;-------------------------------------*/ 
void calc_sht10(float *p_humidity ,float *p_temperature)
// calculates temperature [C] and humidity [%RH] 
// input :  humi [Ticks] (12 bit) 
//          temp [Ticks] (14 bit)
// output:  humi [%RH]
//          temp [C]
{ 
  const float C1=-4.0;              // for 12 Bit
  const float C2=+0.0405;           // for 12 Bit
  const float C3=-0.0000028;        // for 12 Bit

//  const float C1=-2.0468;             // for 12 Bit
//  const float C2=+0.0367;             // for 12 Bit
//  const float C3=-0.0000015955;       // for 12 Bit

  const float T1=+0.01;             // for 14 Bit @ 5V
  const float T2=+0.00008;          // for 14 Bit @ 5V 

  float rh=*p_humidity;             // rh:      Humidity [Ticks] 12 Bit 
  float t=*p_temperature;           // t:       Temperature [Ticks] 14 Bit
  float rh_lin;                     // rh_lin:  Humidity linear
  float rh_true;                    // rh_true: Temperature compensated humidity
  float t_C;                        // t_C   :  Temperature [C]

  t_C=t*0.01 - 39.6;                  //calc. temperature from ticks to [C]
  rh_lin=C3*rh*rh + C2*rh + C1;     //calc. humidity from ticks to [%RH]
  rh_true=(t_C-25)*(T1+T2*rh)+rh_lin;   //calc. temperature compensated humidity [%RH]
  //if(rh_true>100)rh_true=100;       //cut if the value is outside of
  //if(rh_true<0.1)rh_true=0.1;       //the physical possible range

  *p_temperature=t_C;               //return temperature [C]
  *p_humidity=rh_true;              //return humidity[%RH]
}




//*********************第二部分DHT90设置   END****************************************

//**************************
void  Temperatuer(float *wendu,float *shidu)
{
    unsigned char w = 0;
	value humi_val,temp_val;
    unsigned char checksum;
		
    s_connectionreset(); 
        
    delay_ms(0x3fffff);     //延时0.2s 
    w += s_measure((unsigned char*) &humi_val.i,&checksum,1);  //measure humidity 
    w += s_measure((unsigned char*) &temp_val.i,&checksum,0);  //measure temperature 
    if( w != 0 )  
	 {
	 s_connectionreset();                        //in case of an error: connection reset 
     }
	else 
     {
	 humi_val.f=(float)humi_val.i;                   //converts integer to float
     temp_val.f=(float)temp_val.i;                   //converts integer to float
     calc_sht10(&humi_val.f,&temp_val.f);            //calculate humidity, temperature
     } 
    *wendu=temp_val.f;
	*shidu=humi_val.f;          
    
	
	//delay_ms(0x9fffff);                                //延时约0.8s

}
		





