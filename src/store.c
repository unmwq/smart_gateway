#include "store.h"

Ctl_Relation    C_Relation;
Scenario_mode   S_MODE;
STOR_433_DATA	StudyDat;
SEND_433_DATA   SendDat;
PA_data         PA_DATA;
IR_Data  ir_data;
DEVICE_INFO		Deviceinfo;
Alarm_Flags		AlarmFlags;


BOOL			FlagFirstRun;  
u8 flash_data[1024];

extern WIFI_INFO wifi_info;
extern char				addr[35];
extern u16				port;



void handsave_guanlin(u8 *data)
{
  
  u8 i;
  Ctl_Relation *pC_Relation;
  pC_Relation = &C_Relation; 
  
  for(i=0;i<5;i++)
  {
    if(C_Relation.rec_data[i][0]==data[1]
       &&C_Relation.rec_data[i][1]==data[2]
         &&C_Relation.rec_data[i][2]==data[3]
           &&C_Relation.rec_data[i][3]==data[4]
             &&C_Relation.rec_data[i][4]==data[5])
    {
      memcpy(&C_Relation.Send_433[i][0],&data[6],data[6]*12);
      C_Relation.area[i]=data[0]+1;
      SaveCtl_Relation(&C_Relation);
      return ;
    }
  }
  
  for(i=0;i<5;i++)
  {
    if(C_Relation.area[i]!=0)
    {
      i++;
      
    }else if(C_Relation.area[i]==0)
    {
      memcpy(&C_Relation.rec_data[i][0],&data[1],5);
      memcpy(&C_Relation.Send_433[i][0],&data[6],data[6]*12+1);
      C_Relation.area[i]=data[0]+1;
      SaveCtl_Relation(&C_Relation);
      return ;
    }
    if(i==5)
    {
      //存储区满，请删除一条后继续
    }
    
  }
}

void delete_Scenar(u8 data)
{
  if(data<5){
    memset(&S_MODE.Send_433[data][0],0,60);
    //  S_MODE.area[data][0]=0;
    memset(&S_MODE.area[data][0],0,6);
    S_MODE.enable_num= S_MODE.enable_num-1;
    SaveScenario_mode(&S_MODE);
    wifi_sendcmd((BYTE *)&data,1,delete_Scenario,0);
    handly_get_value(1,2);
    return ;
  }
}

void Delete_guanlin(u8 *data)
{
  
  u8 i;
  // Ctl_Relation *pC_Relation;
  //  pC_Relation = &C_Relation; 
  
  for(i=0;i<5;i++)
  {
    if(C_Relation.rec_data[i][0]==data[0]
       &&C_Relation.rec_data[i][1]==data[1]
         &&C_Relation.rec_data[i][2]==data[2]
           &&C_Relation.rec_data[i][3]==data[3]
             &&C_Relation.rec_data[i][4]==data[4])
    {
      memset(&C_Relation.Send_433[i][0],0,60);
      C_Relation.area[i]=0;
      SaveCtl_Relation(&C_Relation);
      
      wifi_sendcmd((BYTE *)&data[0],5,delete_guanlin,0);
      handly_get_value(1,2);
      return ;
    }
  }
  if(i==5)
  {
    //存储区满，请删除一条后继续
  }
  
}

void InitSendDefault(SEND_433_DATA *pSendDat)
{
  memset(pSendDat,0,sizeof(SEND_433_DATA));
  Save433SendDat(&SendDat);
}

BOOL Save433SendDat(SEND_433_DATA *pSendDat)
{
  int32_t datalong=0;
  u32 checksum=0;
  u8 *ptr;
  u32 i=0;
  
  ptr = (u8*)FLASH_SEND_ADDRESS;
  
  memset(&flash_data[0], 0x00, sizeof(flash_data));
  memcpy(&flash_data[0],ptr,1024);
  
  datalong =sizeof(SEND_433_DATA);
  
  checksum =(u32)CRC16((u8*)pSendDat,datalong);
  
  memcpy(&flash_data[0],(u8 *)pSendDat,datalong);
  memcpy(&flash_data[datalong],(u8 *)&checksum,sizeof(checksum));
  
  FLASH_Unlock();
  FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_PGERR|FLASH_FLAG_WRPRTERR);
  FLASH_ErasePage(FLASH_SEND_ADDRESS);
  for(i=0;i<512;i++)
  {
    FLASH_ProgramHalfWord((FLASH_SEND_ADDRESS+2*i ),*((u16 *)&flash_data[0]+i));
    
  }
  FLASH_Lock();
  
  
  return TRUE;
  
}


void InitSendDat(SEND_433_DATA *pSendDat)
{
  
  if(CheckSendDatExist()) //保存区若有有效配置，则恢复
  {
    //InitSendDefault(pSendDat);
    Load433SendDat(pSendDat);
  }
  else				//否则默认配置
  {
    InitSendDefault(pSendDat);
    
  }
  
}


BOOL Load433SendDat(SEND_433_DATA *pSendDat)
{
  SEND_433_DATA * ptr;
  ptr = (SEND_433_DATA *) FLASH_SEND_ADDRESS;
  memcpy(pSendDat,ptr,sizeof(SEND_433_DATA));
  return TRUE;
}




/***********************************************************
* Function:       // 函数名称
* Description:    // 函数功能、性能等的描述
* Input:          // 1.输入参数1，说明，包括每个参数的作用、取值说明及参数间关系
* Input:          // 2.输入参数2，说明，包括每个参数的作用、取值说明及参数间关系
* Output:         // 1.输出参数1，说明
* Return:         // 函数返回值的说明
* Others:         // 其它说明
***********************************************************/


void InitStudatDefault(STOR_433_DATA *pStudyDat)
{
  memset(pStudyDat,0,sizeof(STOR_433_DATA));
  Save433StudyDat(&StudyDat);
}


void InitAlarmFlags(void)
{
  AlarmFlags.bIsRmt_Deploy = 0; //撤防
  FlagFirstRun = TRUE;
  
}

BOOL Save433StudyDat(STOR_433_DATA *pStudyDat)
{
  int32_t datalong=0;
  u32 checksum=0;
  u8 *ptr;
  u32 i=0;
  
  ptr = (u8*)FLASH_STUDAT_ADDRESS;
  
  memset(&flash_data[0], 0x00, sizeof(flash_data));
  memcpy(&flash_data[0],ptr,1024);
  
  datalong =sizeof(STOR_433_DATA);
  
  checksum =(u32)CRC16((u8*)pStudyDat,datalong);
  
  memcpy(&flash_data[0],(u8 *)pStudyDat,datalong);
  memcpy(&flash_data[datalong],(u8 *)&checksum,sizeof(checksum));
  
  FLASH_Unlock();
  FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_PGERR|FLASH_FLAG_WRPRTERR);
  FLASH_ErasePage(FLASH_STUDAT_ADDRESS);
  for(i=0;i<512;i++)
  {
    FLASH_ProgramHalfWord((FLASH_STUDAT_ADDRESS+2*i ),*((u16 *)&flash_data[0]+i));
    
  }
  FLASH_Lock();
  
  
  return TRUE;
  
}


BOOL SaveDevInfo(DEVICE_INFO *pDevInfo)
{
  
  
  int32_t datalong=0;
  u32 checksum=0;
  u8 *ptr;
  u32 i=0;
  
  ptr = (u8*)FLASH_SAVE_ADDRESS;
  
  memset(&flash_data[0], 0x00, sizeof(flash_data));
  memcpy(&flash_data[0],ptr,1024);
  
  datalong =sizeof(DEVICE_INFO);
  checksum = (u32)CRC16((u8 *)(pDevInfo),datalong);
  
  memcpy(&flash_data[FLASH_DEVINFO_OFFSET],(u8 *)pDevInfo,datalong);
  memcpy(&flash_data[FLASH_DEVINFO_OFFSET+datalong],(u8 *)&checksum,sizeof(checksum));
  
  FLASH_Unlock();
  FLASH_ClearFlag( FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR );
  FLASH_ErasePage( FLASH_DEVINFO_ADDRESS );
  for(i=0;i<512;i++)
  {
    FLASH_ProgramHalfWord((FLASH_SAVE_ADDRESS+2*i ),*((u16 *)&flash_data[0]+i));
    
  }
  FLASH_Lock();
  
  
  return TRUE;
  
}


BOOL SaveCtl_Relation(Ctl_Relation *pCtl_R)
{
  
  
  int32_t datalong=0;
  u32 checksum=0;
  u8 *ptr;
  u32 i=0;
  
  ptr = (u8*)FLASH_SEND_ADDRESS;
  
  memset(&flash_data[0], 0x00, sizeof(flash_data));
  memcpy(&flash_data[0],ptr,1024);
  
  datalong =sizeof(Ctl_Relation);
  checksum = (u32)CRC16((u8 *)(pCtl_R),datalong);
  
  memcpy(&flash_data[FLASH_Ctl_OFFSET],(u8 *)pCtl_R,datalong);
  memcpy(&flash_data[FLASH_Ctl_OFFSET+datalong],(u8 *)&checksum,sizeof(checksum));
  
  FLASH_Unlock();
  FLASH_ClearFlag( FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR );
  FLASH_ErasePage( FLASH_SEND_ADDRESS );
  for(i=0;i<512;i++)
  {
    FLASH_ProgramHalfWord((FLASH_SEND_ADDRESS+2*i ),*((u16 *)&flash_data[0]+i));
    
  }
  FLASH_Lock();
  
  
  return TRUE;
  
}

BOOL SaveScenario_mode(Scenario_mode *pS_MODE)
{
  
  
  int32_t datalong=0;
  u32 checksum=0;
  u8 *ptr;
  u32 i=0;
  
  ptr = (u8*)FLASH_SEND_ADDRESS;
  
  memset(&flash_data[0], 0x00, sizeof(flash_data));
  memcpy(&flash_data[0],ptr,1024);
  
  datalong =sizeof(Scenario_mode);
  checksum = (u32)CRC16((u8 *)(pS_MODE),datalong);
  
  memcpy(&flash_data[FLASH_Scenar_OFFSET],(u8 *)pS_MODE,datalong);
  memcpy(&flash_data[FLASH_Scenar_OFFSET+datalong],(u8 *)&checksum,sizeof(checksum));
  
  FLASH_Unlock();
  FLASH_ClearFlag( FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR );
  FLASH_ErasePage( FLASH_SEND_ADDRESS );
  for(i=0;i<512;i++)
  {
    FLASH_ProgramHalfWord((FLASH_SEND_ADDRESS+2*i ),*((u16 *)&flash_data[0]+i));
    
  }
  FLASH_Lock();
  
  
  return TRUE;
  
}

BOOL Save_ir_data(IR_Data *pIr_data)
{
  
  
  int32_t datalong=0;
  u32 checksum=0;
  u8 *ptr;
  u32 i=0;
  
  ptr = (u8*)FLASH_IR_ADDRESS;
  
  memset(&flash_data[0], 0x00, sizeof(flash_data));
  memcpy(&flash_data[0],ptr,1024);
  
  datalong =sizeof(IR_Data);
  checksum = (u32)CRC16((u8 *)(pIr_data),datalong);
  
  memcpy(&flash_data[0],(u8 *)pIr_data,datalong);
  memcpy(&flash_data[datalong],(u8 *)&checksum,sizeof(checksum));
  
  FLASH_Unlock();
  FLASH_ClearFlag( FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR );
  FLASH_ErasePage( FLASH_IR_ADDRESS );
  for(i=0;i<512;i++)
  {
    FLASH_ProgramHalfWord((FLASH_IR_ADDRESS+2*i ),*((u16 *)&flash_data[0]+i));
    
  }
  FLASH_Lock();
  
  
  return TRUE;
  
}

BOOL SavePA_data(PA_data *pPA_data)
{
  
  
  int32_t datalong=0;
  u32 checksum=0;
  u8 *ptr;
  u32 i=0;
  
  ptr = (u8*)FLASH_SEND_ADDRESS;
  
  memset(&flash_data[0], 0x00, sizeof(flash_data));
  memcpy(&flash_data[0],ptr,1024);
  
  datalong =sizeof(PA_data);
  checksum = (u32)CRC16((u8 *)(pPA_data),datalong);
  
  memcpy(&flash_data[FLASH_PAdata_OFFSET],(u8 *)pPA_data,datalong);
  memcpy(&flash_data[FLASH_PAdata_OFFSET+datalong],(u8 *)&checksum,sizeof(checksum));
  
  FLASH_Unlock();
  FLASH_ClearFlag( FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR );
  FLASH_ErasePage( FLASH_SEND_ADDRESS );
  for(i=0;i<512;i++)
  {
    FLASH_ProgramHalfWord((FLASH_SEND_ADDRESS+2*i ),*((u16 *)&flash_data[0]+i));
  }
  FLASH_Lock();
  
  
  return TRUE;
  
}

BOOL CheckIRdataExist(void)
{
  u8 * ptr;
  int32_t datalong=0;
  u32 checksum=0;
  u16 tmp=0;
  
  ptr = (u8 *) FLASH_IR_ADDRESS;
  datalong =sizeof(IR_Data);
  checksum = (u32)CRC16(ptr,datalong);
  tmp = *((u16*)(ptr+datalong));
  
  
  if(tmp==checksum)	
  {
    return TRUE;
  }
  else
  {
    return FALSE;
  }
  
}
BOOL CheckStuDatExist(void)
{
  u8 * ptr;
  int32_t datalong=0;
  u32 checksum=0;
  u16 tmp=0;
  
  ptr = (u8 *) FLASH_STUDAT_ADDRESS;
  datalong =sizeof(STOR_433_DATA);
  checksum = (u32)CRC16(ptr,datalong);
  tmp = *((u16*)(ptr+datalong));
  
  
  if(tmp==checksum)	
  {
    return TRUE;
  }
  else
  {
    return FALSE;
  }
  
}


BOOL CheckPA_data(void)
{
  u8 * ptr;
  int32_t datalong=0;
  u32 checksum=0;
  u16 tmp=0;
  
  ptr = (u8 *) FLASH_PAdata_ADDRESS;
  datalong =sizeof(PA_data);
  checksum = (u32)CRC16(ptr,datalong);
  tmp = *((u16*)(ptr+datalong));
  
  
  if(tmp==checksum)	
  {
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

BOOL CheckScenario_mode(void)
{
  u8 * ptr;
  int32_t datalong=0;
  u32 checksum=0;
  u16 tmp=0;
  
  ptr = (u8 *) FLASH_Scenar_ADDRESS;
  datalong =sizeof(Scenario_mode);
  checksum = (u32)CRC16(ptr,datalong);
  tmp = *((u16*)(ptr+datalong));
  
  
  if(tmp==checksum)	
  {
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

BOOL CheckCtl_Relation(void)
{
  u8 * ptr;
  int32_t datalong=0;
  u32 checksum=0;
  u16 tmp=0;
  
  ptr = (u8 *) FLASH_Ctl_ADDRESS;
  datalong =sizeof(Ctl_Relation);
  checksum = (u32)CRC16(ptr,datalong);
  tmp = *((u16*)(ptr+datalong));
  
  
  if(tmp==checksum)	
  {
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}


BOOL CheckDevInfoExist(void)
{
  u8 * ptr;
  
  int32_t datalong=0;
  u32 checksum=0;
  u16 tmp=0;
  
  ptr = (u8 *) FLASH_DEVINFO_ADDRESS;
  datalong =sizeof(DEVICE_INFO);
  checksum = (u32)CRC16(ptr,datalong);
  tmp = *((u16*)(ptr+datalong));
  
  
  if(tmp==checksum)	
  {
    return TRUE;
  }
  else
  {
    return FALSE;
  }
  
}

void InitIRDat(IR_Data *pIR_data)
{
  
  if(CheckIRdataExist()) //保存区若有有效配置，则恢复
  {
    LoadIRDat(pIR_data);
  }
  else				//否则默认配置
  {
    InitIRdatDefault(pIR_data);	
  }
}


void InitStuDat(STOR_433_DATA *pStudyDat)
{
  
  if(CheckStuDatExist()) //保存区若有有效配置，则恢复
  {
    Load433StudyDat(pStudyDat);
  }
  else				//否则默认配置
  {
    InitStudatDefault(pStudyDat);	
  }
}

BOOL LoadPA_data(PA_data *pPA_data)
{
  
  
  PA_data* ptr;
  
  ptr = (PA_data *) FLASH_PAdata_ADDRESS;
  
  memcpy(pPA_data,ptr,sizeof(PA_data));
  
  return TRUE;
}

BOOL LoadCtl_Relation(Ctl_Relation *pCtl_R)
{
  
  
  Ctl_Relation* ptr;
  
  ptr = (Ctl_Relation *) FLASH_Ctl_ADDRESS;
  
  memcpy(pCtl_R,ptr,sizeof(Ctl_Relation));
  
  return TRUE;
}

void InitCtl_RelationDefault(Ctl_Relation *pCtl_R)
{
  memset(pCtl_R,0,sizeof(Ctl_Relation));
  SaveCtl_Relation(&C_Relation);
}
void InitPA_dataDefault(PA_data *pPA_data)
{
  memset(pPA_data,0,sizeof(PA_data));
  SavePA_data(&PA_DATA);
}

void InitCtl_Relation(Ctl_Relation *pCtl_R)
{
  
  if(CheckCtl_Relation()) //保存区若有有效配置，则恢复
  {
    LoadCtl_Relation(pCtl_R);
    // InitCtl_RelationDefault(pCtl_R);	
    
  }
  else				//否则默认配置
  {
    InitCtl_RelationDefault(pCtl_R);	
  }
}

void InitPA_data(PA_data *pPA_data)
{
  
  if(CheckPA_data()) //保存区若有有效配置，则恢复
  {
    LoadPA_data(pPA_data);
    // InitCtl_RelationDefault(pCtl_R);	
    
  }
  else				//否则默认配置
  {
    InitPA_dataDefault(pPA_data);	
  }
}



BOOL LoadScenario_mode(Scenario_mode *pS_MODE)
{
  
  
  Scenario_mode* ptr;
  
  ptr = (Scenario_mode *) FLASH_Scenar_ADDRESS;
  
  memcpy(pS_MODE,ptr,sizeof(Scenario_mode));
  
  return TRUE;
}

void InitScenario_modeDefault(Scenario_mode *pS_MODE)
{
  memset(pS_MODE,0,sizeof(Scenario_mode));
  SaveScenario_mode(&S_MODE);
}
void InitScenario_mode(Scenario_mode *pS_MODE)
{
  
  if(CheckScenario_mode()) //保存区若有有效配置，则恢复
  {
    LoadScenario_mode(pS_MODE);
  }
  else				//否则默认配置
  {
    InitScenario_modeDefault(pS_MODE);	
  }
}

BOOL LoadIRDat(IR_Data *pIR_data)
{
  
  
  IR_Data * ptr;
  
  ptr = (IR_Data *) FLASH_IR_ADDRESS;
  
  memcpy(pIR_data,ptr,sizeof(IR_Data));
  
  return TRUE;
}


void InitDevInfoDefault(DEVICE_INFO *pDevInfo)
{
  
  memset(pDevInfo,0,sizeof(DEVICE_INFO));
  pDevInfo->DeviceName[0] = 'W';
  pDevInfo->DeviceName[1] = 'G';
  pDevInfo->DeviceName[2] = 'A';
  pDevInfo->DeviceName[3] = '2';
  pDevInfo->DeviceName[4] = '-';
  pDevInfo->DeviceName[5] = 'V';
  pDevInfo->DeviceName[6] = '1';
  pDevInfo->DeviceName[7] = '3';
  pDevInfo->DeviceName[8] = '0';
  
  MakeDeviceID(&pDevInfo->DevcieID[0]);
  pDevInfo->FirmwareVer[0] = 'V';
  pDevInfo->FirmwareVer[1] = '1';
  pDevInfo->FirmwareVer[2] = '.';
  pDevInfo->FirmwareVer[3] = '2';
  
  pDevInfo->SerialNO[0] = 'V';
  pDevInfo->SerialNO[1] = '0';
  pDevInfo->SerialNO[2] = '1';
  pDevInfo->SerialNO[3] = '.';
  pDevInfo->SerialNO[4] = '2';
  pDevInfo->SerialNO[5] = '2';
  pDevInfo->SerialNO[6] = '0';
  pDevInfo->SerialNO[7] = '0';
  SaveDevInfo(&Deviceinfo);
}

void InitWifDefault(WIFI_INFO *pwifi_info)
{
  
  memset(pwifi_info,0,sizeof(WIFI_INFO));
  pwifi_info->voice_flags=1;
  memcpy(&(pwifi_info->Address[0]),&addr,sizeof(addr));
  memcpy(&pwifi_info->Port,&port,sizeof(port));
  SaveWifiInfo(pwifi_info);
}

void InitIRdatDefault(IR_Data *pIR_data)
{
  memset(pIR_data,0,sizeof(IR_Data));
  Save_ir_data(&ir_data);
}


BOOL SaveWifiInfo(WIFI_INFO *pWifi_info)
{
  
  
  int32_t datalong=0;
  uint32_t checksum=0;
  uint8_t *ptr;
  uint32_t i=0;
  
  ptr = (uint8_t*)FLASH_SAVE_ADDRESS;
  
  memset(&flash_data[0], 0x00, sizeof(flash_data));
  memcpy(&flash_data[0],ptr,1024);
  
  datalong =sizeof(WIFI_INFO);
  checksum = (uint32_t)CRC16((uint8_t *)(pWifi_info),datalong);
  
  memcpy(&flash_data[FLASH_WIFINFO_OFFSET],(uint8_t *)pWifi_info,datalong);
  memcpy(&flash_data[FLASH_WIFINFO_OFFSET+datalong],(uint8_t *)&checksum,sizeof(checksum));
  
  FLASH_Unlock();
  
  FLASH_ClearFlag( FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR );
  
  FLASH_ErasePage(FLASH_WIFINFO_ADDRESS);
  for(i=0;i<512;i++)
  {
    FLASH_ProgramHalfWord((FLASH_SAVE_ADDRESS+2*i ),*((uint16_t *)&flash_data[0]+i));
    
  }
  FLASH_Lock();
  
  
  return TRUE;
  
}


BOOL Load433StudyDat(STOR_433_DATA *pStudyDat)
{
  
  
  STOR_433_DATA * ptr;
  
  ptr = (STOR_433_DATA *) FLASH_STUDAT_ADDRESS;
  
  memcpy(pStudyDat,ptr,sizeof(STOR_433_DATA));
  
  return TRUE;
}

BOOL LoadWifInfo(WIFI_INFO *pwifi_info)
{
  
  
  WIFI_INFO * ptr;
  
  ptr = (WIFI_INFO *) FLASH_WIFINFO_ADDRESS;
  
  memcpy(pwifi_info,ptr,sizeof(WIFI_INFO));
  
  return TRUE;
}



BOOL CheckSendDatExist(void)
{
  u8 * ptr;
  int32_t datalong=0;
  u32 checksum=0;
  u16 tmp=0;
  
  ptr = (u8 *) FLASH_SEND_ADDRESS;
  datalong =sizeof(SEND_433_DATA);
  checksum = (u32)CRC16(ptr,datalong);
  tmp = *((u16*)(ptr+datalong));
  
  
  if(tmp==checksum)	
  {
    return TRUE;
  }
  else
  {
    return FALSE;
  }
  
}

BOOL CheckWifInfoExist(void)
{
  uint8_t * ptr;
  
  int32_t datalong=0;
  uint32_t checksum=0;
  uint16_t tmp=0;
  
  ptr = (uint8_t *) FLASH_WIFINFO_ADDRESS;
  datalong =sizeof(WIFI_INFO);
  checksum = (uint32_t)CRC16(ptr,datalong);
  tmp = *((uint16_t*)(ptr+datalong));
  
  
  if(tmp==checksum)	
  {
    return TRUE;
  }
  else
  {
    return FALSE;
  }
  
}

BOOL LoadDevInfo(DEVICE_INFO *pDevInfo)
{
  
  
  DEVICE_INFO * ptr;
  
  ptr = (DEVICE_INFO *) FLASH_DEVINFO_ADDRESS;
  
  memcpy(pDevInfo,ptr,sizeof(DEVICE_INFO));
  
  return TRUE;
}


void InitWifInfo(WIFI_INFO *wifi_info)
{
  
  if(CheckWifInfoExist()) //保存区若有有效配置，则恢复
  {
    //InitWifDefault(wifi_info);
    LoadWifInfo(wifi_info);
  }
  else				//否则默认配置
  {
    InitWifDefault(wifi_info);
  }
  
}

void InitDevInfo(DEVICE_INFO *pDevInfo)
{
  if(CheckDevInfoExist()) //保存区若有有效配置，则恢复
  {
    //InitDevInfoDefault(&Deviceinfo);
    LoadDevInfo(&Deviceinfo);
  }
  else				//否则默认配置
  {
    InitDevInfoDefault(&Deviceinfo);
  }
  
}

void Factory_Reset()
{ 
  InitDevInfoDefault(&Deviceinfo);
  InitSendDefault(&SendDat);
  InitStudatDefault(&StudyDat);
  InitWifDefault(&wifi_info);
  //InitCtl_RelationDefault(pCtl_R);
  InitCtl_RelationDefault(&C_Relation);
  InitScenario_mode(&S_MODE);
}
//-----------------------------------------------------------------end of the file----------------------------------------------------------------------//