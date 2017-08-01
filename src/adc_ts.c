
#include "delay.h"
#include "main.h"
#include "GenericTypeDefs.h"
#include "stm32f10x.h"
#include <string.h>
#define Data_40 temp_table[34]
#define Data_6 temp_table[0]
#define NUMBER_SAMPLES 10
/*
 unsigned short temp_table[35]=//5k
{ 
 // 80299,77263,74328,71542,68905, 66417,64029,61741,
   64029,61741,59587,57522,55557,53681,51895,//6℃++
   50189,48567,47014,45537,44124,42776,41487,//13
   40258,39054,37960,36885,35855,34875,33935,//14
   33034,32174,31353,30562,29810,29084,28393, //21
   27726,27089,26477,25895,25333,24791,24273//28
};
*/
/*
 unsigned short temp_table[35]=//10k
{ 
   13058 ,12468,11910,11384,10887,10417,9975,//6℃++
   9556,9162,8789,8437,8104,7789,7492,//13
   7210,6942,6689,6448,6219,6000,5757,//20
   5532,5324,5132,4952,4784,4627,4480, //27
   4341,4210,4085,3966,3853,3745,3642//34
};
*/
/*

unsigned short temp_table[35]=//10k-2k
{ 
   314 ,329,344,360,376,393,411,//6℃++
   429,447,466,486,505,526,548,//13
   568,590,612,635,658,683,712,//20
   740,769,798,827,856,885,914, //27
   944,973,1003,1033,1063,1094,1125//34
};
*/
/*
unsigned short temp_table[35]=//10k-2k
{ 
   901 ,937,973,1010,1048,1087,1125,1165,//6℃++
   1205,1245,1285,1326,1367,1408,//14
   1449,1490,1532,1574,1616,1658,1707,1756,//20
   1803,1849,1894,1938,1982,2024,2066, //28
   2107,2147,2188,2227,2266,2305,//35
};
*/
/*
unsigned short temp_table[35]=//10k-2k
{ 
573,593,614,635,658,681,705,731,
757,786,816,847,882,918,
957,1000,1045,1095,1149,1206,1269,1335,
1405,1478,1553,1627,1699,1763,1814,
1848,1856,1929,2007,2089,2175//35
};
*/

u16 temp_table[35]=//10k-2k
{ 
   2175 ,2089,2007,1929,1856,1848,1814,1763,//6℃++
   1699,1627,1553,1478,1405,1335,//14
   1269,1206,1149,1095,1045,1000,957,918,//20
   882,847,816,786,757,731,705, //28
   681,658,635,614,593,573,//35
};


#define FILTER_LIMIT 5

static BOOL
Touch_Pen_filtering(uint16_t *px)
{
	BOOL RetVal = TRUE;
        
	static u8 count = 0;
	static uint16_t x[2];
	uint16_t TmpX ;
	uint16_t dx;
	uint16_t Filter_Margin;
	
    if(*px<1)
    	{count=0;
	return FALSE;
    	}
   else  count++;
   	
     if (count > 2) 
     { 
	count = 2;
	TmpX = (x[0] + *px) >>1;
        dx = (x[1] > TmpX) ? (x[1] - TmpX) : (TmpX - x[1]);
        Filter_Margin = (x[1] > x[0]) ? (x[1]-x[0]) : (x[0]-x[1]);  
        Filter_Margin += FILTER_LIMIT;
	if (dx > Filter_Margin)
          {  
	     *px = x[1];
	      RetVal = FALSE;
	      count = 0;
	  } 
	else		
	{			
	      x[0] = x[1];	
	      x[1] = *px;  
	      RetVal = TRUE;
	}
		
    }
   else { 
	     x[0] = x[1];	
	     x[1] = *px; 
	     RetVal = FALSE;
	}
	return RetVal;
}






u16 ADC_temp(void)
{
	uint8_t i,j,k;
        uint16_t rgPointSamples[NUMBER_SAMPLES];
	uint16_t res,temp;
// ADC_temp_Init();

/* initialize result */
  res = 0;
  for(i=NUMBER_SAMPLES; i>0; i--)
  {
/* start ADC convertion by software */
  //  ADC_SoftwareStartConv(ADC1);
    ADC_SoftwareStartConvCmd( ADC1, ENABLE );                           //使能指定的ADC1的软件转换启动功能
/* wait until end-of-covertion */
    while( ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == 0 );
/* read ADC convertion result */
    rgPointSamples[i]= ADC_GetConversionValue(ADC1);
  }
  
 for (j = 0; j < NUMBER_SAMPLES; ++j)  
    {  
        for (k = j+1; k < NUMBER_SAMPLES; ++k)  
            {  
             if(rgPointSamples[ j ]>rgPointSamples[ k ])  
                {  
                temp = rgPointSamples[ j ];  
                rgPointSamples[ j ]=rgPointSamples[ k ];  
                rgPointSamples[ k ]=temp;  
               }  
           }  
     }  
    res = rgPointSamples[ 2 ] +  rgPointSamples[ 3 ] +  rgPointSamples[ 4 ] + rgPointSamples[ 5 ] ;
 //   res = (res+((rgPointSamples[ 3] + rgPointSamples[ 4 ] )<<1));//>>3
res=res/4;
 //   if ((res & 0x7) > 3) res = (res>>3) + 1;
 //   else res = res>>3;
//ADC_DeInit(ADC1);
 // CLK_PeripheralClockConfig(CLK_Peripheral_ADC1, DISABLE);
 // ADC_ChannelCmd(ADC1, ADC_Channel_24, DISABLE);	
  
  return (res);
}




 void Lookup_TAB(uint16_t data, uint16_t *TABLE,uint16_t *aptr)
{
u8 i;
uint16_t pp;
uint16_t *eptr=(uint16_t *)&TABLE[34]; //高端指针
uint16_t *sptr=(uint16_t *)TABLE; //低端指针
uint16_t *ptr; //查表指针
for(i=0;i<6;i++) //搜索全表
{
  pp=(((uint16_t)eptr-(uint16_t)sptr))>>1;
  ptr = (uint16_t *)((uint16_t)sptr+((pp&0x0001)?(pp+1):pp) );

if(*(ptr)<data) sptr = ptr;
 else if(*(ptr)>data) eptr = ptr;
    else //查到相等的节点
       {
         aptr[2] = *ptr; //Y1
         aptr[1] = *(ptr+1); //Y2
         aptr[0] = (uint16_t)(ptr-TABLE);//X1
         break;
       }
if(eptr-sptr==1) //查到节点的范围
{
  aptr[2] = *sptr; //Y1
aptr[1] = *eptr; //Y2
aptr[0] = (uint16_t)(sptr-TABLE);//X1
break;
}
}
}



//分段线性插值
 uint16_t LinearInsert(uint16_t data, uint16_t *TABLE)//范围值高八位为温度整数，第八位为小数后两位
{
uint16_t Array[3];
uint16_t Temp;
  if((data)<(Data_6)) //?Beyond Lowest Temperature
    {
      if(data > Data_40) //?Beyond Highest Temperature
        {
          Lookup_TAB(data,TABLE,Array); //Lookup data
         if(data == Array[2]) 
         {
           Temp = (Array[0] +6);
           Temp = (Array[0] +6)<<8;
         }
         else 
            Temp = ((Array[0]+6)<<8)|((data-Array[2])*100/(Array[1]-Array[2]));
        }
     else 
       Temp = 0xFFFF; //高于40°
    }
   else 
     Temp = 0x7FFF; //低于6°
return(Temp);
}



u16 Gettemp(void)
{
  u8 i=0;

u16 tempdata;
u16 data,data1,data2,data3;
//uint16_t tab[3];



tempdata = ADC_temp();

//Touch_Pen_filtering(&tempdata);

 



data1 = 3300-tempdata;



data2 = (tempdata*1000)/data1;
//data3 = (data2*10)%tempdata;

//data1 =(data3*10)/tempdata;
//data =data+data1*10;
//ata4= (data3*10)%tempdata;

for(i=0;i<34;)
{
  if(data2<temp_table[i])
  {
  i++;
  }
 // data= temp_table[i-1]-temp_table[i];
//data3=data2-temp_table[i-1];
// data=(data3*data);
//tempdata=(i+7)*1000+data;
  
 // printf("i=%d\r\n");
   break;   
}
printf("i=%d\r\n",i);

//tempdata=LinearInsert( data2, temp_table);

// tab[5] = ' ';
		
/* Test the significant digit for displays 3 or 4 digits*/
 
    /* To shift for suppress '0' before decimal */
 //   tab[1]=  tab[1] | DOT ;	
    
tempdata=i+19;
	
 
 return tempdata;
// LCD_GLASS_DisplayStrDeci(tab,4);
//LCD_GLASS_DisplayStrDeci(tab,6);
}

void conv_into_char(uint16_t number, uint16_t *p_tab)
{
  uint16_t Integer=(number&0xff00)>>8;
  uint16_t Fraction=(number&0x00ff);
  uint16_t Integer_units=0, Integer_tens=0, Fraction_units=0, Fraction_tens=0;
  

  
  Integer_units =Integer%10;
  Integer_tens = (Integer-Integer_units)/10;
  Fraction_units =Fraction%10 ;
  Fraction_tens =(Fraction-Fraction_units)/10 ;
 
  if(Fraction_units>4) 
      Fraction_tens +=1;
 if(Fraction_tens>9)
   {   Fraction_tens=0;
       Integer_units +=1;
   }   
  if(Integer_units>9)
   { Integer_units=0;
     Integer_tens +=1; 
   } 

/* */

 // *(p_tab+4) = ' ';
 //*(p_tab+3) = Fraction_units+0x30;
 // *(p_tab+3) = ' ';
  *(p_tab+2) = Fraction_tens + 0x30;
  *(p_tab+1) = Integer_units + 0x30;
  *(p_tab) = Integer_tens + 0x30;

}

void time_into_char(uint16_t number, uint16_t *p_tab)
{
  uint16_t Integer=(number&0xff00)>>8;
  uint16_t Fraction=(number&0x00ff);
  uint16_t Integer_units=0, Integer_tens=0, Fraction_units=0, Fraction_tens=0;
  

  
  Integer_units =Integer%10;
  Integer_tens = (Integer-Integer_units)/10;
  Fraction_units =Fraction%10 ;
  Fraction_tens =(Fraction-Fraction_units)/10 ;
*(p_tab)=(uint16_t)(Integer_tens + 0x30);
*(p_tab+1)=(uint16_t)(Integer_units + 0x30);
*(p_tab+2)=(uint16_t)(Fraction_tens+ 0x30);
*(p_tab+3)=(uint16_t)(Fraction_units + 0x30);
}









