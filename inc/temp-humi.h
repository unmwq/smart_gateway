/******************************************************/
/*   temperature and humidity sensor IO definition    */
/******************************************************/
                        //adr       command    r/w
#define MEASURE_TEMP    0x03   //000   0001    1
#define MEASURE_HUMI    0x05   //000   0010    1
#define MEASURE_STATUS  0x06   //000   0011    0
#define TEMP            0
#define HUMI            1
#define NOACK           0
#define ACK             1
#define TEMP_INC        0.5
#define HUMIDITY_INC    4





//#define S_DATA_OUT      GPIO_DIRSETL = BIT(12)	

//#define S_DATA_IN       GPIO_DIRCLRL = BIT(12)
//#define S_DATA_IN       GPIO_DIRCLRL = BIT(12)

#define S_DATA          GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_9)
#define S_DATA_LOW      GPIO_ResetBits(GPIOB, GPIO_Pin_9)
#define S_DATA_HIGH     GPIO_SetBits(GPIOB, GPIO_Pin_9)
//#define S_SCK_OUT       GPIO_DIRSETL = BIT(11)
#define S_SCK_LOW       GPIO_ResetBits(GPIOB, GPIO_Pin_8)
#define S_SCK_HIGH      GPIO_SetBits(GPIOB, GPIO_Pin_8)

void Temp_Humi_Init(void);
void temp_humi(void);