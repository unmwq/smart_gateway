
#ifndef _JQ6500_H_
#define _JQ6500_H_

#include "GenericTypeDefs.h"
#include "stm32f10x.h"



void Specify_musi_play(u8 num);
void Next_Play(void);
void Last_Play(void);
void Specify_Musi_Play(u8 num);
u16 Get_Musi_Status(void);
void Volume_Add();
void Volume_Dec();
void Specify_Volume(u8 num);
void Music_play();
void Music_pause();
void Reset_Device();
void Specify_play_Mode(u8 num);
void Switch_play_Device(u8 num);
void Cycle_play(u8 num);

void ctl_vol_init();
void ctl_vol();
void Rectl_vol();













#endif