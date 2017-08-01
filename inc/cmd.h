#ifndef _CMD_H
#define _CMD_H

/*************************************************************************
**Copyright (c) 2010
** All rights reserved.
**
** File name:		
** Descriptions:		
**------------------------------------------------------------------------
** Created by:			
** Created date:	
** Version:				
** Descriptions:		The original version
**------------------------------------------------------------------------
** Modified by:
** Modified date:
** Version:
** Descriptions:
**************************************************************************/

#define   Heartbeat_cmd       0x01

#define   Bufang_remote          0x02
#define   Chefang_remote         0x03
#define   ALM_remote             0x04
#define   Doorbell_remote        0x05


#define   Study_remote        0x06
#define   Study_meci          0x07
#define   Study_infrared      0x08
#define   Study_smoke         0x09
#define   Study_gas           0x0a
#define   Study_arlrm         0x0b


/************************************
#define   Study_other      0x0b---0x0f      
**************************************/


#define   Delete_remote        0x10
#define   Delete_meci          0x11
#define   Delete_infrared      0x12
#define   Delete_smoke         0x13
#define   Delete_gas           0x14
#define   Delete_arlrm         0x15


#define   Delete_remote_all        0x30
#define   Delete_meci_all          0x31
#define   Delete_infrared_all      0x32
#define   Delete_smoke_all         0x33
#define   Delete_gas_all           0x34
#define   Delete_arlrm_all         0x35

/************************************
#define   Delete_other      0x15---0x19      
**************************************/


#define study_from_net 1
#define study_from_key 0
//#define   ALM_remote        0x10
#define   ALM_meci          0x1a
#define   ALM_infrared      0x1b
#define   ALM_smoke         0x1c
#define   ALM_gas           0x1d
#define   ALM_key          0x1e
/************************************
#define  ALM_other      0x1e---0x1f     
**************************************/


#define   Correction_time    0x20

#define   Bufang_NET         0x21
#define   Chefang_NET        0x22

#define   Volume_set         0x23

#define   IP_set             0x24

#define Heartbeat_Cycle      0x25

#define save_433_data        0x26

#define get_433_data         0x27

#define ALM_net              0x28

#define voice_mode           0x29
//#define open_voice           0x29
#define send_dev_info        0x2a
//#define set_voice            0x2b

#define study_app            0x50
#define delete_app           0x51
#define save_app             0x52
#define check_app            0x53
#define restore_app          0x54
#define delete_sensor        0x55



#define Study_433_device         0x40
#define Delete_433_device        0x41
#define Send_433_device          0x42

#define zhuhe_mode               0x43
#define set_self_mode            0x44
#define sleep_mode               0x45


#define reset_net                0x4a
#define en_disable_time_ctl     0x5f
#define study_kaiguan           0x60   //学习金西瑞开关
#define save_guanlin            0x61  //存储关联控制指令
#define save_Scenario           0x62  //存储预设情景模式指令
#define delete_guanlin          0x63  //删除关联控制指令
#define delete_Scenario         0x64  //删除预设情景模式指令
#define check_guanlin           0x65  //查询关联控制指令
#define check_Scenario          0x66  //查询关联控制指令
#define ctl_Scenario            0x67  //查询关联控制指令
#define get_back_gl             0x68


#define Study_IR                0x70  //查询关联控制指令
#define SET_IR_MODE             0x71  //查询关联控制指令
#define DELE_IR_MODE            0x72  //查询关联控制指令
#define ctl_IR_MODE             0x73  //查询关联控制指令
#define ctl_IR                  0x74  //查询关联控制指令

#define time_ok               0xaa
#define time_off              0xff

#define hand_time             0x80 //
#define PA_en_disable         0x81
#define PA                    0x24



#define xiaomi                0x01  //小米
#define letv                  0x02  //乐视
#define sony                  0x03  //索尼
#define midea                 0x04  //美的
#define gree                  0x05  //格力
#define samsung               0x06  //三星
#define nec                   0x10  //nec类型
//#define PA_disable            0x83
/************************************
#define  ALM_other      0x1e---0x1f     
**************************************/

















#endif

