#include "cc1101.h"
INT8U TxBuf[8]={0};	 // 8�ֽ�, �����Ҫ���������ݰ�,����ȷ����
INT8U RxBuf[8]={0}; 
INT8U leng =8;
//***************���๦�ʲ������ÿ���ϸ�ο�DATACC1100Ӣ���ĵ��е�48-49ҳ�Ĳ�����******************
//INT8U PaTabel[8] = {0x04 ,0x04 ,0x04 ,0x04 ,0x04 ,0x04 ,0x04 ,0x04};  //-30dBm   ������С
//INT8U PaTabel[8] = {0x60 ,0x60 ,0x60 ,0x60 ,0x60 ,0x60 ,0x60 ,0x60};  
//INT8U PaTabel[8] = {0xC0 ,0xC0 ,0xC0 ,0xC0 ,0xC0 ,0xC0 ,0xC0 ,0xC0};   //10dBm     �������
INT8U PaTabel_AskTxd[8] =  {0x00,0xc0,0x00,0x00,0x00,0x00,0x00,0x00};//0dBm
INT8U PaTabel_AskRxd[8] = {0x00 ,0x12 ,0x0E ,0x33 ,0x68 ,0x53 ,0x61 ,0x60};  //0dBm
//*****************************************************************************************
// RF_SETTINGS is a data structure which contains all relevant CCxxx0 registers
typedef struct S_RF_SETTINGS
{
  INT8U FSCTRL2;   //????
  INT8U FSCTRL1;   // Frequency synthesizer control.
  INT8U FSCTRL0;   // Frequency synthesizer control.
  INT8U FREQ2;     // Frequency control word, high INT8U.
  INT8U FREQ1;     // Frequency control word, middle INT8U.
  INT8U FREQ0;     // Frequency control word, low INT8U.
  INT8U MDMCFG4;   // Modem configuration.
  INT8U MDMCFG3;   // Modem configuration.
  INT8U MDMCFG2;   // Modem configuration.
  INT8U MDMCFG1;   // Modem configuration.
  INT8U MDMCFG0;   // Modem configuration.
  INT8U CHANNR;    // Channel number.
  INT8U DEVIATN;   // Modem deviation setting (when FSK modulation is enabled).
  INT8U FREND1;    // Front end RX configuration.
  INT8U FREND0;    // Front end RX configuration.
  INT8U MCSM0;     // Main Radio Control State Machine configuration.
  INT8U FOCCFG;    // Frequency Offset Compensation Configuration.
  INT8U BSCFG;     // Bit synchronization Configuration.
  INT8U AGCCTRL2;  // AGC control.
  INT8U AGCCTRL1;  // AGC control.
  INT8U AGCCTRL0;  // AGC control.
  INT8U FSCAL3;    // Frequency synthesizer calibration.
  INT8U FSCAL2;    // Frequency synthesizer calibration.
  INT8U FSCAL1;    // Frequency synthesizer calibration.
  INT8U FSCAL0;    // Frequency synthesizer calibration.
  INT8U FSTEST;    // Frequency synthesizer calibration control
  INT8U TEST2;     // Various test settings.
  INT8U TEST1;     // Various test settings.
  INT8U TEST0;     // Various test settings.
  INT8U IOCFG2;    // GDO2 output pin configuration
  INT8U IOCFG0;    // GDO0 output pin configuration
  INT8U PKTCTRL1;  // Packet automation control.
  INT8U PKTCTRL0;  // Packet automation control.
  INT8U ADDR;      // Device address.
  INT8U PKTLEN;    // Packet length.
} RF_SETTINGS;
/////////////////////////////////////////////////////////////////
const RF_SETTINGS rfSettings =
{
  0x00,
  0x08,   // FSCTRL1   Frequency synthesizer control.
  0x00,   // FSCTRL0   Frequency synthesizer control.
  0x10,   // FREQ2     Frequency control word, high byte.
  0xB0,   // FREQ1     Frequency control word, middle byte.
  0x71,   // FREQ0     Frequency control word, low byte.
  
  0x5B,   // MDMCFG4   Modem configuration.
  0xF8,   // MDMCFG3   Modem configuration. 
  0x30,   // MDMCFG2   Modem configuration.
  0x22,   // MDMCFG1   Modem configuration.
  0xF8,   // MDMCFG0   Modem configuration.
  
  0x00,   // CHANNR    Channel number.
  0x47,   // DEVIATN   Modem deviation setting (when FSK modulation is enabled).
  0xB6,   // FREND1    Front end RX configuration.
  0x17,   // FREND0    Front end RX configuration.
  0x18,   // MCSM0     Main Radio Control State Machine configuration.
  0x1D,   // FOCCFG    Frequency Offset Compensation Configuration.
  0x1C,   // BSCFG     Bit synchronization Configuration.
  0x45,   // AGCCTRL2  AGC control.
  0x40,   // AGCCTRL1  AGC control.
  0x91,   // AGCCTRL0  AGC control.
  0xEA,   // FSCAL3    Frequency synthesizer calibration.
  0x2A,   // FSCAL2    Frequency synthesizer calibration.
  0x00,   // FSCAL1    Frequency synthesizer calibration.
  0x11,   // FSCAL0    Frequency synthesizer calibration.
  0x59,   // FSTEST    Frequency synthesizer calibration.
  0x81,   // TEST2     Various test settings.
  0x35,   // TEST1     Various test settings.
  0x09,   // TEST0     Various test settings.
  0x0B,   // IOCFG2    GDO2 output pin configuration.
  0x0D,   // IOCFG0D   GDO0 output pin configuration. Refer to SmartRF?Studio User Manual for detailed pseudo register explanation.
  0x04,   // PKTCTRL1  Packet automation control.
  //0x05,   // PKTCTRL0  Packet automation control.
  0x32, //PKTCTRL0  crc disable chang by allen at 09.12.24
  0x00,   // ADDR      Device address.
  0x0c    // PKTLEN    Packet length.
};

//*****************************************************************************************
//��������delay(unsigned int s)
//���룺ʱ��
//�������
//������������ͨ͢ʱ,�ڲ���
//*****************************************************************************************		
static void delay(u16 s)
{
  u16 i;
  for(i=0; i<s; i++);
  for(i=0; i<s; i++);
}

static void Delay(vu32 nCount)
{
  int i,j;
  for(j=0;j<nCount;j++)
  {
    for(i=0;i<10;i++);
  }
}

void halWait(INT16U timeout)
{
  do 
  {
    delay_us(15);
  } while (--timeout);
}
/*****************************************************************************************
//��������CpuInit()
//���룺��
//�������
//����������SPI��ʼ������
*****************************************************************************************/
void CpuInit(void)
{
  SPI_InitTypeDef SPI_InitStr;
  GPIO_InitTypeDef GPIO_InitStr;
  EXTI_InitTypeDef EXTI_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  
  /*ʹ��GPIOB,GPIOD,���ù���ʱ��*/
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB|RCC_APB2Periph_AFIO, ENABLE);
  
  /*ʹ��SPI1ʱ��*/
  RCC_APB1PeriphClockCmd(	RCC_APB1Periph_SPI2,  ENABLE );//SPI1ʱ��ʹ�� 
  
  /*��Ӳ��stm32��SPI����*/
  /*���� SPI_LDC_SPI�� SCK,MISO,MOSI���ţ�GPIOB^13,GPIOB^14,GPIOB^15 */
  GPIO_InitStr.GPIO_Pin = GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;
  GPIO_InitStr.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStr.GPIO_Mode = GPIO_Mode_AF_PP; //���ù���
  GPIO_Init(GPIOB, &GPIO_InitStr);
  
  /*�Դӻ�LCD�Ŀ��ƽ�����*/
  /*����CS ����: GPIOB^12,����LCD A0���ţ�GPIOB^10,���ø�λ����RESET,GPIOB^11*/
  GPIO_InitStr.GPIO_Pin = GPIO_Pin_12;
  GPIO_InitStr.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStr.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOB, &GPIO_InitStr);
  
  
  //GPIO_InitStr.GPIO_Pin = GPIO_Pin_8| GPIO_Pin_9;
  //GPIO_InitStr.GPIO_Mode = GPIO_Mode_IPU;
  // GPIO_Init(GPIOB, &GPIO_InitStr);
  
  
  /*
  //�����жϷ���
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource11);
  EXTI_InitStructure.EXTI_Line	  = EXTI_Line11; 	 
  EXTI_InitStructure.EXTI_Mode	  = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising; 
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;  
  
  EXTI_Init(&EXTI_InitStructure);
  
  NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn; 
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =3; 
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;	 
  NVIC_InitStructure.NVIC_IRQChannelCmd  = ENABLE;
  NVIC_Init(&NVIC_InitStructure);	 
  
  
  */
  
  
  
  
  SPI_InitStr.SPI_Direction = SPI_Direction_2Lines_FullDuplex; //˫��ȫ˫��
  SPI_InitStr.SPI_Mode = SPI_Mode_Master;	 					//��ģʽ
  SPI_InitStr.SPI_DataSize = SPI_DataSize_8b;	 				//���ݴ�С8λ
  SPI_InitStr.SPI_CPOL = SPI_CPOL_High;		 				//ʱ�Ӽ��ԣ�����ʱΪ��
  SPI_InitStr.SPI_CPHA = SPI_CPHA_2Edge;						//��1��������Ч��������Ϊ����ʱ��
  SPI_InitStr.SPI_NSS = SPI_NSS_Soft;		   					//NSS�ź����������
  SPI_InitStr.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_128;  //2��Ƶ��36MHz/2
  SPI_InitStr.SPI_FirstBit = SPI_FirstBit_MSB;  				//��λ��ǰ
  SPI_InitStr.SPI_CRCPolynomial = 7;                          //CRCУ�鸴λ
  SPI_Init(SPI2, &SPI_InitStr);
  SPI_Cmd(SPI2, ENABLE);
}
void Ask_gpio_init(void)
{
  
  GPIO_InitTypeDef GPIO_InitStr;
  //SPI_Cmd(SPI2, DISABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
  
  GPIO_InitStr.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStr.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStr.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOB, &GPIO_InitStr);
  
  
  GPIO_InitStr.GPIO_Pin = GPIO_Pin_8;
  GPIO_InitStr.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_Init(GPIOB, &GPIO_InitStr);
  
}
//*****************************************************************************************
//��������SpisendByte(INT8U dat)
//���룺���͵�����
//�������
//����������SPI����һ���ֽ�
//*****************************************************************************************
u8 SpiTxRxByte(u8 TxData)
{		
  u8 retry=0;				 	
  while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET) //���ָ����SPI��־λ�������:���ͻ���ձ�־λ
  {
    retry++;
    if(retry>200)return 0;
  }			  
  SPI_I2S_SendData(SPI2, TxData); //ͨ������SPIx����һ������
  retry=0;
  
  while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET) //���ָ����SPI��־λ�������:���ܻ���ǿձ�־λ
  {
    retry++;
    if(retry>200)return 0;
  }	  						    
  return SPI_I2S_ReceiveData(SPI2); //����ͨ��SPIx������յ�����					    
}
//*****************************************************************************************
//��������void RESET_CC1100(void)
//���룺��
//�������
//������������λCC1100
//*****************************************************************************************
void RESET_CC1100(void) 
{
  CSN = 0; 
  while (MISO);
  SpiTxRxByte(CCxxx0_SRES); 		//д�븴λ����
  while (MISO); 
  CSN = 1; 
}
//*****************************************************************************************
//��������void POWER_UP_RESET_CC1100(void) 
//���룺��
//�������
//�����������ϵ縴λCC1100
//*****************************************************************************************
void POWER_UP_RESET_CC1100(void) 
{
  CSN = 1; 
  halWait(2); 
  CSN = 0; 
  halWait(3); 
  CSN = 1; 
  halWait(41); 
  RESET_CC1100();   		//��λCC1100
}
//*****************************************************************************************
//��������void halSpiWriteReg(INT8U addr, INT8U value)
//���룺��ַ��������
//�������
//����������SPIд�Ĵ���
//*****************************************************************************************
void halSpiWriteReg(INT8U addr, INT8U value) 
{
  CSN = 0;
  while (MISO);
  SpiTxRxByte(addr);		//д��ַ
  SpiTxRxByte(value);		//д������
  CSN = 1;
}
//*****************************************************************************************
//��������void halSpiWriteBurstReg(INT8U addr, INT8U *buffer, INT8U count)
//���룺��ַ��д�뻺������д�����
//�������
//����������SPI����д���üĴ���
//*****************************************************************************************
void halSpiWriteBurstReg(INT8U addr, INT8U *buffer, INT8U count) 
{
  INT8U i, temp;
  temp = addr | WRITE_BURST;
  CSN = 0;
  while (MISO);
  SpiTxRxByte(temp);
  for (i = 0; i < count; i++)
  {
    SpiTxRxByte(buffer[i]);
  }
  CSN = 1;
}
//*****************************************************************************************
//��������void halSpiStrobe(INT8U strobe)
//���룺����
//�������
//����������SPIд����
//*****************************************************************************************
void halSpiStrobe(INT8U strobe) 
{
  CSN = 0;
  while (MISO);
  SpiTxRxByte(strobe);		//д������
  CSN = 1;
}
//*****************************************************************************************
//��������INT8U halSpiReadReg(INT8U addr)
//���룺��ַ
//������üĴ�����������
//����������SPI���Ĵ���
//*****************************************************************************************
INT8U halSpiReadReg(INT8U addr) 
{
  INT8U temp, value;
  temp = addr|READ_SINGLE;//���Ĵ�������
  CSN = 0;
  while (MISO);
  SpiTxRxByte(temp);
  value = SpiTxRxByte(0);
  CSN = 1;
  return value;
}
//*****************************************************************************************
//��������void halSpiReadBurstReg(INT8U addr, INT8U *buffer, INT8U count)
//���룺��ַ���������ݺ��ݴ�Ļ��������������ø���
//�������
//����������SPI����д���üĴ���
//*****************************************************************************************
void halSpiReadBurstReg(INT8U addr, INT8U *buffer, INT8U count) 
{
  INT8U i,temp;
  temp = addr | READ_BURST;		//д��Ҫ�������üĴ�����ַ�Ͷ�����
  CSN = 0;
  while (MISO);
  SpiTxRxByte(temp);   
  for (i = 0; i < count; i++) 
  {
    buffer[i] = SpiTxRxByte(0);
  }
  CSN = 1;
}
//*****************************************************************************************
//��������INT8U halSpiReadReg(INT8U addr)
//���룺��ַ
//�������״̬�Ĵ�����ǰֵ
//����������SPI��״̬�Ĵ���
//*****************************************************************************************
INT8U halSpiReadStatus(INT8U addr) 
{
  INT8U value,temp;
  temp = addr | READ_BURST;		//д��Ҫ����״̬�Ĵ����ĵ�ַͬʱд�������
  CSN = 0;
  while (MISO);
  SpiTxRxByte(temp);
  value = SpiTxRxByte(0);
  CSN = 1;
  return value;
}
//*****************************************************************************************
//��������void halRfWriteRfSettings(RF_SETTINGS *pRfSettings)
//���룺��
//�������
//��������������CC1100�ļĴ���
//*****************************************************************************************
void halRfWriteRfSettings_AskTxd(void) 
{
  halSpiWriteReg(CCxxx0_IOCFG2,0x0B);  //GDO2 Output Pin Configuration
  halSpiWriteReg(CCxxx0_IOCFG0,0x0c);  //GDO0 Output Pin Configuration
  halSpiWriteReg(CCxxx0_FIFOTHR,0x47); //RX FIFO and TX FIFO Thresholds
  halSpiWriteReg(CCxxx0_PKTCTRL0,0x32);//Packet Automation Control
  halSpiWriteReg(CCxxx0_FSCTRL1,0x06); //Frequency Synthesizer Control
  halSpiWriteReg(CCxxx0_FREQ2,0x10);   //Frequency Control Word, High Byte
  halSpiWriteReg(CCxxx0_FREQ1,0xB0);   //Frequency Control Word, Middle Byte
  halSpiWriteReg(CCxxx0_FREQ0,0x71);   //Frequency Control Word, Low Byte
  halSpiWriteReg(CCxxx0_MDMCFG4,0x78); //Modem Configuration
  halSpiWriteReg(CCxxx0_MDMCFG3,0x93); //Modem Configuration
  halSpiWriteReg(CCxxx0_MDMCFG2,0x30); //Modem Configuration
  
  halSpiWriteReg(CCxxx0_AGCCTRL2, 0x03);
  halSpiWriteReg(CCxxx0_AGCCTRL1, 0x00);
  halSpiWriteReg(CCxxx0_AGCCTRL0, 0x92);
  
  halSpiWriteReg(CCxxx0_DEVIATN,0x15); //Modem Deviation Setting
  halSpiWriteReg(CCxxx0_MCSM0,0x18);   //Main Radio Control State Machine Configuration
  halSpiWriteReg(CCxxx0_FOCCFG,0x16);  //Frequency Offset Compensation Configuration
  halSpiWriteReg(CCxxx0_WORCTRL,0xFB); //Wake On Radio Control
  halSpiWriteReg(CCxxx0_FREND0,0x11);  //Front End TX Configuration
  halSpiWriteReg(CCxxx0_FSCAL3,0xE9);  //Frequency Synthesizer Calibration
  halSpiWriteReg(CCxxx0_FSCAL2,0x2A);  //Frequency Synthesizer Calibration
  halSpiWriteReg(CCxxx0_FSCAL1,0x00);  //Frequency Synthesizer Calibration
  halSpiWriteReg(CCxxx0_FSCAL0,0x1F);  //Frequency Synthesizer Calibration
  halSpiWriteReg(CCxxx0_TEST2,0x81);   //Various Test Settings
  halSpiWriteReg(CCxxx0_TEST1,0x35);   //Various Test Settings
  halSpiWriteReg(CCxxx0_TEST0,0x09);   //Various Test Settings	
  
  
  
}

void halRfWriteRfSettings_AskRxd(void) 
{
  halSpiWriteReg(CCxxx0_IOCFG2, 0x0d);
  halSpiWriteReg(CCxxx0_IOCFG0, 0x0D);
  halSpiWriteReg(CCxxx0_FIFOTHR, 0x47);	 
  
  halSpiWriteReg(CCxxx0_PKTCTRL1, 0x04);
  halSpiWriteReg(CCxxx0_PKTCTRL0, 0x32);
  
  halSpiWriteReg(CCxxx0_FSCTRL1, 0x06);
  halSpiWriteReg(CCxxx0_FSCTRL0, 0x00);
  
  //???? = 26M / (2 ^ 16) * 0x10b071 = 433,919,830.322265625
  halSpiWriteReg(CCxxx0_FREQ2, 0x10);
  halSpiWriteReg(CCxxx0_FREQ1, 0xB0);
  halSpiWriteReg(CCxxx0_FREQ0, 0x8A);
  
  halSpiWriteReg(CCxxx0_MDMCFG4, 0x58);
  halSpiWriteReg(CCxxx0_MDMCFG3, 0x93);
  halSpiWriteReg(CCxxx0_MDMCFG2, 0x30);
  halSpiWriteReg(CCxxx0_MDMCFG1, 0x22);
  halSpiWriteReg(CCxxx0_MDMCFG0, 0xF8);
  
  halSpiWriteReg(CCxxx0_DEVIATN, 0x15);
  halSpiWriteReg(CCxxx0_CHANNR, 0x00);
  
  halSpiWriteReg(CCxxx0_MCSM2, 0x07);
  halSpiWriteReg(CCxxx0_MCSM1, 0x30);
  halSpiWriteReg(CCxxx0_MCSM0, 0x18);
  halSpiWriteReg(CCxxx0_FOCCFG, 0x16);
  
  
  //?IDLE????RX, TX(??FSTXON)?, ??????
  halSpiWriteReg(CCxxx0_BSCFG, 0x6c);
  halSpiWriteReg(CCxxx0_AGCCTRL2, 0x47);
  halSpiWriteReg(CCxxx0_AGCCTRL1, 0x40);
  halSpiWriteReg(CCxxx0_AGCCTRL0, 0x91);
  
  halSpiWriteReg(CCxxx0_WORCTRL, 0xFB);
  halSpiWriteReg(CCxxx0_FREND1, 0x56);
  halSpiWriteReg(CCxxx0_FREND0, 0x11);
  
  halSpiWriteReg(CCxxx0_FSCAL3, 0xe9);
  halSpiWriteReg(CCxxx0_FSCAL2, 0x2A);
  halSpiWriteReg(CCxxx0_FSCAL1, 0x00);
  halSpiWriteReg(CCxxx0_FSCAL0, 0x1f);
  
  //halSpiWriteReg(CCxxx0_FSTEST, 0x59);
  
  halSpiWriteReg(CCxxx0_TEST2, 0x81);
  halSpiWriteReg(CCxxx0_TEST1, 0x35);
  halSpiWriteReg(CCxxx0_TEST0, 0x09);
  
  halSpiWriteReg(CCxxx0_ADDR, 0x00);
  halSpiWriteReg(CCxxx0_PKTLEN, 0xff);
  
}
void Set_Ask_TxMode(void)
{
  //CpuInit();
  POWER_UP_RESET_CC1100();
  halRfWriteRfSettings_AskTxd();
  halSpiWriteBurstReg(CCxxx0_PATABLE, PaTabel_AskTxd, 8);
  halSpiStrobe(CCxxx0_STX); 
  //Ask_gpio_init();
  
}

void Set_Ask_RxMode(void)
{
  //CpuInit();
  POWER_UP_RESET_CC1100();
  halRfWriteRfSettings_AskRxd();
  halSpiWriteBurstReg(CCxxx0_PATABLE, PaTabel_AskRxd, 8);
  halSpiStrobe(CCxxx0_SRX);		//�������״̬
  //Ask_gpio_init();
  
}
//*****************************************************************************************
//��������void halRfSendPacket(INT8U *txBuffer, INT8U size)
//���룺���͵Ļ��������������ݸ���
//�������
//����������CC1100����һ������
//*****************************************************************************************
void halRfSendPacket(INT8U *txBuffer, INT8U size) 
{
  halSpiWriteReg(CCxxx0_TXFIFO, size);
  //	halSpiWriteReg(CCxxx0_TXFIFO, 0);
  halSpiWriteBurstReg(CCxxx0_TXFIFO, txBuffer, size); //????????
  halSpiStrobe(CCxxx0_STX);  //??????????
  // Wait for GDO0 to be set -> sync transmitted
  while (!GDO0);
  // Wait for GDO0 to be cleared -> end of packet
  while (GDO0);
  halSpiStrobe(CCxxx0_SFTX);
  delay(20);
}
void setRxMode(void)
{
  halSpiStrobe(CCxxx0_SRX);		//�������״̬
}
/*
// Bit masks corresponding to STATE[2:0] in the status byte returned on MISO
#define CCxx00_STATE_BM                 0x70
#define CCxx00_FIFO_BYTES_AVAILABLE_BM  0x0F
#define CCxx00_STATE_TX_BM              0x20
#define CCxx00_STATE_TX_UNDERFLOW_BM    0x70
#define CCxx00_STATE_RX_BM              0x10
#define CCxx00_STATE_RX_OVERFLOW_BM     0x60
#define CCxx00_STATE_IDLE_BM            0x00

static INT8U RfGetRxStatus(void)
{
INT8U temp, spiRxStatus1,spiRxStatus2;
INT8U i=4;// ѭ�����Դ���
temp = CCxxx0_SNOP|READ_SINGLE;//���Ĵ�������
CSN = 0;
while (MISO);
SpiTxRxByte(temp);
spiRxStatus1 = SpiTxRxByte(0);
do
{
SpiTxRxByte(temp);
spiRxStatus2 = SpiTxRxByte(0);
if(spiRxStatus1 == spiRxStatus2)
{
if( (spiRxStatus1 & CCxx00_STATE_BM) == CCxx00_STATE_RX_OVERFLOW_BM)
{
halSpiStrobe(CCxxx0_SFRX);
return 0;
			}
return 1;
		}
spiRxStatus1=spiRxStatus2;
	}
while(i--);
CSN = 1;
return 0;	
}
*/
INT8U halRfReceivePacket(INT8U *rxBuffer, INT8U *length) 
{
  INT8U status[2];
  INT8U packetLength;
  INT8U i=(*length)*4;  // �������Ҫ����datarate��length������
  
  halSpiStrobe(CCxxx0_SRX);		//�������״̬
  //delay(5);
  //while (!GDO1);
  //while (GDO1);
  delay(2);
  while (GDO0)
  {
    delay(2);
    --i;
    if(i<1)
      return 0; 	    
  }	 
  if ((halSpiReadStatus(CCxxx0_RXBYTES) & BYTES_IN_RXFIFO)) //����ӵ��ֽ�����Ϊ0
  {
    packetLength = halSpiReadReg(CCxxx0_RXFIFO);//������һ���ֽڣ����ֽ�Ϊ��֡���ݳ���
    if (packetLength <= *length) 		//�����Ҫ����Ч���ݳ���С�ڵ��ڽ��յ������ݰ��ĳ���
    {
      halSpiReadBurstReg(CCxxx0_RXFIFO, rxBuffer, packetLength); //�������н��յ�������
      *length = packetLength;				//�ѽ������ݳ��ȵ��޸�Ϊ��ǰ���ݵĳ���
      
      // Read the 2 appended status bytes (status[0] = RSSI, status[1] = LQI)
      halSpiReadBurstReg(CCxxx0_RXFIFO, status, 2); 	//����CRCУ��λ
      halSpiStrobe(CCxxx0_SFRX);		//��ϴ���ջ�����
      return (status[1] & CRC_OK);			//���У��ɹ����ؽ��ճɹ�
    }
    else 
    {
      *length = packetLength;
      halSpiStrobe(CCxxx0_SFRX);		//��ϴ���ջ�����
      return 0;
    }
  } 
  else
    return 0;
}







