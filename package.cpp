#include "package.h"
#include <stdio.h>
#include <vcl.h>
#include "main_gui.h"
// ģ������

// unsigned char Temp_Buf[9] = { 0xaa, 0x44, 0x09, 0x01, 0xDA, 0XE3, 0X4F, 0X4B, 0XDB };
/*
 unsigned char Temp_Buf[44] = { 0Xaa, 0X44, 0X2c, 0X2, 0Xaa, 0Xbb, 0Xcc, 0Xdd, 0X9a, 0X99, 0X99, 0X3e, 0X9a, 0Xf9, 0X79, 0X44, 0X85, 0Xeb, 0X34, 0X42, 0X66, 0X66, 0X16, 0X40, 0X48, 0Xe1, 0Xaa, 0X41, 0X7b, 0Xd4, 0Xd9, 0X43, 0Xda, 0Xf, 0X49, 0X40, 0X42, 0X3e, 0X4e, 0X40, 0X1, 0X78, 0X1, 0Xd5};
 unsigned char Buf[44];      //���͵İ�  ���������
 unsigned char Heart_Buf[5] = { 0xaa, 0x44, 0x05, 0x01, 0xea };  //��������������
 unsigned char Data_Buf[6] = { 0xaa, 0x44, 0x06, 0x02, 0x00, 0xea }; //���ݰ����ر�
 */

TransferUnion Envelop_Data; // ���������
TransferUnion Unbind_Data; // ���������

// �������
char Data_Packet(unsigned char *buf) {
	Envelop_Data.accData.head[0] = 0XAA;
	Envelop_Data.accData.head[1] = 0X44; // ����ͷ

	Envelop_Data.accData.length = 44; // ���ݳ���
	Envelop_Data.accData.cTrl = 2; // CTRLָ��

	Envelop_Data.accData.ID[0] = 0XAA;
	Envelop_Data.accData.ID[1] = 0XBB;
	Envelop_Data.accData.ID[2] = 0XCC;
	Envelop_Data.accData.ID[3] = 0XDD; // �豸id

	Envelop_Data.accData.price = 0.3; // ����
	Envelop_Data.accData.balance = 999.9; // ���
	Envelop_Data.accData.gross = 45.23; // ��ˮ����
	Envelop_Data.accData.amount = 2.35; // ������ˮ��

	Envelop_Data.accData.purityTDS = 21.36; // ��ˮTDS
	Envelop_Data.accData.rawTDS = 435.66; // ԭˮTDS

	Envelop_Data.accData.ready1 = 3.1415926; // ����
	Envelop_Data.accData.ready2 = 3.22255; // ����

	Envelop_Data.accData.chargingMode = 1; // �Ʒ�ģʽ
	Envelop_Data.accData.Family = 'x'; // �豸ϵ��
	Envelop_Data.accData.eState = 1; // �豸״̬
	Envelop_Data.accData.check = 0;

	// ����У����
	for (int i = 0; i < sizeof(Envelop_Data.accData) - 1; i++) {
		Envelop_Data.accData.check ^= Envelop_Data.stream[i];
	}

	// ���

	for (int i = 0; i < sizeof(Envelop_Data.stream); i++) {
		buf[i] = Envelop_Data.stream[i];
	}

	return 0;
}

// �������
// ����ֵ  0��ȷ  -1����   1��������

char Data_Analysis(unsigned char *buf) {
	unsigned char temp_buf = 0;
    Form2->Memo1->Lines->Add("++++++++++++++");
	if (buf[0] == 0xaa && buf[1] == 0x44) // �ж�����ͷ
	{
		for (int i = 0; i < (buf[2] - 1); i++) // ����У����
		{
			temp_buf ^= buf[i];
		}
		if (temp_buf != buf[buf[2] - 1]) // ���ݴ���
		{
			printf("����У������ܲ�������\r\n");
			return -1; // ���ݴ���
		}
		else {
			printf("����У����ȷ���Բ�������\r\n"); // ������ȷ
		}

		if (buf[2] == 0x09 && buf[3] == 0x01) // ������
		{
			printf("������\r\n");
			return 1; // ������
		}
		if (buf[3] > 0x02) {
			//return 2;
		}
	}

	// ���

	for (int i = 0; i < sizeof(Envelop_Data.stream); i++) {
		Envelop_Data.stream[i] = buf[i];
	}
	//OutputDebugStringA("ddddddddd");

	//UnicodeString tmp_s("");
	//FloatToStr(Envelop_Data.accData.balance);

	//OutputDebugStringW(FloatToStr(Envelop_Data.accData.balance).w_str());
	Form2->Memo1->Lines->Add("---------"+FloatToStr(Envelop_Data.accData.balance));

	// ��ӡ����
	// printf("����ͷ:%x0x", Envelop_Data.accData.head[0], Envelop_Data.accData.head[1]);
	//
	// printf("���ݳ���:%x\r\n",Envelop_Data.accData.length);
	// printf("Ctrָ��:%x\r\n", Envelop_Data.accData.cTrl);
	// printf("�豸ID:%x%x%x%x\r\n", Envelop_Data.accData.ID[0], Envelop_Data.accData.ID[1], Envelop_Data.accData.ID[2],Envelop_Data.accData.ID[3]);
	// printf("��ˮ�շѵ���:%.2f\r\n", Envelop_Data.accData.price);

	// printf("���:%.2f\r\n", Envelop_Data.accData.balance);
	// printf("��ˮ����:%.2f\r\n", Envelop_Data.accData.gross);
	// printf("������ˮ��:%.2f\r\n", Envelop_Data.accData.amount);
	// printf("��ˮTDS:%.2f\r\n", Envelop_Data.accData.purityTDS);
	// printf("ԭˮTDS:%.2f\r\n", Envelop_Data.accData.rawTDS);

	// printf("�����ֽ�1:%.2f\r\n", Envelop_Data.accData.ready1);
	// printf("�����ֽ�2:%.2f\r\n", Envelop_Data.accData.ready2);

	// printf("�Ʒ�ģʽ:%x\r\n", Envelop_Data.accData.chargingMode);
	// printf("�豸ϵ��:%c\r\n", Envelop_Data.accData.Family);
	// printf("�豸״̬��%x\r\n", Envelop_Data.accData.eState);
	return 0;
}

/*
 int main(void)
 {
 char buf1[4] = { 0x01, 0x02, 0x03, 0x04 };

 char buf2[10];


 //sprintf_s();
 //Data_Packet(Buf);   //���÷籩����
 //for (int i = 0;i<sizeof(Buf);i++)
 //{
 //	printf("0X%x,", Buf[i]);
 //}


 Data_Analysis(Temp_Buf);  //���ý������





 return 0;
 }
 */

char Data_Analyze(unsigned char* in_buf, unsigned char* out_buf) {
	int length = 0; // �������ݳ���
	unsigned char temp_key = 0; // ��ʱУ����

	for (int i = 0; i < 44; i++) // �������
	{
		out_buf[i] = 0;
	}

	in_buf[2] = 0x09;

	if (in_buf[3] == 3 || in_buf[3] == 4 || in_buf[3] == 5 || in_buf[3] == 9 ||
		in_buf[3] == 10 || in_buf[3] == 11 || in_buf[3] == 12)
		// �ػ�ָ��  //����ָ��  ǿ��ָ��
	{

		for (int i = 0; i < 8; i++) {
			out_buf[i] = in_buf[i]; // ��������
			temp_key ^= out_buf[i]; // ����key
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

	// ��ֵ  �Ʒ��޸� �Ʒ�ģʽ
	if (in_buf[3] == 6 || in_buf[3] == 7 || in_buf[3] == 8 || in_buf[3] == 13 ||
		in_buf[3] == 14 || in_buf[3] == 15) {
		for (int i = 0; i < 43; i++) {
			out_buf[i] = in_buf[i]; // ��������
			temp_key ^= out_buf[i]; // ����key
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
