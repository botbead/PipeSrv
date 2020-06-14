#include "package.h"
#include <stdio.h>
#include <vcl.h>
#include "main_gui.h"
// 模拟数据

// unsigned char Temp_Buf[9] = { 0xaa, 0x44, 0x09, 0x01, 0xDA, 0XE3, 0X4F, 0X4B, 0XDB };
/*
 unsigned char Temp_Buf[44] = { 0Xaa, 0X44, 0X2c, 0X2, 0Xaa, 0Xbb, 0Xcc, 0Xdd, 0X9a, 0X99, 0X99, 0X3e, 0X9a, 0Xf9, 0X79, 0X44, 0X85, 0Xeb, 0X34, 0X42, 0X66, 0X66, 0X16, 0X40, 0X48, 0Xe1, 0Xaa, 0X41, 0X7b, 0Xd4, 0Xd9, 0X43, 0Xda, 0Xf, 0X49, 0X40, 0X42, 0X3e, 0X4e, 0X40, 0X1, 0X78, 0X1, 0Xd5};
 unsigned char Buf[44];      //发送的包  封包缓冲区
 unsigned char Heart_Buf[5] = { 0xaa, 0x44, 0x05, 0x01, 0xea };  //心跳包返回数据
 unsigned char Data_Buf[6] = { 0xaa, 0x44, 0x06, 0x02, 0x00, 0xea }; //数据包返回报
 */

TransferUnion Envelop_Data; // 封包联合体
TransferUnion Unbind_Data; // 解包联合体

// 封包函数
char Data_Packet(unsigned char *buf) {
	Envelop_Data.accData.head[0] = 0XAA;
	Envelop_Data.accData.head[1] = 0X44; // 数据头

	Envelop_Data.accData.length = 44; // 数据长度
	Envelop_Data.accData.cTrl = 2; // CTRL指令

	Envelop_Data.accData.ID[0] = 0XAA;
	Envelop_Data.accData.ID[1] = 0XBB;
	Envelop_Data.accData.ID[2] = 0XCC;
	Envelop_Data.accData.ID[3] = 0XDD; // 设备id

	Envelop_Data.accData.price = 0.3; // 单价
	Envelop_Data.accData.balance = 999.9; // 余额
	Envelop_Data.accData.gross = 45.23; // 制水总量
	Envelop_Data.accData.amount = 2.35; // 本次制水量

	Envelop_Data.accData.purityTDS = 21.36; // 净水TDS
	Envelop_Data.accData.rawTDS = 435.66; // 原水TDS

	Envelop_Data.accData.ready1 = 3.1415926; // 备用
	Envelop_Data.accData.ready2 = 3.22255; // 备用

	Envelop_Data.accData.chargingMode = 1; // 计费模式
	Envelop_Data.accData.Family = 'x'; // 设备系列
	Envelop_Data.accData.eState = 1; // 设备状态
	Envelop_Data.accData.check = 0;

	// 计算校验码
	for (int i = 0; i < sizeof(Envelop_Data.accData) - 1; i++) {
		Envelop_Data.accData.check ^= Envelop_Data.stream[i];
	}

	// 封包

	for (int i = 0; i < sizeof(Envelop_Data.stream); i++) {
		buf[i] = Envelop_Data.stream[i];
	}

	return 0;
}

// 解包函数
// 返回值  0正确  -1错误   1是心跳包

char Data_Analysis(unsigned char *buf) {
	unsigned char temp_buf = 0;
    Form2->Memo1->Lines->Add("++++++++++++++");
	if (buf[0] == 0xaa && buf[1] == 0x44) // 判断数据头
	{
		for (int i = 0; i < (buf[2] - 1); i++) // 计算校验码
		{
			temp_buf ^= buf[i];
		}
		if (temp_buf != buf[buf[2] - 1]) // 数据错误
		{
			printf("数据校验错误不能参与运算\r\n");
			return -1; // 数据错误
		}
		else {
			printf("数据校验正确可以参与运算\r\n"); // 数据正确
		}

		if (buf[2] == 0x09 && buf[3] == 0x01) // 心跳包
		{
			printf("心跳包\r\n");
			return 1; // 心跳包
		}
		if (buf[3] > 0x02) {
			//return 2;
		}
	}

	// 解包

	for (int i = 0; i < sizeof(Envelop_Data.stream); i++) {
		Envelop_Data.stream[i] = buf[i];
	}
	//OutputDebugStringA("ddddddddd");

	//UnicodeString tmp_s("");
	//FloatToStr(Envelop_Data.accData.balance);

	//OutputDebugStringW(FloatToStr(Envelop_Data.accData.balance).w_str());
	Form2->Memo1->Lines->Add("---------"+FloatToStr(Envelop_Data.accData.balance));

	// 打印测试
	// printf("数据头:%x0x", Envelop_Data.accData.head[0], Envelop_Data.accData.head[1]);
	//
	// printf("数据长度:%x\r\n",Envelop_Data.accData.length);
	// printf("Ctr指令:%x\r\n", Envelop_Data.accData.cTrl);
	// printf("设备ID:%x%x%x%x\r\n", Envelop_Data.accData.ID[0], Envelop_Data.accData.ID[1], Envelop_Data.accData.ID[2],Envelop_Data.accData.ID[3]);
	// printf("制水收费单价:%.2f\r\n", Envelop_Data.accData.price);

	// printf("余额:%.2f\r\n", Envelop_Data.accData.balance);
	// printf("制水总量:%.2f\r\n", Envelop_Data.accData.gross);
	// printf("本次制水量:%.2f\r\n", Envelop_Data.accData.amount);
	// printf("净水TDS:%.2f\r\n", Envelop_Data.accData.purityTDS);
	// printf("原水TDS:%.2f\r\n", Envelop_Data.accData.rawTDS);

	// printf("备用字节1:%.2f\r\n", Envelop_Data.accData.ready1);
	// printf("备用字节2:%.2f\r\n", Envelop_Data.accData.ready2);

	// printf("计费模式:%x\r\n", Envelop_Data.accData.chargingMode);
	// printf("设备系列:%c\r\n", Envelop_Data.accData.Family);
	// printf("设备状态：%x\r\n", Envelop_Data.accData.eState);
	return 0;
}

/*
 int main(void)
 {
 char buf1[4] = { 0x01, 0x02, 0x03, 0x04 };

 char buf2[10];


 //sprintf_s();
 //Data_Packet(Buf);   //调用风暴函数
 //for (int i = 0;i<sizeof(Buf);i++)
 //{
 //	printf("0X%x,", Buf[i]);
 //}


 Data_Analysis(Temp_Buf);  //调用解包函数





 return 0;
 }
 */

char Data_Analyze(unsigned char* in_buf, unsigned char* out_buf) {
	int length = 0; // 返回数据长度
	unsigned char temp_key = 0; // 临时校验码

	for (int i = 0; i < 44; i++) // 清空数据
	{
		out_buf[i] = 0;
	}

	in_buf[2] = 0x09;

	if (in_buf[3] == 3 || in_buf[3] == 4 || in_buf[3] == 5 || in_buf[3] == 9 ||
		in_buf[3] == 10 || in_buf[3] == 11 || in_buf[3] == 12)
		// 关机指令  //开机指令  强冲指令
	{

		for (int i = 0; i < 8; i++) {
			out_buf[i] = in_buf[i]; // 拷贝数据
			temp_key ^= out_buf[i]; // 计算key
			length = i + 2;
		}
		// out_buf[2] = 0x09;
		// for (int i = 0; i < 8; i++) {
		// temp_key ^= out_buf[i];
		// }
		out_buf[8] = temp_key; // key
		// out_buf[8] = 0xe3;

		Form2->Memo1->Lines->Add(IntToStr((int)out_buf[8]));
		return length;
	}

	in_buf[2] = 0x2c;

	// 充值  计费修改 计费模式
	if (in_buf[3] == 6 || in_buf[3] == 7 || in_buf[3] == 8 || in_buf[3] == 13 ||
		in_buf[3] == 14 || in_buf[3] == 15) {
		for (int i = 0; i < 43; i++) {
			out_buf[i] = in_buf[i]; // 拷贝数据
			temp_key ^= out_buf[i]; // 计算key
			length = i + 2;
		}
		// out_buf[2] = 0x2c;
		// for (int i = 0; i < 43; i++) {
		// temp_key ^= out_buf[i];
		// }
		out_buf[43] = temp_key; // key

		return length;

	}
	return length;
}
