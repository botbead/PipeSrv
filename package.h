#ifndef _PACKAGE_H
#define _PACKAGE_H

/**********
 定义结构体用于解析数据

 ***********/
typedef struct

{
	unsigned char head[2]; // 数据头
	unsigned char length; // 数据长度
	unsigned char cTrl; // 数据指令

	unsigned char ID[4]; // 设备ID

	float price; // 制水单价
	float balance; // 余额
	float gross; // 制水总量
	float amount; // 本次制水量
	float purityTDS; // 净水TDS
	float rawTDS; // 原水TDS
	float ready1; // 备用字节1
	float ready2; // 备用字节2

	unsigned char chargingMode; // 计费模式
	unsigned char Family; // 设备系列
	unsigned char eState; // 设备状态码
	unsigned char check; // 校验码
} TcpData;

/***********

 定义联合体 用于数据拆解成字节

 ************/
typedef union {
	TcpData accData;
	unsigned char stream[sizeof(TcpData)];
} TransferUnion;

char Data_Analysis(unsigned char *buf);
char Data_Analyze(unsigned char* in_buf, unsigned char* out_buf);

#endif // !_MAIN_H
