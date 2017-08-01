
#include "temp-humi.h"
#include "main.h"
//use for temperature and humidity sensor sht10
u8 sensorMeasureFlag = 0;
u8 tempIncFlag = FALSE;
u8 humiIncFlag = FALSE;

u8 sensorMeasureStep = 0;
u8 sensorError = 0;
float temperature,oldTemperature=0;
u8 humidity,oldHumidity=0;              
u16 m_temperature,m_humidity;         
u8 humiCompensation,tempCompensation;





void Temp_Humi_Init(void)
{
  
  GPIO_InitTypeDef GPIO_InitStructure;
  
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);//使腜ORTB时钟
  
  GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_9 ;//PB8、PB9
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD; 
  // GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_8 ;//PB8、PB9
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz
  //GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD; 
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
} 
void S_DATA_OUT(void)
{
  
  GPIO_InitTypeDef GPIO_InitStructure;
  
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);//使腜ORTB时钟
  
  GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_9 ;//PB8、PB9
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD; 
  // GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  
  
}
void S_DATA_IN(void)
{
  
  GPIO_InitTypeDef GPIO_InitStructure;
  
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);//使腜ORTB时钟
  
  GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_9 ;//PB8、PB9
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD; 
  // GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  
  
}
void S_SCK_OUT(void)
{
  
  GPIO_InitTypeDef GPIO_InitStructure;
  
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);//使腜ORTB时钟
  
  GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_8 ;//PB8、PB9
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz
  //GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD; 
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  
  
}

/******************************************************************************/
/*           Temperature and  humidity sensor function start                  */
/******************************************************************************/
// generates a transmission start 
//       _____         ________
// DATA:      |_______|
//           ___     ___
// SCK : ___|   |___|   |______ 
void sensorTransStart(void)
{  
  S_DATA_HIGH;			                      //Initial state
  delay_ms(1);
  S_SCK_LOW;
  delay_ms(1);
  S_SCK_HIGH;
  delay_ms(1);
  S_DATA_LOW;
  delay_ms(1);
  S_SCK_LOW;
  delay_ms(1);
  S_SCK_HIGH;  
  delay_ms(1);
  S_DATA_HIGH;
  delay_ms(1);
  S_SCK_LOW;   
  delay_ms(1);
}

// communication reset: DATA-line=1 and at least 9 SCK cycles followed by transstart
//       _____________________________________________________         ________
// DATA:                                                      |_______|
//          _    _    _    _    _    _    _    _    _        ___     ___
// SCK : __| |__| |__| |__| |__| |__| |__| |__| |__| |______|   |___|   |______
void sensorConnectionReset(void)
{  
  u8 i; 
  S_DATA_OUT();
  delay_ms(1);
  S_SCK_OUT();
  delay_ms(1);
  S_DATA_HIGH;			                      //Initial state
  delay_ms(1);
  S_SCK_LOW;                     
  delay_ms(1);
  for(i = 0;i<9;i++)                                    //9 SCK cycles
  { 
    S_SCK_HIGH;
    delay_ms(1);
    S_SCK_LOW;
    delay_ms(1);
  }
  sensorTransStart();                                     //transmission start
}


u8 sensorWriteByte(u8 value)
{ 
  u8 i,error=0;  
  S_DATA_OUT();
  for (i = 0x80;i>0;i = i>>1)                              //shift bit for masking
  { 
    if (i & value) S_DATA_HIGH;                       //masking value with i , write to SENSI-BUS
    else S_DATA_LOW;          
    delay_ms(1);
    S_SCK_HIGH;                                      //clk for SENSI-BUS
    delay_ms(1);
    S_SCK_LOW;
  }
  S_DATA_IN();                          
  delay_ms(1);
  S_SCK_HIGH;                                       //clk #9 for ack 
  delay_ms(1);
  error=S_DATA;                                     //check ack (DATA will be pulled down by SHT11)
  S_SCK_LOW;        
  delay_ms(1);
  S_DATA_OUT();			                    //release DATA-line
  delay_ms(1);
  return error;                                     //error=1 in case of no acknowledge
}

u8 sensorReadByte(u8 ack)
{ 
  u8 i,val=0;
  S_DATA_IN();
  delay_ms(1);
  for (i=0x80;i>0;i = i>>1)                             //shift bit for masking
  { 
    S_SCK_HIGH;                                     //clk for SENSI-BUS
    delay_ms(1);
    if (S_DATA) val = (val | i);                      //read bit  
    S_SCK_LOW;  					 
    delay_ms(1);
  }
  S_DATA_OUT();
  delay_ms(1);
  if(ack)
    S_DATA_LOW;
  else
    S_DATA_HIGH;                                   //in case of "ack==1" pull down DATA-Line
  delay_ms(1);
  S_SCK_HIGH;                                       //clk #9 for ack
  delay_ms(1);
  S_SCK_LOW;	                                    //release DATA-line
  delay_ms(1);
  return val;
} 


u8 sensorMeasureCmd(u8 mode)
{
  u8 error = 0;
  sensorTransStart();
  if(mode==TEMP)                                    
    error += sensorWriteByte(MEASURE_TEMP);
  else
    error += sensorWriteByte(MEASURE_HUMI);
  S_DATA_IN();
  return error;
}


u8 sensorReadData(u16 *pValue)
{
  if(S_DATA)
    return 1;
  else
  {
    *pValue    = sensorReadByte(ACK);                      //read the first byte (MSB)
    *pValue    = (*pValue)<<8;
    (*pValue) += sensorReadByte(NOACK);                    //read the second byte (LSB)
    return 0;
  }
}


void sensorWriteStatus(void)
{
  sensorConnectionReset();
  sensorWriteByte(MEASURE_STATUS);
  sensorWriteByte(0x01);  
}

// calculates temperature and humidity [%RH] 
// input :  humi [Ticks] (8 bit) 
//          temp [Ticks] (12 bit)
// output:  humi [%RH]
//          temp 

float rh;
void sensorCalcSht10(void)
{ 
  //float rh;                                         // rh:      Humidity
  float t;                                          // t:       Temperature
  u8 tempCompensationInt,tempCompensationDot;
  u16 ttt;
  //int8s temperatureInt;
  
  u8 temperatureInt;
  u8 temperatureDot;
  // t = m_temperature*0.04 - 39.63; // - 0.00000032*ttt*ttt;    //calc. temperature
  t = m_temperature*0.01 - 39.7; // - 0.00000032*ttt*ttt;    //calc. temperature
  // rh = 0.648-0.00072*m_humidity;     //calc. humidity from ticks to [%RH]
  // rh = rh *m_humidity -4.0;             //calc. temperature compensated humidity [%RH]
  //rh = (t-25)*(0.01+0.00128*m_humidity)+rh;  
  
  // rh=-2.0468+0.0367*m_humidity-0.0000015955*m_humidity*m_humidity;
  
  temperature = t-3;  
  rh=0.0367-0.000008*m_humidity;
  rh=rh *m_humidity -2.0468;
  rh=(temperature*10-25)*(0.01+0.000085*m_humidity)+rh;
  
  
  
  //temperature=t;
  if((tempCompensation&0x80)==0)
  {
    tempCompensationInt=(tempCompensation&0x70)>>4;
    tempCompensationDot=tempCompensation&0x0f;
    temperature=temperature+tempCompensationInt;
    temperature=temperature+tempCompensationDot*0.1;
    //   emberSerialPrintf(APP_SERIAL,"a \r\n");  
  }
  else
  {                
    tempCompensationInt=(tempCompensation&0x70)>>4;
    tempCompensationDot=tempCompensation&0x0f;
    temperature=temperature-tempCompensationInt;
    temperature=temperature-tempCompensationDot*0.1;
    //   emberSerialPrintf(APP_SERIAL,"b \r\n");  
  }   
  
  humidity = (u8)rh;
  if((humiCompensation&0x80)==0)
  {
    humidity=humidity+humiCompensation;
  }
  else
  {
    humidity=humidity-humiCompensation+0x80;
  }
  printf("当前环境温度为:%2.1f℃，湿度为:%2.1f%\r\n",temperature,rh);
  
  
  // printf("humidity=%d,temperature=2.1f%\r\");
  if(humidity>100) humidity = 100;                  //cut if the value is outside of  
  if(humidity<1)   humidity = 1;                      //the physical possible range
}












void temp_humi(void)

{
  static u16 sensorTime,sensorStepTime;
  //static int8s temperatureInt;
  // static int8u temperatureDot;
  
  static u8 temperatureInt;
  static u8 temperatureDot;
  
  if(((u16)(TickGet()- sensorTime) > 4000) && (sensorMeasureFlag == 0)) 
  {
    sensorTime = TickGet();
    sensorMeasureStep = 0;
    sensorMeasureFlag = 1;
  }
  if(((u16)(TickGet() - sensorStepTime) > 5000) && (sensorMeasureFlag == 1))
  {
    //halResetWatchdog();
    sensorStepTime = TickGet();
    
    switch(sensorMeasureStep)                                     
    {
      
    case 0:
      sensorError = sensorMeasureCmd(TEMP);
      sensorMeasureStep = 1;
      break;
    case 1:
      sensorError += sensorReadData(&m_temperature);
      sensorMeasureStep = 2;
      break;
    case 2:
      sensorError += sensorMeasureCmd(HUMI);
      sensorMeasureStep = 3;
      break;
    case 3:
      sensorError += sensorReadData(&m_humidity);
      sensorMeasureStep = 4;
    case 4:
      if(sensorError==0) 
        sensorCalcSht10();           //calculate humidity, temperature 
      sensorMeasureStep = 5;    
      break;
    case 5:
      if(sensorError!=0) 
      {  
        sensorConnectionReset(); 
      }              //in case of an error: connection reset
      else
      {                
        sensorMeasureFlag = 0;
        if(temperature > oldTemperature)
        {
          if((temperature - oldTemperature)>TEMP_INC)
            tempIncFlag = TRUE;
          else
            tempIncFlag = FALSE;
        }
        else
        {         
          if((oldTemperature - temperature)>TEMP_INC)
            tempIncFlag = TRUE;
          else
            tempIncFlag = FALSE;
        }
        if(humidity > oldHumidity)
        {
          if((humidity - oldHumidity)>HUMIDITY_INC)
            humiIncFlag = TRUE;
          else
            humiIncFlag = FALSE;
        }
        else
        {         
          if((oldHumidity - humidity)>HUMIDITY_INC)
            humiIncFlag = TRUE;
          else
            humiIncFlag = FALSE;
        }
        //  temperatureInt = (int8s)temperature;  
        temperatureInt = temperature;  
        if(temperature>0)
          temperatureDot = (u8)((temperature-temperatureInt)*10);
        else
          temperatureDot = (u8)((temperatureInt - temperature)*10);
        // emberSerialPrintf(APP_SERIAL,"temp:%d.%d humi:%d \r\n",temperatureInt,temperatureDot,humidity);
        if(tempIncFlag == TRUE)
        {          
          // emberSerialPrintf(APP_SERIAL,"temp:%d.%d \r\n",temperatureInt,temperatureDot);
          //发送函数
          /*     m_CmdFrameBuffer.length = LEN_DATA_FIELD_TWO_BYTE;
          m_CmdFrameBuffer.cmd = CMD_OBJECT_TO_SUB;
          if(temperature>0)
          m_CmdFrameBuffer.data = (temperatureInt<<8) + temperatureDot;
                else
          {
          m_CmdFrameBuffer.data = (((temperatureInt^0x7F)+1)<<8) + temperatureDot;
        }      
          m_CmdFrameBuffer.clusterId = CLUSTER_TEMP_DETECT;
          m_CmdFrameBuffer.sourceEndpoint = ENDPOINT3_TEMPERATURE;
          sendCmdToSub();*/
          tempIncFlag = FALSE; 
          
          oldTemperature = temperature;
          
        }
      }
      //emberSerialPrintf(APP_SERIAL,"STEP  7 \r\n"); 
      sensorError = 0;
      sensorMeasureStep = 0;           
      break;
    default:
      break;
    }
  }
}

