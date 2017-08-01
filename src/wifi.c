#include "wifi.h"
extern u32 ID;

extern Alarm_Flags AlarmFlags;
extern SEND_433_DATA   SendDat;
extern u8 first_bf;
extern PA_data         PA_DATA;
extern float temperature;
extern float rh;
extern Scenario_mode   S_MODE;

//extern u16 humidity;
//extern float humi_val_real; 
// extern float temp_val_real;
WIFI_INFO	wifi_info;
u8 GprsData[300];
u8 DS=0xFF;
int HN=0x02;//设置心跳间隔
static u32 handtime=0;
char addr[35]="120.27.154.133";
//char addr[35]="192.168.11.244";
//char addr[35]="192.168.11.77";
//u16 port=5008;

u16 port=5008;
char		*SSID	   = "yanfa";
char		*PassWord  = "zhihuiijia";

u8 error_wifi_time=0;  //wifi断网次数
u8   wifi_flas=0;

void mesg_comeing(char *addr);


void LED_delay(){
  static u32 ledon_time=0;
  static u32 ledoff_time=0;
  
  if(wifi_flas==0){
    
    if(TickGet()-ledon_time>2000){
      //  ledoff_time=TickGet();
      ledon_time=TickGet();
      GPIO_ResetBits(GPIOB,GPIO_Pin_0);//输出高 LED off
      //delay_ms(750);
    }
    if(TickGet()-ledoff_time>800){
      ledoff_time=TickGet();
      // ledon_time=TickGet();
      GPIO_SetBits(GPIOB,GPIO_Pin_0);//输出高 LED off
    }
  }
  
  
}

void zhuhe_send(u8 *data){
  u8 i,j;
  for(j=0;j<2;j++){//每个波形发送5次
    
    for(i=0;i<data[0];i++)
    {
      send_433data(&data[1+12*i],j);  
      delay_ms(40);
    }
    
  }
}


void Esp8266_Init()
{
  u8 szTmpl[400];
  u8 i=0;
  Usart1_Send_Str("AT\r\n");
  delay_ms(100);
  handly_get_value(1,2);
  Usart1_Send_Str("AT+CWMODE=1\r\n");
  delay_ms(100);
  handly_get_value(1,5);
  Usart1_Send_Str("AT+GMR\r\n");
  delay_ms(500);
  handly_get_value(1,2);
  Usart1_Send_Str("AT+CWLAP\r\n");
  delay_ms(3000);
  handly_get_value(3,10);
  Usart1_Send_Str("AT+CIPMUX=0\r\n");
  delay_ms(500);
  handly_get_value(1,7);
  
  if(strlen(wifi_info.ssid)!=0)
  {
    Usart1_Send_Str("AT+CIPCLOSE=0\r\n");
    delay_ms(1000);
    handly_get_value(1,2);
    sprintf(szTmpl,"AT+CWJAP=\"%s\",\"%s\"\r\n",(wifi_info.ssid),(wifi_info.password));//connect net 
    // sprintf(szTmpl,"AT+CWJAP=\"%s\",\"%s\"\r\n",SSID,PassWord);//connect net 
    for(i=0;i<strlen(szTmpl);i++)
    {
      Usart1_SendByte(szTmpl[i]);
    }
    // delay_ms(3000);
    handly_get_value(10,i);
    
  }
  else
  {
    smart_link();
    // delay_ms(500);  
    //   Wifi_udp_connect();
    //准备智能联网（重新上电的情况下）  
  }
  delay_ms(500);
  UARTR1BUF_Read(szTmpl,UARTR1BUF_HoldNum());
  Wifi_udp_connect();
  UARTR1BUF_Read(szTmpl,UARTR1BUF_HoldNum());
}

void wif_pw_ctl(){
  
  GPIO_InitTypeDef GPIO_InitStructure;
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,ENABLE);//使能PORTA,PORTC时钟
  GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_7;//PA11
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; //设置成上拉输出
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  //GPIO_ResetBits(GPIOB,GPIO_Pin_14);
  GPIO_SetBits(GPIOC,GPIO_Pin_7);
  GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_8;//PA11
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; //设置成上拉输出
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  GPIO_SetBits(GPIOC,GPIO_Pin_8);
  
  
  GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_9;//PA11
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; //设置成上拉输出
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  GPIO_SetBits(GPIOC,GPIO_Pin_9);
}

void wifi_on(){
  GPIO_SetBits(GPIOB,GPIO_Pin_14);
}
void  wifi_off(){
  GPIO_ResetBits(GPIOB,GPIO_Pin_14);
}

int handly_get_value(u16 timeout,u8 num)
{
  char Dataincoming[400]={0};
  u8 Len=0;
  u8 i=0;
  char *p;
  u32 time_timeout=0;
  memset(Dataincoming,0,sizeof(Dataincoming));
  time_timeout=TickGet();
  while(UARTR1BUF_HoldNum()<num)
  {
    //  IWDG_Feed();
    if((TickGet()-time_timeout)>timeout*1000)
      return -2;
    //超时退出
  }
  
  time_timeout=TickGet();
  while(strstr(Dataincoming,"OK")==NULL)
  {
    delay_ms(50);
    Len=UARTR1BUF_HoldNum();
    
    UARTR1BUF_Read((BYTE*)Dataincoming,Len);
    
    /*2015.11.2修改字符串长度，由原来的200改为100，当没有返回ok时，速度上有一定的提升by：cq*/
    
    if(i=search(Dataincoming,0x4F,100)){  //OK
      if(strstr(&Dataincoming[i],"OK")!=NULL)
        return 1;
    }
    
    if(i=search(Dataincoming,0x6E,100)){  //no change
      if(strstr(&Dataincoming[i],"no change")!=NULL)
        return 1;
    }
    if(i=search(Dataincoming,0x64,100)){  //device busy
      if(strstr(&Dataincoming[i],"device busy")!=NULL)
        return -1;
    }
    /*2015-8-27*/
    if(i=search(Dataincoming,0x46,100)){   //FAIL
      
      if(strstr(&Dataincoming[i],"FAIL")!=NULL)
        return -1;
    }
    
    if(i=search(Dataincoming,0x45,100)){  //ERROR
      if(strstr(&Dataincoming[i],"ERROR")!=NULL)
        return -1;
    }
    /*
    if(i=search(Dataincoming,0x57,100)){  //WIFI GOT IP
    if( (p=strstr(&Dataincoming[i],"+IPD")) != NULL){
    //memcpy();
    
    return 1;
    
  }
  }*/
    if(i=search(Dataincoming,0x2b,100)){  //"+"
      if(strstr(&Dataincoming[i],"+IPD")!=NULL){
        mesg_comeing(&Dataincoming[i]);
        memset(GprsData,0,sizeof(GprsData));
        return 1;
      }
    }
    if(TickGet()-time_timeout>timeout*1000)
      break;  //超时退出 一般是空命令 或者脏数据
  }
  return 0;
  
}

int smart_link()
{
  u8 Len=0;
  u8 i=0;
  char Dataincoming[400]={0};
  u32 time_timeout=0;
  char *pointer1;
  char *pointer2;
  char ssid[49]="";
  char password[49]="";
  u8 szTmpl[100];
  
  
  
  Specify_musi_play(35);	//准备智能联网
  
  
  Usart1_Send_Str("AT+CWQAP\r\n");
  delay_ms(500);
  handly_get_value(1,1);
  Len=UARTR1BUF_HoldNum();
  
  UARTR1BUF_Read((BYTE*)Dataincoming,Len);
  
  memset(Dataincoming,0,sizeof(Dataincoming));
  // Usart1_Send_Str("AT+CWSTARTSMART\r\n");
  Usart1_Send_Str("AT+CWSMARTSTART=1\r\n");
  // delay_ms(2000);
  time_timeout=TickGet();
  while(UARTR1BUF_HoldNum()<70)
  {
    IWDG_Feed();
    if((TickGet()-time_timeout)>30*1000)
      break ;
    
    //  LED_delay_smartlink();
    //超时退出
  }
  
  time_timeout=TickGet();
  while(strstr(Dataincoming,"OK")==NULL){
    IWDG_Feed();
    
    //   LED_delay_smartlink();
    delay_ms(300);
    Len=UARTR1BUF_HoldNum();
    
    UARTR1BUF_Read((BYTE*)Dataincoming,Len);
    if(strstr(Dataincoming,"SMART SUCCESS")!=NULL){
      // delay_ms(100);
      pointer1=strchr(Dataincoming,':');
      pointer1++;
      
      pointer2=pointer1;
      pointer1=strchr(pointer2,'\r'); 
      
      memcpy(ssid,pointer2,pointer1-pointer2);
      
      //  pointer2++;
      while(*pointer1==0x20){
        pointer1++;
      }
      
      pointer2=strchr(pointer1,':');
      pointer2++;
      pointer1=strchr(pointer2,'\r');
      memcpy(password,pointer2,pointer1-pointer2);
      /*   
      memcpy(ssid,pointer2,pointer1-pointer2);
      pointer2++;
      pointer1=strchr(pointer2,':');
      pointer2=strchr(pointer1,'\r');
      pointer1++;
      while(*pointer1==0x20){
      pointer1++;
    }*/
      //memcpy(password,pointer1,pointer2-pointer1);
      memcpy( &( wifi_info.ssid[0] ), &ssid, sizeof( ssid ) );    //如果保存有IP，端口，则优先使用保存好的，修改IP端口后第一时间存入内存
      memcpy( &( wifi_info.password[0] ), &password, sizeof( password ) );
      SaveWifiInfo( &wifi_info );
      delay_ms(5000);
      // Usart1_Send_Str("AT+CWSTOPSMART\r\n");  //退出smartlink
      Usart1_Send_Str("AT+CWSMARTSTOP\r\n");  //退出smartlink
      delay_ms(200);
      handly_get_value(1,2);
      delay_ms(500);
      return 1;
      
    }
    
    
    if((TickGet()-time_timeout)>30*1000){
      break;
    }
  }
  Usart1_Send_Str("AT+CWSMARTSTOP\r\n");  //退出smartlink
  // Usart1_Send_Str("AT+CWSTOPSMART\r\n");  //退出smartlink
  delay_ms(200);
  handly_get_value(1,2);
}

void Wifi_udp_connect()
{
  char Dataincoming[400]={0};
  u32 time_timeout=0;
  u8 szTemp[50]="";
  u16 temp=0;
  u16 loc_port=1000;
  u8 mode=0;
  // u8 mode=1;
  volatile u8 Len=0;
  u8 i;
  
  if(check_IP_stat(10,10))
  {
    for(i=0;i<10;i++) 
      temp += wifi_info.Address[i];
    if( temp != 0 )
    {
      memcpy( &addr, &( wifi_info.Address[0] ), 20);                 //如果保存有IP，端口，则优先使用保存好的，修改IP端口后第一时间存入内存
    }
    // AT+CIPSTART=“UDP”,"192.168.101.110",1000,1002,2
    //sprintf(szTemp,"AT+CIPSTART=\"UDP\",\"%s\",%d,%d,%d\r\n",addr,port,loc_port,mode);//udp clinet
    sprintf(szTemp,"AT+CIPSTART=\"UDP\",\"%s\",%d\r\n",addr,port);//udp clinet
    for(i=0;i<strlen(szTemp);i++)
      Usart1_SendByte(szTemp[i]);
    delay_ms(1000);
    time_timeout=TickGet();
    while(UARTR1BUF_HoldNum()<10)
    {
      IWDG_Feed();
      if((TickGet()-time_timeout)>30*1000)
        break ;
      //超时退出
    }
    
    time_timeout=TickGet();
    while(strstr(Dataincoming,"OK")==NULL)
    {
      
      IWDG_Feed();
      delay_ms(200);
      Len=UARTR1BUF_HoldNum();
      
      UARTR1BUF_Read((BYTE*)Dataincoming,UARTR1BUF_HoldNum());
      
      if(i=search(Dataincoming,0x43,200))		//CONNECT OK
      {  
        if(strstr(&Dataincoming[i],"CONNECT")!=NULL)
        {
          Specify_musi_play(34); //联网成功
          wifi_flas=1;
          GPIO_ResetBits(GPIOB,GPIO_Pin_0);//输出高 LED off
          delay_ms(500);
          return;
        }
      }
      
      if((TickGet()-time_timeout)>30*1000)
      {
        
        Specify_musi_play(47); //联网失败
        wifi_flas=0;
        delay_ms(500);
        
        return;
      }
      
    }
  }else
  {
    Specify_musi_play(47); //联网失败
    wifi_flas=0;
    delay_ms(500);
  }
}

/*
查询AP信号
*/
void check_wifi_apr()
{
  static u32 time_timeout=0;
  if(TickGet()-time_timeout>5*1000)
  {
    time_timeout=TickGet();
    Usart1_Send_Str("AT+CAPR\r\n");
    delay_ms(1000);
    handly_get_value(1,2);
  }
}

int search(const char *s,const char b,u8 lens)
{
  int i;
  for(i=0;i<lens;i++)
  {
    if(s[i]==b)
      return i;
  }
  return -1;
}

void Wifi_GetData(char *temp,u8 lens)
{ 
  char *pointer;
  
  pointer = strchr(temp,':');
  pointer++;
  
  memcpy(GprsData,pointer,lens);		
  //  zj=0; 
  //  memset(sdata,0,sizeof(sdata));     
}

void handle_WiFi_time_first()
{
  u16 temperature1=0;
  u16 humidity=0;
  char sztmp[10]={0};
  TIME_Get(TIM2_tick);  //获取系统时间并存储在全局时间结构体中
  //	sprintf(szTmp,"%x%d%d%[",DS,calendar.hour,calendar.min,calendar.sec);
  
  temperature1=  (temperature1*10);
  humidity=     (rh*10);
  sztmp[0]=DS;
  sztmp[1]=calendar.hour;
  sztmp[2]=calendar.min;
  sztmp[3]=calendar.sec;
  sztmp[4]=AlarmFlags.bIsRmt_Deploy;
  memcpy(&sztmp[5],&temperature,2);
  memcpy(&sztmp[7],&humidity,2);
  // printf("TickGet()=%d\r\n",TickGet());
  wifi_sendcmd((BYTE *)sztmp,9,Heartbeat_cmd,0); 
  delay_ms(200);
  handly_get_value(1,2);
}


void handle_udp_connect()
{
  u32 time_timeout=0;
  u8 szTemp[50]="";
  u8 i;
  volatile u8 Len=0;
  char Dataincoming[400]={0};
  
  
  if(strlen(wifi_info.ssid)!=0)
  {
    Usart1_Send_Str("AT+CIPCLOSE=0\r\n");
    delay_ms(1000);
    handly_get_value(1,2);
    memset(szTemp,0,sizeof(szTemp));
    sprintf(szTemp,"AT+CWJAP=\"%s\",\"%s\"\r\n",(wifi_info.ssid),(wifi_info.password));//connect net 
    // sprintf(szTmpl,"AT+CWJAP=\"%s\",\"%s\"\r\n",SSID,PassWord);//connect net 
    for(i=0;i<strlen(szTemp);i++)
    {
      Usart1_SendByte(szTemp[i]);
    }
    handly_get_value(10,i);
  } 
  memset(szTemp,0,sizeof(szTemp));
  sprintf(szTemp,"AT+CIPSTART=\"UDP\",\"%s\",%d\r\n",addr,port);//udp clinet
  for(i=0;i<strlen(szTemp);i++)
    Usart1_SendByte(szTemp[i]);
  handly_get_value(10,i);
}


u8 handly_wifi_megs()
{
  //  int i;
  char szTmp[50];
  static      u32 firsttime=0;
  static      u8 error_time=0;
  static      u32 udp_connect_time=0;
  u16 temperature1=0;
  u16 humidity=0;
  if(DS==0xFF&&(TickGet()-firsttime>10000) && TickGet()<5*60*1000)
  {
    firsttime=TickGet();
    handle_WiFi_time_first();
  }
  if(TickGet()>(handtime+HN*1000*30))     //心跳包  30秒一次   5分钟需要测定时间   HN=1
  {
    handtime=TickGet();
    TIME_Get(TIM2_tick);  //获取系统时间并存储在全局时间结构体中
    temperature1= (temperature*10);
    humidity=   (rh*10);
    szTmp[0]=DS;
    szTmp[1]=calendar.hour;
    szTmp[2]=calendar.min;
    szTmp[3]=calendar.sec;
    szTmp[4]=AlarmFlags.bIsRmt_Deploy;
    memcpy(&szTmp[5],&temperature1,2);
    memcpy(&szTmp[7],&humidity,2);
    //  szTmp[5]=humi_val_real;
    //  szTmp[6]=;
    //  szTmp[7]=;
    //  szTmp[8]=;
    if(check_IP_stat(2,3))
    {
      wifi_sendcmd((BYTE *)szTmp,9,Heartbeat_cmd,0); 
      delay_ms(100);
      //	   for(i=0;i<20;i++)
      //		printf("szTmp[%d]=%x\r\n",i,szTmp[i]);
      if(handly_get_value(1,2)!=1)
      {
        error_time++;
      }
      else
      {
        error_time=0;
      }
    } 
    else 
    {
      //hand_wifi_err();
      error_time++;
    }
    if(error_time>3)
    {
      wifi_flas=0;
      hand_wifi_err();
    }
  }
  /*
  if((TickGet()-udp_connect_time)>1000*60*10)
  {
  udp_connect_time=TickGet();
  handle_udp_connect();
}*/
  hand_wifi_mesg();
}


void check_wifi_mesg()
{
  if(UARTR1BUF_HoldNum()>5)
    hand_wifi_mesg(); //防止不解析命令
}

void  hand_wifi_err()
{
  static u32 err_time=0;
  u8 len;
  len=strlen(wifi_info.ssid);
  if(TickGet()-err_time>(1000*60*10*error_wifi_time) &&(len!=0))
  { 
    Esp8266_Init();
    error_wifi_time++;
    err_time=TickGet();
  }
}

u32 check_wifi_Message(char *temp)
{ 
  char *pointer;
  
  char s[2]={0};
  u8 totalnum=0;
  int i=0;
  char my_buf[50]="";
  memcpy(my_buf,temp,50);
  // UART4_Send_Str(sdata);
  pointer = strchr(temp,',');
  printf("pointer=%s\r\n",pointer); 
  pointer++;
  
  s[0]=*pointer;
  pointer++;
  if(*pointer!=':')
  {
    s[1]=*pointer;
    i=1;
  }
  else 
    s[1]='0';
  if(i==1)
    totalnum=(s[0]-'0')*10+(s[1]-'0');
  else
    totalnum=(s[0]-'0');
  
  delay_ms(100);
  printf("totalnum=%d\r\n",totalnum); 
  
  return totalnum;
}


void time_u32(u8 *str)
{	
  calendar.w_year=((str[7])*100+(str[8]));
  calendar.w_month=(str[9]);
  calendar.w_date=(str[10]);
  calendar.hour=(str[11]);
  calendar.min=(str[12]);
  calendar.sec=(str[13]);
  printf("%d-%d-%d-%d-%d-%d\r\n",calendar.w_year,calendar.w_month,calendar.w_date,calendar.hour,calendar.min,calendar.sec);
}



void handle_wifi_timemessage()
{							///对时处理函数
  
  time_u32(GprsData);  //十进制输入
  
  //Htoi(GprsData);   //十六进制输入
  if(TIME_Set(calendar.w_year,calendar.w_month,calendar.w_date,calendar.hour,calendar.min,calendar.sec)==1)
    printf("TIM2_tick = %d\r\n",TIM2_tick);
  TIME_Get(TIM2_tick);
  DS=0xAA;   //对时完成状态
  // sprintf(szTemp,"%x",Time);
  wifi_sendcmd((BYTE *)Correction_time,0,Correction_time,GprsData[4]);
  delay_ms(200);
  handly_get_value(1,2);
}

u32 wifi_Message_Apr(char *temp)
{ 
  char *wifi_Apr;
  u8 totalnum;
  char my_buf[100]="";
  memcpy(my_buf,temp,50);
  // UART4_Send_Str(sdata);
  wifi_Apr = strchr(temp,',');
  wifi_Apr++;	//指针指向下一个值
  wifi_Apr++;
  wifi_Apr++;
  printf("wifi=%s\r\n",wifi_Apr); 
  totalnum=atoi(wifi_Apr);		//atoi();字符串转换为数字函数（是C语言标准函数库）
  printf("wifi_Apr=%d\r\n",totalnum); 
  delay_ms(500);
  return totalnum;
}


void handly_mode_time()
{
  u8 i=0;
  u8 weekend;
  u8 week1;
  
  if(DS==0xAA)
  {
    if(S_MODE.enable_num!=0)
    {
      for(i=0;i<5;i++)
      {
        
        if(S_MODE.area[i][1]!=0)
        {
          // weekend=calendar.week;
          
          //week1=S_MODE.area[i][2]<<(8-weekend);
          // week1=week1>>8;
          // if(week1==1)
          // {
          if((S_MODE.area[i][3]==calendar.hour)&&(S_MODE.area[i][4]==calendar.min)&&(S_MODE.area[i][5]==calendar.sec))
          {
            weekend=calendar.week;
            week1=S_MODE.area[i][2];
            week1=week1>>(weekend-1);
            
            if(week1%2==1)
            {
              // zhuhe_send(&S_MODE.Send_433[i][0]);
              handly_ctl_mode(i,0);
              if(S_MODE.area[i][1]==2)  //只执行一次
              {
                S_MODE.area[i][1]=0;
                SaveScenario_mode(&S_MODE);
              }
              return ;
            }
          }
          // }
          // if(weekend==S_MODE.area[])
        }
      }
      
    } 
  }
  
}

void mesg_comeing(char *addr)
{
  int i,j;
  u8 len;
  u8 mseg_len=0;
  u32 tempID=(Deviceinfo.DevcieID[0]<<24)|(Deviceinfo.DevcieID[1]<<16)|(Deviceinfo.DevcieID[2]<<8)|(Deviceinfo.DevcieID[3]);  
  
  //data[0-3]   ID
  //data[4]  报文计数器
  //data[5] 报文长度
  //data[6] cmd
  //data[7-] 命令细节
  mseg_len=check_wifi_Message(addr);
  Wifi_GetData(addr,mseg_len);
  if(((GprsData[0]<<24) | (GprsData[1]<<16)| (GprsData[2]<<8) | GprsData[3])==tempID)
  {
    
    switch(GprsData[6])
    {
    case Correction_time: //对时程序
      
      handle_wifi_timemessage();
      break;
    case Bufang_NET:    //布防
      wifi_sendcmd((BYTE *)Bufang_NET,0,Bufang_NET,GprsData[4]);
      Specify_musi_play(18);
      AlarmFlags.bIsRmt_Deploy=1;
      first_bf=1;
      hand_pa_send(1);
      //delay_ms(100);
      
      delay_ms(200);
      handly_get_value(3,4);
      
      break;
    case Chefang_NET:  
      wifi_sendcmd((BYTE *)Chefang_NET,0,Chefang_NET,GprsData[4]);  //撤防
      Specify_musi_play(19);
      AlarmFlags.bIsRmt_Deploy=0;
      hand_pa_send(2);
      //delay_ms(100);
      
      delay_ms(200);
      handly_get_value(3,4);
      break;
    case Volume_set:   
      Specify_Volume(GprsData[7]);
      wifi_sendcmd((BYTE *)&GprsData[7],1,Volume_set,GprsData[4]);
      handly_get_value(1,2);
      break;
    case IP_set:     //客户端的服务器地址设置
      memcpy(&addr[0],&GprsData[7],mseg_len-9);
      memset(&(wifi_info.Address[0]),0,20);
      memcpy(&(wifi_info.Address[0]),&addr[0],mseg_len-9); //如果保存有IP，端口，则优先使用保存好的，修改IP端口后第一时间存入内存
      SaveWifiInfo(&wifi_info);
      wifi_sendcmd((BYTE *)IP_set,0,IP_set,GprsData[4]);
      handly_get_value(1,2);
      break;
      
    case Heartbeat_Cycle: //设置心跳包间
      UART4_Send_Str("心跳包间隔设置\r\n");
      HN=GprsData[7];
      wifi_sendcmd((BYTE *)Heartbeat_Cycle,0,Heartbeat_Cycle,GprsData[4]);
      handly_get_value(3,4);
      break;
    case ALM_net:  
      Specify_musi_play(51);
      hand_pa_send(3);
      //Specify_Musi_Play(38);  //紧急报警
      wifi_sendcmd((BYTE *)ALM_net,0,ALM_net,GprsData[4]);
      handly_get_value(1,2);
      break;
      
    case voice_mode:   
      
      wifi_info.voice_flags=GprsData[7]; //如果保存有IP，端口，则优先使用保存好的，修改IP端口后第一时间存入内存
      SaveWifiInfo(&wifi_info);
      if(wifi_info.voice_flags==0){
        // Specify_musi_play(44); //报警声音已开启
        Specify_musi_play(45); //报警声音已关闭 
      }
      else if(wifi_info.voice_flags==1)
      {
        Specify_musi_play(44); //报警声音已开启
      }
      else if(wifi_info.voice_flags==2)
      {
        Specify_musi_play(44); //报警声音已开启
      }
      
      wifi_sendcmd((BYTE *)&GprsData[7],1,voice_mode,GprsData[4]);
      
      delay_ms(100);
      handly_get_value(3,4);
      //send_close_motion();
      break;
    case send_dev_info: 
      send_device_info();
      break;
      
    case Study_433_device:   
      Specify_musi_play(41);     
      
      hand_save_433(GprsData[7]);
      break;
      
    case Send_433_device:   
      if(GprsData[7]!=3)
      {
        AlarmFlags.bIsRmt_Deploy=GprsData[7];
      } 
      if(GprsData[8]==0)
      {
        //不播报语音
      }
      else if(GprsData[8]==1)
      {
        Specify_musi_play(48);
      }
      else if(GprsData[8]==2)
      {
        Specify_musi_play(49);
      }
      else if(GprsData[8]==3)
      {
        Specify_musi_play(50);
      }
      for(j=0;j<2;j++){
        
        send_433data(&GprsData[9],j);
        delay_ms(50);
        
      }
      
      wifi_sendcmd((BYTE *)&GprsData[9],0,Send_433_device,GprsData[4]);
      handly_get_value(1,2);
      break;
      
    case zhuhe_mode:   
      
      AlarmFlags.bIsRmt_Deploy=GprsData[7];
      if(GprsData[8]==1)
      {
        Specify_musi_play(48);
      }
      else if(GprsData[8]==2)
      {
        Specify_musi_play(49);
      }
      else if(GprsData[8]==3)
      {
        Specify_musi_play(50);
      }
      zhuhe_send(&GprsData[9]);
      wifi_sendcmd((BYTE *)&GprsData[7],0,zhuhe_mode,GprsData[4]);
      break;
      
    case set_self_mode:        
      AlarmFlags.bIsRmt_Deploy=2;
      Specify_musi_play(54);//自定义模式
      memset(&SendDat,0,sizeof(SEND_433_DATA));
      if(GprsData[7]==0)
      {
        GprsData[7]=16;//全部关闭
        memcpy(&SendDat,&GprsData[7],1);
      }
      else{
        memcpy(&SendDat,&GprsData[7],GprsData[7]*6+1);
      }
      Save433SendDat(&SendDat);
      wifi_sendcmd((BYTE *)set_self_mode,0,set_self_mode,GprsData[4]);
      delay_ms(100);
      handly_get_value(3,4);
      break;
    case sleep_mode:        
      AlarmFlags.bIsRmt_Deploy=2;
      
      wifi_sendcmd((BYTE *)sleep_mode,0,sleep_mode,GprsData[4]);
      Specify_musi_play(53);//自定义
      handly_get_value(1,2);
      
      break;
    case reset_net:
      
      Factory_Reset();
      Specify_musi_play(52);
      wifi_sendcmd((BYTE *)reset_net,0,reset_net,GprsData[4]);
      handly_get_value(3,4);
      IWDG_Init(0,0);
      break;
    case study_app:   //获取状态! 
      Study_app(GprsData[7]);
      break;
    case delete_app:   
      Delete_app(&GprsData[8]);
      break;
    case save_app:   
      save_sensor(GprsData[7],&GprsData[8],0);
      break;
    case check_app:  
      check_sensor();
      break;
    case restore_app:
      InitStudatDefault(&StudyDat);
      for(i=0;i<GprsData[7];i++){
        save_sensor(GprsData[8+i*6]-5,&GprsData[9+i*6],1);
      }
      Specify_musi_play(0x06);          //学习成功
      break;
    case delete_sensor:   
      Delete_Sensor(GprsData[7]);
      break;
    case en_disable_time_ctl:   
      S_MODE.area[GprsData[5]][1]=GprsData[6];  //area   bufan/kaiqidingshi/zhou/shi/fen/miao
      S_MODE.enable_num=S_MODE.area[0][1]+S_MODE.area[1][1]+S_MODE.area[2][1]+S_MODE.area[3][1]+S_MODE.area[4][1];
      SaveScenario_mode(&S_MODE);
      break;
      
    case study_kaiguan: 
      Specify_musi_play(41);   
      delay_ms(500);
      wxkg_study();
      break;  
    case save_guanlin:
      Specify_musi_play(54);
      handsave_guanlin(&GprsData[7]);
      wifi_sendcmd((BYTE *)save_guanlin,0,save_guanlin,GprsData[4]);
      handly_get_value(1,2);
      break; 
    case save_Scenario:
      Specify_musi_play(54);
      save_Scenar(&GprsData[7]);
      wifi_sendcmd((BYTE *)save_Scenario,0,save_Scenario,GprsData[4]);
      handly_get_value(1,2);
      break;  
    case delete_guanlin:
      Delete_guanlin(&GprsData[7]);
      // wifi_sendcmd((BYTE *)delete_guanlin,0,delete_guanlin,0);
      //handly_get_value(1,2);
      break; 
    case delete_Scenario:
      delete_Scenar(GprsData[7]);
      // wifi_sendcmd((BYTE *)delete_Scenario,0,delete_Scenario,0);
      //handly_get_value(1,2);
      break; 
    case check_guanlin:
      //save_Scenar(&GprsData[5]);
      break; 
    case check_Scenario:
      // save_Scenar(&GprsData[5]);
      break; 
    case ctl_Scenario:
      handly_ctl_mode(GprsData[7],1);
      break; 
    case Study_IR:
      hand_study_IR(&GprsData[7]);
      break; 
    case SET_IR_MODE:
      hand_SET_IR_MODE(&GprsData[7]);
      wifi_sendcmd((BYTE *)SET_IR_MODE,0,SET_IR_MODE,GprsData[4]);
      handly_get_value(1,2);
      //  bee_bee(100,100);
      break; 
    case DELE_IR_MODE:
      //  bee(100);
      hand_DELETE_IR_MODE(GprsData[7]);
      wifi_sendcmd((BYTE *)&GprsData[7],1,DELE_IR_MODE,GprsData[4]);
      handly_get_value(1,2);
      break; 
    case ctl_IR_MODE:
      //  bee(100);
      GPIO_ResetBits(GPIOB,GPIO_Pin_1);//输出高 LED off
      hand_ir_mode(GprsData[7]);
      GPIO_SetBits(GPIOB,GPIO_Pin_1);//输出高 LED off
      wifi_sendcmd((BYTE *)ctl_IR_MODE,0,ctl_IR_MODE,GprsData[4]);
      handly_get_value(1,2);
      //  bee(100);
      //   hand_study_IR(&GprsData[5]);
      break; 
    case ctl_IR:
      //   AlarmFlags.bIsRmt_Deploy=GprsData[7];
      GPIO_ResetBits(GPIOB,GPIO_Pin_1);//输出高 LED off
      IR_OUT(&GprsData[8]);
      GPIO_SetBits(GPIOB,GPIO_Pin_1);//输出高 LED off
      wifi_sendcmd((BYTE *)ctl_IR,0,ctl_IR,GprsData[4]);
      handly_get_value(1,2);
      break; 
    case hand_time:
      wifi_sendcmd((BYTE *)hand_time,0,hand_time,GprsData[4]);
      handly_get_value(1,2); 
      break;
    case PA_en_disable:
      PA_DATA.en_disable=GprsData[7];
      SavePA_data(&PA_DATA);
      break;
    default:
      break;
    }
  }
}


void handly_ctl_mode(u8 data,u8 info)
{
  if(data==0)
  {
    // Specify_musi_play(48);
    Specify_musi_play(48);
  }
  else if(data==1)
  {
    Specify_musi_play(49);
  }
  else if(data==2)
  {
    Specify_musi_play(50);
  }
  else
  {
    Specify_musi_play(56);
  }
  
  
  if(S_MODE.area[data][0]!=3)
  {
    AlarmFlags.bIsRmt_Deploy = S_MODE.area[data][0];
  }
  
  if(info==1)//主动控制
  {
    data=5;
  }
  
  zhuhe_send(&S_MODE.Send_433[data][0]);
  wifi_sendcmd((BYTE *)&data,1,ctl_Scenario,0);
  handly_get_value(1,2);
  delay_ms(100);
  
  
}

void  hand_wifi_mesg(void)
{
  int i,j;
  u8 len;
  // u8 mseg_len=0;
  // u32 tempID=(Deviceinfo.DevcieID[0]<<24)|(Deviceinfo.DevcieID[1]<<16)|(Deviceinfo.DevcieID[2]<<8)|(Deviceinfo.DevcieID[3]);  
  char Dataincoming[400]={0};
  if(UARTR1BUF_HoldNum()>10)
  {
    delay_ms(20);
    len=UARTR1BUF_HoldNum();
    memset(Dataincoming,0,sizeof(Dataincoming));
    memset(GprsData,0,sizeof(GprsData));
    UARTR1BUF_Read((BYTE*)Dataincoming,len);
    
    if(strstr(Dataincoming,"+IPD")!=NULL)
    {
      mesg_comeing(Dataincoming);
      memset(GprsData,0,sizeof(GprsData));
    }  //if +IPD
  }  //if len >5
  
}


void Specify_Musi_Play(u8 num)
{
  //static u8 last_num;
  // static u32 last_time;
  if(wifi_info.voice_flags==1)
  {
    Specify_musi_play(num);
    hand_pa_send(3);
    delay_ms(500);
    
  }
  else if(wifi_info.voice_flags==2)
  {
    Specify_musi_play(51);
    hand_pa_send(3);
    // delay_ms(500);
  }
  
  
}

//-------------------------------------------------------
int wifi_sendchar(BYTE *buffer, int count)
{
  
  unsigned char *pByte;
  int i = 0;
  char buffer1[50];
  u8 typeinfo[2]={0x45,0x43};  //EU
  
  sprintf(buffer1,"AT+CIPSEND=%d\r\n",count+6);
  Usart1_Send_Str(buffer1);
  delay_ms (10); 
  for(i=0;i<2;i++){
    Usart1_SendByte(typeinfo[i]);
  }
  //pByte=ID;
  for(i=0;i<4;i++){
    Usart1_SendByte(Deviceinfo.DevcieID[i]);
  }
  //delay_ms (80);
  pByte = buffer;
  for (i = 0; i<count; i++)
  {
    Usart1_SendByte(pByte[i]);
  }
  
  return count;
}


int wifi_sendcmd(BYTE *buffer, int count,u8 cmd,u8 svrProfileId)
{
  
  u16  crc_value=0;
  char buffer1[50];
  u8 buffer2[250];
  u8 i;
  
  u8 typeinfo[2]={0x57,0x52};  //EU
  
  // sprintf(buffer0,"AT+CIPSTART=\"UDP\",\"%s\",%d,%d,%d\r\n",addr,port,loc_port,mode);//udp clinet
  //for(i=0;i<strlen(buffer0);i++)
  //  Usart1_SendByte(buffer0[i]);
  //  handly_get_value(1,5);
  //sprintf(buffer1,"AT+CIPSEND=%d,\"%s\",%d\r\n",count+9,addr,port);
  
  handtime=TickGet();
  sprintf(buffer1,"AT+CIPSEND=%d\r\n",count+11);
  
  memcpy(buffer2,&typeinfo,2);
  memcpy(&(buffer2[2]),&(Deviceinfo.DevcieID[0]),4);
  buffer2[6]=svrProfileId;
  buffer2[7]=count+1;
  memcpy(&(buffer2[8]),&cmd,1);
  memcpy(&(buffer2[9]),buffer,count);
  crc_value=CRC16(buffer2,count+9);
  memcpy(&(buffer2[9+count]),&crc_value,sizeof(crc_value));
  Usart1_Send_Str(buffer1);
  delay_ms (100); 
  Usart1_SendData(buffer2,count+11);
  
  return count;
}

/********************************************************************************************************
********************************************************************************************************
check_IP_stat()
判断获取到IP没有 总共等待5秒
返回1  ip可用  
返回0  Ip不可用
可以考虑重新初始化wifi
********************************************************************************************************
********************************************************************************************************
*/
u8 check_IP_stat(u8 num,u8 timeout)
{
  u8 i=0;
  char Dataincoming[400]={0};
  u8 Len;
  u8 IP_flags;
  u32 time_timeout=0;
  Len=UARTR1BUF_HoldNum();
  UARTR1BUF_Read((BYTE*)Dataincoming,Len);
  memset(Dataincoming,0,sizeof(Dataincoming));
  
  for(i=0;i<num;i++)
  {
    //delay_ms(500);
    Usart1_Send_Str("AT+CIFSR\r\n");//查询ip
    delay_ms(200);
    //这里考虑是否有返回值
    time_timeout=TickGet();
    while(UARTR1BUF_HoldNum()<5 )
    {
      IWDG_Feed();
      if((TickGet()-time_timeout)>timeout*1000)
      {
        IP_flags=0;//该指令5秒都没有反应
        return IP_flags;
        
      }
    }
    Len=UARTR1BUF_HoldNum();
    UARTR1BUF_Read((BYTE*)Dataincoming,Len);
    
    if(strstr(Dataincoming,"0.0.0.0")!=NULL)
    {
      memset(Dataincoming,0,sizeof(Dataincoming));
      //i++;
      IP_flags=0;
    }
    else
    {
      IP_flags=1;
      break;
    }
  }
  return IP_flags;
}