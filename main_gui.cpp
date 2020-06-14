// ---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include <Windows.h>
#include <WinBase.h >
#include <winsock.h>
#include <Winsock2.h>
#include "SimpleThreads.h"
#include <process.h>
#include <iostream>
#include <string>
#include <sstream>
#include <queue>
#include <vector>
#include <algorithm>
#include <map>
#include <math.h>
#include <System.Math.hpp>
#include "main_gui.h"
#include "package.h"
#include "CJsonObject.hpp"
// ---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm2 *Form2;

struct netio_buffer {
#define FROMMNG 0
#define FROMDEV 1
	int direction;
	unsigned real_len;
	SOCKET tcp_channel;
#define NETIO_BUF_LEN 1024
	unsigned char buf[NETIO_BUF_LEN];
};

struct id_flow {
	AnsiString id;
	int pre_count, count;
};

thread_selecting2 ts2;
thread_common tc1;
std::queue<netio_buffer*>netio_queue;
SPIN_LOCK netio_queue_lock;
std::map<AnsiString, SOCKET>id_sock_assoc;
// std::map<SOCKET, AnsiString>sock_id_assoc;
std::map<SOCKET, id_flow*>sock_id_assoc1;
SPIN_LOCK lock_clean_dead_sock;

unsigned char Buf[44]; // 发送的包  封包缓冲区
// if 1,return heart_buf
const unsigned char Heart_Buf[5] = {0xaa, 0x44, 0x05, 0x01, 0xea}; // 心跳包返回数据
// if 0, return data_buf
// if >1, return anything
// if -1, return nothing
const unsigned char Data_Buf[5] = {0xaa, 0x44, 0x05, 0x02, 0xe9}; // 数据包返回报

extern TransferUnion Envelop_Data; // 封包联合体
extern TransferUnion Unbind_Data; // 解包联合体

const UnicodeString colon(":");
UnicodeString http_port("");

// ---------------------------------------------------------------------------
unsigned __stdcall data_handling(void *param) {
	bool tc_exited;
	int i;
	int k;
	unsigned j;
	// comm_cmd *c_cmd;
	BUF44 buf44;
	SOCKET s;
	thread_common *tc = (thread_common*)param;
	std::map<SOCKET, id_flow*>::iterator it_stoid;
	netio_buffer *netio_buffer1;
	id_flow *one_id;
	TJSONCollectionBuilder::TPairs *pairs;
	AnsiString temp_str1;
	AnsiString temp_str3;

	UnicodeString temp_str2;
	float f_v;
	int temp_i;

	UnicodeString debug_msg;
	int debug_i;

	int send_err_code;

	UnicodeString json_from_webmng;
	std::map<AnsiString, SOCKET>::iterator it;
	std::map<SOCKET, AnsiString>::iterator it_stoa;
	while (!(tc_is_exited(&tc_exited, tc), tc_exited)) {
		while (!netio_queue.empty()) {

			netio_buffer1 = netio_queue.front();
			// c_cmd = new comm_cmd;
			// SecureZeroMemory(c_cmd, sizeof *c_cmd);
			if (FROMMNG == netio_buffer1->direction) {
				// UnicodeString dbg_msgddd("");
				// Form2->Memo1->Lines->Add(IntToStr((int)netio_buffer1->real_len));
				// for (int d1 = 0; d1 < netio_buffer1->real_len; d1++) {
				// dbg_msgddd.operator += (IntToHex(netio_buffer1->buf[d1], 2));
				// dbg_msgddd += " ";
				// }
				// Form2->Memo1->Lines->Add(dbg_msgddd);
				// // Form2->Memo1->Lines->Add("数据处理激活By HTTP SERVER：" + DateTimeToStr(Now()) + "\r\n");
				// TStringReader *string_reader;
				// TJsonTextReader *text_reader;
				// /*
				// string_reader = new TStringReader;
				// string_reader->ReadBlock(netio_buffer1->buf, 0,
				// netio_buffer1->real_len);
				// */
				// // string_reader = (TStringReader*)TJsonTextReader::NewInstance();
				// json_from_webmng.SetLength(0);
				// // json_from_webmng.operator=(&(netio_buffer1->buf[0]));
				// // json_from_webmng.operator+=(netio_buffer1->buf);
				// for (int p = 0; p < netio_buffer1->real_len; p++) {
				// json_from_webmng.operator += ((char)((netio_buffer1->buf[p])));
				// }
				// string_reader = new TStringReader(json_from_webmng);
				// text_reader = new TJsonTextReader(string_reader);
				// TcpData *tcp_data_buf1 = new TcpData;
				// SecureZeroMemory(tcp_data_buf1, sizeof *tcp_data_buf1);
				// // unsigned char *out_buffer = new unsigned char[44];
				// unsigned char out_buffer[44];
				// SecureZeroMemory(out_buffer, 44);
				// while (text_reader->Read()) {
				// if (TJsonToken::PropertyName == text_reader->TokenType) {
				// if ("package_heder" == text_reader->Value.AsString()) {
				// text_reader->Read();
				// // temp_str2 = text_reader->Value.ToString();
				// // AnsiString(temp_str2).c_str();
				// tcp_data_buf1->head[0] = 0xaa;
				// tcp_data_buf1->head[1] = 0x44;
				//
				// }
				// /*
				// else if ("package_type" ==
				// text_reader->Value.AsString()) {
				// text_reader->Read();
				// }
				// */
				// else if ("data_length" == text_reader->Value.AsString()) {
				// text_reader->Read();
				// tcp_data_buf1->length = StrToInt(text_reader->Value.ToString());
				// }
				// else if ("ctrl_cmd" == text_reader->Value.AsString()) {
				// text_reader->Read();
				// tcp_data_buf1->cTrl = (unsigned char)(text_reader->Value.AsInteger());
				//
				// }
				// else if ("id" == text_reader->Value.AsString()) {
				// text_reader->Read();
				// temp_str2.SetLength(0);
				//
				// temp_str2 = text_reader->Value.ToString();
				// tcp_data_buf1->ID[0] =
				// (unsigned char)(StrToInt("0x" + temp_str2.SubString(1, 2)));
				// tcp_data_buf1->ID[1] =
				// (unsigned char)(StrToInt("0x" + temp_str2.SubString(3, 2)));
				// tcp_data_buf1->ID[2] =
				// (unsigned char)(StrToInt("0x" + temp_str2.SubString(5, 2)));
				// tcp_data_buf1->ID[3] =
				// (unsigned char)(StrToInt("0x" + temp_str2.SubString(7, 2)));
				// }
				// else if ("price" == text_reader->Value.AsString()) {
				// text_reader->Read();
				// tcp_data_buf1->price =
				// Math::RoundTo(StrToFloat(text_reader->Value.ToString()), -4);
				//
				// }
				// else if ("balance" == text_reader->Value.AsString()) {
				// text_reader->Read();
				// tcp_data_buf1->balance =
				// Math::RoundTo(StrToFloat(text_reader->Value.ToString()), -4);
				// }
				// else if ("gross" == text_reader->Value.AsString()) {
				// text_reader->Read();
				// tcp_data_buf1->gross =
				// Math::RoundTo(StrToFloat(text_reader->Value.ToString()), -4);
				// }
				// else if ("amount" == text_reader->Value.AsString()) {
				// text_reader->Read();
				// tcp_data_buf1->amount =
				// Math::RoundTo(StrToFloat(text_reader->Value.ToString()), -4);
				// }
				// else if ("purityTDS" == text_reader->Value.AsString()) {
				// text_reader->Read();
				// tcp_data_buf1->purityTDS =
				// Math::RoundTo(StrToFloat(text_reader->Value.ToString()), -4);
				// }
				// else if ("rawTDS" == text_reader->Value.AsString()) {
				// text_reader->Read();
				// tcp_data_buf1->rawTDS =
				// Math::RoundTo(StrToFloat(text_reader->Value.ToString()), -4);
				// }
				// else if ("ready1" == text_reader->Value.AsString()) {
				// text_reader->Read();
				// tcp_data_buf1->ready1 =
				// Math::RoundTo(StrToFloat(text_reader->Value.ToString()), -4);
				// }
				// else if ("ready2" == text_reader->Value.AsString()) {
				// text_reader->Read();
				// tcp_data_buf1->ready2 =
				// Math::RoundTo(StrToFloat(text_reader->Value.ToString()), -4);
				// }
				// else if ("chargingMode" == text_reader->Value.AsString()) {
				// text_reader->Read();
				// tcp_data_buf1->chargingMode =
				// (unsigned char)(StrToInt(text_reader->Value.ToString()));
				// }
				// else if ("Family" == text_reader->Value.AsString()) {
				// text_reader->Read();
				// tcp_data_buf1->chargingMode =
				// (unsigned char)(StrToInt(text_reader->Value.ToString()));
				// }
				// else if ("eState" == text_reader->Value.AsString()) {
				// text_reader->Read();
				// tcp_data_buf1->eState =
				// (unsigned char)(StrToInt(text_reader->Value.ToString()));
				// }
				//
				// }
				// }
				// int pppp = Data_Analyze((unsigned char*)tcp_data_buf1, out_buffer);
				// it = id_sock_assoc.find(AnsiString(temp_str2));
				// if (id_sock_assoc.end() != it) {
				//
				// Form2->Memo1->Lines->Add(DateTimeToStr(Now()) + " \r\n发送到设备(ID)：" + temp_str2);
				// debug_msg.SetLength(0);
				// for (debug_i = 0; debug_i < pppp; debug_i++) {
				// debug_msg += IntToHex(out_buffer[debug_i], 2);
				// debug_msg += " ";
				// }
				// Form2->Memo1->Lines->Add(debug_msg);
				// // SO_SNDBUF
				// /*
				// int k8;
				// int k9 = sizeof k8;
				// getsockopt(it->second, SOL_SOCKET, SO_SNDBUF,
				// (char *)&k8, &k9);
				// Form2->Memo1->Lines->Add("发送缓冲区长度：" + IntToStr(k8));
				// */
				// send_err_code = send_tot((SOCKET)(it->second), (char *)out_buffer, pppp);
				// if (send_err_code > 0) {
				// Form2->Memo1->Lines->Add("发送错误，代码：" + IntToStr(send_err_code));
				// }
				//
				// /*
				// unsigned threadID;
				// data_to_send *dts1 = new data_to_send;
				// SecureZeroMemory(dts1, sizeof *dts1);
				// dts1->len = pppp;
				// dts1->s = it->second;
				// MoveMemory(dts1->buf, out_buffer, pppp);
				// CloseHandle((HANDLE)_beginthreadex(0, 0, &exc_sending, dts1,
				// 0, &threadID));
				// */
				// /*
				// Form2->Memo1->Lines->Add
				// (DateTimeToStr(Now()) + " \r\n发送完毕(ID)：" + temp_str2 +
				// "\r\n");
				// */
				// }
				// else {
				// Form2->Memo1->Lines->Add("can't find the device by id");
				// }
				// // delete[]out_buffer;
				// delete tcp_data_buf1;
				json_from_webmng.SetLength(0);
				for (int p = 0; p < netio_buffer1->real_len; p++) {
					json_from_webmng.operator +=
						((char)((netio_buffer1->buf[p])));
				}
				unsigned char out_buffer[44];
				SecureZeroMemory(out_buffer, 44);
				TcpData *tcp_data_buf1 = new TcpData;
				SecureZeroMemory(tcp_data_buf1, sizeof *tcp_data_buf1);
				std::string std_json_str =
					std::string(AnsiString(json_from_webmng).c_str());
				neb::CJsonObject jo(std_json_str);
				std::string tmp_str_value;
				UnicodeString tmp_cbstr_value;
				int tmp_i_value;
				float tmp_f_value;
				jo.Get("package_heder", tmp_str_value);
				tmp_cbstr_value = UnicodeString(tmp_str_value.c_str());
				tcp_data_buf1->head[0] =
					(unsigned char)
					(StrToInt("0x" + tmp_cbstr_value.SubString(1, 2)));
				tcp_data_buf1->head[1] =
					(unsigned char)
					(StrToInt("0x" + tmp_cbstr_value.SubString(3, 2)));
				jo.Get("data_length", tmp_i_value);
				tcp_data_buf1->length = tmp_i_value;
				jo.Get("ctrl_cmd", tmp_i_value);
				tcp_data_buf1->cTrl = tmp_i_value;
				jo.Get("id", tmp_str_value);
				tmp_cbstr_value = UnicodeString(tmp_str_value.c_str());
				UnicodeString dev_id = tmp_cbstr_value;
				tcp_data_buf1->ID[0] =
					(unsigned char)
					(StrToInt("0x" + tmp_cbstr_value.SubString(1, 2)));
				tcp_data_buf1->ID[1] =
					(unsigned char)
					(StrToInt("0x" + tmp_cbstr_value.SubString(3, 2)));
				tcp_data_buf1->ID[2] =
					(unsigned char)
					(StrToInt("0x" + tmp_cbstr_value.SubString(5, 2)));
				tcp_data_buf1->ID[3] =
					(unsigned char)
					(StrToInt("0x" + tmp_cbstr_value.SubString(7, 2)));
				jo.Get("price", tmp_f_value);
				tcp_data_buf1->price = Math::RoundTo(tmp_f_value, -4);
				jo.Get("balance", tmp_f_value);
				tcp_data_buf1->balance = Math::RoundTo(tmp_f_value, -4);
				jo.Get("gross", tmp_f_value);
				tcp_data_buf1->gross = Math::RoundTo(tmp_f_value, -4);
				jo.Get("amount", tmp_f_value);
				tcp_data_buf1->amount = Math::RoundTo(tmp_f_value, -4);
				jo.Get("purityTDS", tmp_f_value);
				tcp_data_buf1->purityTDS = Math::RoundTo(tmp_f_value, -4);
				jo.Get("rawTDS", tmp_f_value);
				tcp_data_buf1->rawTDS = Math::RoundTo(tmp_f_value, -4);
				jo.Get("ready1", tmp_f_value);
				tcp_data_buf1->ready1 = Math::RoundTo(tmp_f_value, -4);
				jo.Get("ready2", tmp_f_value);
				tcp_data_buf1->ready2 = Math::RoundTo(tmp_f_value, -4);
				jo.Get("chargingMode", tmp_i_value);
				tcp_data_buf1->chargingMode = tmp_i_value;
				jo.Get("Family", tmp_i_value);
				tcp_data_buf1->Family = tmp_i_value;
				jo.Get("eState", tmp_i_value);
				tcp_data_buf1->eState = tmp_i_value;

				tmp_cbstr_value.SetLength(0);
				int kk1 = sizeof*tcp_data_buf1;
				unsigned char *kk3 = (unsigned char *)tcp_data_buf1;
				for (int kk2 = 0; kk2 < kk1; kk2++) {
					tmp_cbstr_value += IntToHex((unsigned char)kk3[kk2], 2);
					tmp_cbstr_value += " ";
				}
				Form2->Memo1->Lines->Add(tmp_cbstr_value);

				int pppp = Data_Analyze((unsigned char*)tcp_data_buf1,
					out_buffer);
				Form2->Memo1->Lines->Add(pppp);
				tmp_cbstr_value.SetLength(0);
				for (int j = 0; j < pppp; j++) {
					tmp_cbstr_value +=
						IntToHex((unsigned char)out_buffer[j], 2);
					tmp_cbstr_value += " ";
				}
				Form2->Memo1->Lines->Add(IntToStr((int)pppp));
				Form2->Memo1->Lines->Add(tmp_cbstr_value);

				lock(&lock_clean_dead_sock);
				it = id_sock_assoc.find(AnsiString(dev_id));
				if (id_sock_assoc.end() != it) {
					Form2->Memo1->Lines->Add
						(DateTimeToStr(Now()) + " \r\n发送到设备(ID)：" + temp_str2);
					Form2->Memo1->Lines->Add(tmp_cbstr_value);
					send_err_code =
						send_tot((SOCKET)(it->second),
						(char *)out_buffer, pppp);
					if (send_err_code > 0) {
						Form2->Memo1->Lines->Add
							("发送错误，代码：" + IntToStr(send_err_code));
					}
				}
				else {
					Form2->Memo1->Lines->Add("找不到设备：" + dev_id);
				}
				unlock(&lock_clean_dead_sock);

			}
			else if (FROMDEV == netio_buffer1->direction) {
				// c_cmd = (comm_cmd*)&(netio_buffer1->buf[0]);
				// Form2->Memo1->Lines->Add("数据处理激活By TCP SERVER：" + DateTimeToStr(Now()) + "\r\n");
				// temp_str.SetLength(0);
				temp_str1.operator = ("");
				// MoveMemory(temp_str.c_str(), &(netio_buffer1->buf[4]), 4);
				for (j = 4; j < 8; j++) {
					temp_str1.operator += (IntToHex(netio_buffer1->buf[j], 2));
				}
				temp_str3.operator != ("");
				temp_str3 = IntToStr((int)netio_buffer1->buf[3]);
				/*
				 it = id_sock_assoc.find(temp_str1);
				 if (id_sock_assoc.end() == it) {
				 id_sock_assoc.insert
				 (std::pair<AnsiString, SOCKET>(temp_str1,
				 netio_buffer1->tcp_channel));
				 }
				 it_stoa = sock_id_assoc.find(netio_buffer1->tcp_channel);
				 if (sock_id_assoc.end() == it_stoa) {
				 sock_id_assoc.insert
				 (std::pair<SOCKET, AnsiString>
				 (netio_buffer1->tcp_channel, temp_str1));
				 }
				 */

				// it_stoa = sock_id_assoc.find(netio_buffer1->tcp_channel);
				// if (sock_id_assoc.end() != it_stoa) {
				// if (it_stoa->second != AnsiString(""));
				// else {
				// it_stoa->second = temp_str1;
				// id_sock_assoc.insert
				// (std::pair<AnsiString, SOCKET>(temp_str1,
				// netio_buffer1->tcp_channel));
				// }
				// }
				lock(&lock_clean_dead_sock);
				it_stoid = sock_id_assoc1.find(netio_buffer1->tcp_channel);
				if (sock_id_assoc1.end() != it_stoid) {
					one_id = it_stoid->second;
					if (AnsiString("") != one_id->id);
					else {
						one_id->id.operator += (temp_str1);
						id_sock_assoc.insert
							(std::pair<AnsiString, SOCKET>(temp_str1,
							netio_buffer1->tcp_channel));
					}
				}
				unlock(&lock_clean_dead_sock);

				// MoveMemory(temp_str.c_str(), c_cmd->ID, 4);
				// it = id_sock_assoc.find(temp_str);
				// if (id_sock_assoc.end() != it) {
				// s = it->second;
				SecureZeroMemory(buf44, 44);
				MoveMemory(buf44, &(netio_buffer1->buf[0]), 44);
				k = Data_Analysis(buf44);
				if (k > 1) {
					// TJsonTextWriter *text_writer;
					// TStringWriter *string_writer;
					// TStringBuilder *string_builder;
					// TJSONObjectBuilder * json_obj_builder;
					// string_builder = new TStringBuilder();
					// string_writer = new TStringWriter(string_builder);
					// text_writer = new TJsonTextWriter(string_writer);
					// // Add json fromatting.
					// // text_writer->Formatting = TJsonFormatting::Indented;
					// json_obj_builder = new TJSONObjectBuilder(text_writer);
					// pairs = json_obj_builder->BeginObject();
					// pairs->Add("package_heder", UnicodeString("AA44"));
					// // pairs->Add("package_type", UnicodeString("other"));
					// // pairs->Add("ctrl_cmd", UnicodeString(temp_str3));
					// pairs->Add("ctrl_cmd", StrToInt(temp_str3));
					// pairs->Add("id", UnicodeString(temp_str1));
					// pairs->EndObject();
					//
					// Form2->Memo1->Lines->Add(DateTimeToStr(Now()) + " \r\n提交设备数据(ID)：" + temp_str1);
					//
					// Form2->NetHTTPRequest1->MethodString = "GET";
					// Form2->NetHTTPRequest1->URL = "http://127.0.0.1/water_puri?cmd=";
					// Form2->NetHTTPRequest1->URL += string_builder->ToString();
					// Form2->Memo1->Lines->Add(Form2->NetHTTPRequest1->URL + "\r\n");
					// Form2->NetHTTPRequest1->Execute();
					//
					// delete json_obj_builder;
					// delete text_writer;
					// delete string_writer;
					// delete string_builder;
					Form2->Memo1->Lines->Add
						(DateTimeToStr(Now()) + " \r\n数据包返回(ID)：" + temp_str1);
					debug_msg.SetLength(0);
					for (debug_i = 0; debug_i < 5; debug_i++) {
						debug_msg += IntToHex(Data_Buf[debug_i], 2);
						debug_msg += " ";
					}
					Form2->Memo1->Lines->Add(debug_msg);

					send_err_code = send_tot(netio_buffer1->tcp_channel,
						(char *)&(Data_Buf[0]), 5);
					/*
					 int k8;
					 int k9 = sizeof k8;
					 getsockopt(it->second, SOL_SOCKET, SO_SNDBUF,
					 (char *)&k8, &k9);
					 Form2->Memo1->Lines->Add("发送缓冲区长度：" + IntToStr(k8));
					 */
					if (send_err_code > 0) {
						Form2->Memo1->Lines->Add
							("发送错误，代码：" + IntToStr(send_err_code));
					}

					/*
					 unsigned threadID;
					 data_to_send *dts1 = new data_to_send;
					 SecureZeroMemory(dts1, sizeof *dts1);
					 dts1->len = 6;
					 dts1->s = netio_buffer1->tcp_channel;
					 MoveMemory(dts1->buf, Data_Buf, 6);
					 CloseHandle((HANDLE)_beginthreadex(0, 0, &exc_sending,
					 dts1, 0, &threadID));
					 */
					/*
					 Form2->Memo1->Lines->Add
					 (DateTimeToStr(Now()) + " \r\n数据包返回完毕(ID)：" +
					 temp_str1 + "\r\n");
					 */

					// Envelop_Data.accData.balance;
					/*
					 Form2->Memo1->Lines->Add
					 (FloatToStr(Envelop_Data.accData.balance));
					 */
					TJsonTextWriter *text_writer;
					TStringWriter *string_writer;
					TStringBuilder *string_builder;
					TJSONObjectBuilder * json_obj_builder;

					// json_writer = new TJsonWriter();
					// json_obj_buider = new TJSONObjectBuilder(json_writer);
					string_builder = new TStringBuilder();
					string_writer = new TStringWriter(string_builder);
					text_writer = new TJsonTextWriter(string_writer);
					// Add json fromatting.
					// text_writer->Formatting = TJsonFormatting::Indented;
					json_obj_builder = new TJSONObjectBuilder(text_writer);
					pairs = json_obj_builder->BeginObject();
					pairs->Add("package_heder", UnicodeString("AA44"));
					// pairs->Add("package_type", UnicodeString("data"));
					pairs->Add("id", UnicodeString(temp_str1));
					pairs->Add("data_length",
						(unsigned)(Envelop_Data.accData.length));
					pairs->Add("ctrl_cmd", (unsigned)Envelop_Data.accData.cTrl);
					pairs->Add("price", (float)Envelop_Data.accData.price);
					pairs->Add("balance", (float)Envelop_Data.accData.balance);
					pairs->Add("gross", (float)Envelop_Data.accData.gross);
					pairs->Add("amount", (float)Envelop_Data.accData.amount);
					pairs->Add("purityTDS",
						(float)Envelop_Data.accData.purityTDS);
					pairs->Add("rawTDS", (float)Envelop_Data.accData.rawTDS);
					pairs->Add("ready1", (float)Envelop_Data.accData.ready1);
					pairs->Add("ready2", (float)Envelop_Data.accData.ready2);

					pairs->Add("chargingMode",
						(int)Envelop_Data.accData.chargingMode);
					pairs->Add("Family", (int)Envelop_Data.accData.Family);

					pairs->Add("eState", (int)Envelop_Data.accData.eState);
					pairs->EndObject();

					AnsiString p99 = string_builder->ToString();
					/*
					 send_tot(netio_buffer1->tcp_channel, p99.c_str(),
					 p99.Length());
					 */
					Form2->Memo1->Lines->Add
						(DateTimeToStr(Now()) + " \r\n提交设备数据(ID)：" + temp_str1);
					Form2->NetHTTPRequest1->MethodString = "GET";
					Form2->NetHTTPRequest1->URL = "http://127.0.0.1";
					Form2->NetHTTPRequest1->URL += ":";
					Form2->NetHTTPRequest1->URL += http_port;
					Form2->NetHTTPRequest1->URL += "/water_puri?cmd=";
					Form2->NetHTTPRequest1->URL += string_builder->ToString();
					Form2->Memo1->Lines->Add(Form2->NetHTTPRequest1->URL +
						"\r\n");
					Form2->NetHTTPRequest1->Execute();
					// Form2->Memo1->Lines->Add(p99);

					delete json_obj_builder;
					delete text_writer;
					delete string_writer;
					delete string_builder;
				}
				else {
					if (1 == k) {
						Form2->Memo1->Lines->Add
							(DateTimeToStr(Now()) + " \r\n返回心跳(ID)：" +
							temp_str1);
						debug_msg.SetLength(0);
						for (debug_i = 0; debug_i < 5; debug_i++) {
							debug_msg += IntToHex(Heart_Buf[debug_i], 2);
							debug_msg += " ";
						}
						Form2->Memo1->Lines->Add(debug_msg);

						send_err_code =
							send_tot(netio_buffer1->tcp_channel,
							(char *)&(Heart_Buf[0]), 5);

						/*
						 int k8;
						 int k9 = sizeof k8;
						 getsockopt(it->second, SOL_SOCKET, SO_SNDBUF,
						 (char *)&k8, &k9);
						 Form2->Memo1->Lines->Add("发送缓冲区长度：" + IntToStr(k8));
						 */

						if (send_err_code > 0) {
							Form2->Memo1->Lines->Add
								("发送错误，代码：" + IntToStr(send_err_code));
						}

						/*
						 unsigned threadID;
						 data_to_send *dts1 = new data_to_send;
						 SecureZeroMemory(dts1, sizeof *dts1);
						 dts1->len = 5;
						 dts1->s = netio_buffer1->tcp_channel;
						 MoveMemory(dts1->buf, Heart_Buf, 5);
						 CloseHandle((HANDLE)_beginthreadex(0, 0, &exc_sending,
						 dts1, 0, &threadID));
						 */
						/*
						 Form2->Memo1->Lines->Add
						 (DateTimeToStr(Now()) + " \r\n心跳包返回完毕(ID)：" +
						 temp_str1 + "\r\n");
						 */

						TJsonTextWriter *text_writer;
						TStringWriter *string_writer;
						TStringBuilder *string_builder;
						TJSONObjectBuilder *json_obj_builder;

						// json_writer = new TJsonWriter();
						// json_obj_buider = new TJSONObjectBuilder(json_writer);
						string_builder = new TStringBuilder();
						string_writer = new TStringWriter(string_builder);
						text_writer = new TJsonTextWriter(string_writer);
						// Add json fromatting.
						// text_writer->Formatting = TJsonFormatting::Indented;
						json_obj_builder = new TJSONObjectBuilder(text_writer);
						// Form2->json_obj_builder->Clear();
						pairs = json_obj_builder->BeginObject();
						pairs->Add("package_heder", UnicodeString("AA44"));
						// pairs->Add("package_type", UnicodeString("heart_hit"));
						pairs->Add("ctrl_cmd", StrToInt(temp_str3));
						pairs->Add("id", UnicodeString(temp_str1));
						pairs->EndObject();

						/*
						 AnsiString p99 = string_builder->ToString();
						 send_tot(netio_buffer1->tcp_channel, p99.c_str(),
						 p99.Length());
						 Form2->Memo1->Lines->Add(p99);
						 */

						// pairs = Form2->json_obj_builder->BeginObject();
						// pairs->Add("id",dkslds);
						// pairs->Add();
						/*
						 pairs->Add("flaot_value", 1999.99);
						 pairs->Add("int_value", 2293);
						 pairs->Add("unsigned_char", System::WideChar('c'));
						 UnicodeString x = "abcd998";
						 pairs->Add("string", x);
						 pairs->EndObject();
						 Memo1->Lines->Add(string_builder->ToString());
						 NetHTTPRequest1->MethodString = "GET";
						 NetHTTPRequest1->URL = "http://127.0.0.1/water_puri?cmd=";
						 NetHTTPRequest1->URL += string_builder->ToString();
						 NetHTTPRequest1->Execute();
						 */
						Form2->Memo1->Lines->Add
							(DateTimeToStr(Now()) + " \r\n提交设备心跳(ID)：" +
							temp_str1);
						Form2->NetHTTPRequest1->MethodString = "GET";
						Form2->NetHTTPRequest1->URL = "http://127.0.0.1";
						Form2->NetHTTPRequest1->URL += ":";
						Form2->NetHTTPRequest1->URL += http_port;
						Form2->NetHTTPRequest1->URL += "/water_puri?cmd=";
						Form2->NetHTTPRequest1->URL +=
							string_builder->ToString();
						Form2->Memo1->Lines->Add(Form2->NetHTTPRequest1->URL +
							"\r\n");
						Form2->NetHTTPRequest1->Execute();

						delete json_obj_builder;
						delete text_writer;
						delete string_writer;
						delete string_builder;
					}
					else if (0 == k) {
						Form2->Memo1->Lines->Add
							(DateTimeToStr(Now()) + " \r\n数据包返回(ID)：" +
							temp_str1);
						debug_msg.SetLength(0);
						for (debug_i = 0; debug_i < 6; debug_i++) {
							debug_msg += IntToHex(Data_Buf[debug_i], 2);
							debug_msg += " ";
						}
						Form2->Memo1->Lines->Add(debug_msg);

						send_err_code =
							send_tot(netio_buffer1->tcp_channel,
							(char *)&(Data_Buf[0]), 6);
						/*
						 int k8;
						 int k9 = sizeof k8;
						 getsockopt(it->second, SOL_SOCKET, SO_SNDBUF,
						 (char *)&k8, &k9);
						 Form2->Memo1->Lines->Add("发送缓冲区长度：" + IntToStr(k8));
						 */
						if (send_err_code > 0) {
							Form2->Memo1->Lines->Add
								("发送错误，代码：" + IntToStr(send_err_code));
						}

						/*
						 unsigned threadID;
						 data_to_send *dts1 = new data_to_send;
						 SecureZeroMemory(dts1, sizeof *dts1);
						 dts1->len = 6;
						 dts1->s = netio_buffer1->tcp_channel;
						 MoveMemory(dts1->buf, Data_Buf, 6);
						 CloseHandle((HANDLE)_beginthreadex(0, 0, &exc_sending,
						 dts1, 0, &threadID));
						 */
						/*
						 Form2->Memo1->Lines->Add
						 (DateTimeToStr(Now()) + " \r\n数据包返回完毕(ID)：" +
						 temp_str1 + "\r\n");
						 */

						// Envelop_Data.accData.balance;
						/*
						 Form2->Memo1->Lines->Add
						 (FloatToStr(Envelop_Data.accData.balance));
						 */
						TJsonTextWriter *text_writer;
						TStringWriter *string_writer;
						TStringBuilder *string_builder;
						TJSONObjectBuilder * json_obj_builder;

						// json_writer = new TJsonWriter();
						// json_obj_buider = new TJSONObjectBuilder(json_writer);
						string_builder = new TStringBuilder();
						string_writer = new TStringWriter(string_builder);
						text_writer = new TJsonTextWriter(string_writer);
						// Add json fromatting.
						// text_writer->Formatting = TJsonFormatting::Indented;
						json_obj_builder = new TJSONObjectBuilder(text_writer);
						pairs = json_obj_builder->BeginObject();
						pairs->Add("package_heder", UnicodeString("AA44"));
						// pairs->Add("package_type", UnicodeString("data"));
						pairs->Add("id", UnicodeString(temp_str1));
						pairs->Add("data_length",
							(unsigned)(Envelop_Data.accData.length));
						pairs->Add("ctrl_cmd",
							(unsigned)Envelop_Data.accData.cTrl);
						pairs->Add("price", (float)Envelop_Data.accData.price);
						pairs->Add("balance",
							(float)Envelop_Data.accData.balance);
						pairs->Add("gross", (float)Envelop_Data.accData.gross);
						pairs->Add("amount",
							(float)Envelop_Data.accData.amount);
						pairs->Add("purityTDS",
							(float)Envelop_Data.accData.purityTDS);
						pairs->Add("rawTDS",
							(float)Envelop_Data.accData.rawTDS);
						pairs->Add("ready1",
							(float)Envelop_Data.accData.ready1);
						pairs->Add("ready2",
							(float)Envelop_Data.accData.ready2);

						pairs->Add("chargingMode",
							(int)Envelop_Data.accData.chargingMode);
						pairs->Add("Family", (int)Envelop_Data.accData.Family);

						pairs->Add("eState", (int)Envelop_Data.accData.eState);
						pairs->EndObject();

						AnsiString p99 = string_builder->ToString();
						/*
						 send_tot(netio_buffer1->tcp_channel, p99.c_str(),
						 p99.Length());
						 */
						Form2->Memo1->Lines->Add
							(DateTimeToStr(Now()) + " \r\n提交设备数据(ID)：" +
							temp_str1);
						Form2->NetHTTPRequest1->MethodString = "GET";
						Form2->NetHTTPRequest1->URL = "http://127.0.0.1";
						Form2->NetHTTPRequest1->URL += ":";
						Form2->NetHTTPRequest1->URL += http_port;
						Form2->NetHTTPRequest1->URL += "/water_puri?cmd=";
						Form2->NetHTTPRequest1->URL +=
							string_builder->ToString();
						Form2->Memo1->Lines->Add(Form2->NetHTTPRequest1->URL +
							"\r\n");
						Form2->NetHTTPRequest1->Execute();
						// Form2->Memo1->Lines->Add(p99);

						delete json_obj_builder;
						delete text_writer;
						delete string_writer;
						delete string_builder;
					}
					else if (-1 == k);
				}

				// }else{

				// }
				// MoveMemory(buf44, &(netio_buffer1->buf[0]), 44);
				/*
				 temp_str1.SetLength(0);
				 for (i = 0; i < netio_buffer1->real_len; i++) {
				 // j = (unsigned)netio_buffer1->buf[i];
				 temp_str1.operator += (IntToHex(netio_buffer1->buf[i], 2));
				 temp_str1 += " ";
				 }
				 Form2->Memo1->Lines->Add(temp_str1);
				 */
			}

			/*
			 temp_str.SetLength(0);
			 for (i = 0; i < netio_buffer1->real_len; i++) {
			 j = (unsigned)netio_buffer1->buf[i];
			 temp_str.operator += (IntToHex(netio_buffer1->buf[i], 2));
			 temp_str += " ";
			 }
			 Form2->Memo1->Lines->Add(temp_str);
			 */
			delete netio_buffer1;
			lock(&netio_queue_lock);
			netio_queue.pop();
			unlock(&netio_queue_lock);

			// delete c_cmd;
		}

	}
	return 0;
}

// ---------------------------------------------------------------------------
unsigned __stdcall comm_slcting(void *param) {
	SOCKET i, j, newfd, temp_sock;
	int nbytes, err_code;
	unsigned char *pnt_param_recv;
	unsigned thread_id;
	netio_buffer *one_netio_buffer;
	id_flow *one_id;
	struct sockaddr_storage remoteaddr;
	socklen_t addrlen;
	AnsiString temp_id;
	std::map<AnsiString, SOCKET>::iterator it;
	// std::map<SOCKET, AnsiString>::iterator it_stoa;
	std::map<SOCKET, id_flow*>::iterator it_stoid;

	selecting_param *sp = (selecting_param*)param;
	init_thread_common(&tc1);
	start_thread_common(&tc1, data_handling, &tc1);
	for (; ;) {
		sp->read_fds = sp->master;
		if (SOCKET_ERROR != select(sp->listener, &(sp->read_fds), 0, 0, 0));
		else {
			Form2->Memo1->Lines->Add("select function error: " +
				WSAGetLastError());
			goto EXIT_COMM_SLCTING;
		}
		for (i = 0; i < sp->read_fds.fd_count; i++) {
			j = sp->read_fds.fd_array[i];
			if (FD_ISSET(j, &(sp->read_fds))) {
				if (j == sp->listener) {
					addrlen = sizeof remoteaddr;
					newfd = accept(j, (struct sockaddr*)&remoteaddr, &addrlen);
					if (INVALID_SOCKET != newfd) {
						set_sock_nblocking(newfd);
						set_tcp_nodelay(newfd);

						FD_SET(newfd, &(sp->master));
						// it_stoa = sock_id_assoc.find(newfd);
						// if (sock_id_assoc.end() != it_stoa) {
						// temp_id = it_stoa->second;
						// it = id_sock_assoc.find(temp_id);
						// if (id_sock_assoc.end() != it) {
						// id_sock_assoc.erase(it);
						// }
						// it_stoa->second = AnsiString("");
						// }
						// else {
						// sock_id_assoc.insert
						// (std::pair<SOCKET, AnsiString>(newfd,
						// AnsiString("")));
						// }
						Form2->timer_conn->Enabled = false;
                        unlock(&lock_clean_dead_sock);
						lock(&lock_clean_dead_sock);
						it_stoid = sock_id_assoc1.find(newfd);
						if (sock_id_assoc1.end() != it_stoid) {
							one_id = it_stoid->second;
							it = id_sock_assoc.find(one_id->id);
							if (id_sock_assoc.end() != it) {
								id_sock_assoc.erase(it);
							}
							it_stoid->second->id.SetLength(0);
							it_stoid->second->pre_count =
								it_stoid->second->count = 0;
						}
						else {
							one_id = new id_flow;
							SecureZeroMemory(one_id, sizeof *one_id);
							sock_id_assoc1.insert
								(std::pair<SOCKET, id_flow*>(newfd, one_id));
						}
						unlock(&lock_clean_dead_sock);
                        Form2->timer_conn->Enabled = true;
					}
					else {
						Form2->Memo1->Lines->Add
							("accept function error: " + WSAGetLastError());
					}
				}
				else {

					do {
						one_netio_buffer = new netio_buffer;
						SecureZeroMemory(one_netio_buffer,
							sizeof *one_netio_buffer);
						pnt_param_recv = one_netio_buffer->buf;
						nbytes = recv(j, (char *)pnt_param_recv,
							NETIO_BUF_LEN, 0);
						if (nbytes > 0) {
							one_netio_buffer->real_len = nbytes;
							one_netio_buffer->tcp_channel = j;
							one_netio_buffer->direction = FROMDEV;
							lock(&netio_queue_lock);
							netio_queue.push(one_netio_buffer);
							unlock(&netio_queue_lock);
							active_thread_common(&tc1);
							lock(&lock_clean_dead_sock);
							it_stoid = sock_id_assoc1.find(j);
							if (sock_id_assoc1.end() != it_stoid) {
								it_stoid->second->count += nbytes;
							}
							unlock(&lock_clean_dead_sock);
						}
						else if (nbytes < 0) {
							err_code = WSAGetLastError();
							if (WSAEWOULDBLOCK != err_code) {
								FD_CLR(j, &(sp->master));
								closesocket(j);

								// it_stoa = sock_id_assoc.find(j);
								// if (sock_id_assoc.end() != it_stoa) {
								// temp_id = it_stoa->second;
								// sock_id_assoc.erase(it_stoa);
								// it = id_sock_assoc.find(temp_id);
								// if (id_sock_assoc.end() != it) {
								// id_sock_assoc.erase(it);
								// }
								// }
								lock(&lock_clean_dead_sock);
								it_stoid = sock_id_assoc1.find(j);
								if (sock_id_assoc1.end() != it_stoid) {
									one_id = it_stoid->second;
									if (one_id) {
										temp_id.SetLength(0);
										temp_id.operator += (one_id->id);
										delete one_id;
										sock_id_assoc1.erase(it_stoid);
										it = id_sock_assoc.find(temp_id);
										if (id_sock_assoc.end() != it) {
										id_sock_assoc.erase(it);
										}
									}
								}
								unlock(&lock_clean_dead_sock);

								Form2->Memo1->Lines->Add
									("recv function error: " + err_code);
							}
							delete one_netio_buffer;
						}
						else {
							FD_CLR(j, &(sp->master));
							closesocket(j);

							// it_stoa = sock_id_assoc.find(j);
							// if (sock_id_assoc.end() != it_stoa) {
							// temp_id = it_stoa->second;
							// sock_id_assoc.erase(it_stoa);
							// it = id_sock_assoc.find(temp_id);
							// if (id_sock_assoc.end() != it) {
							// id_sock_assoc.erase(it);
							// }
							// }
							lock(&lock_clean_dead_sock);
							it_stoid = sock_id_assoc1.find(j);
							if (sock_id_assoc1.end() != it_stoid) {
								one_id = it_stoid->second;
								if (one_id) {
									temp_id.SetLength(0);
									temp_id.operator += (one_id->id);
									delete one_id;
									sock_id_assoc1.erase(it_stoid);
									it = id_sock_assoc.find(temp_id);
									if (id_sock_assoc.end() != it) {
										id_sock_assoc.erase(it);
									}
								}
							}
							unlock(&lock_clean_dead_sock);

							delete one_netio_buffer;

							if (j != ((thread_selecting2*)sp->thread)
								->terminator_ending);
							else
								goto EXIT_COMM_SLCTING;
						}
					}
					while (nbytes > 0);
				}
			}
		}
	}
EXIT_COMM_SLCTING:
	end_thread_common(&tc1);
	for (i = 0; i < sp->master.fd_count; i++) {
		closesocket(sp->master.fd_array[i]);
		sp->master.fd_array[i] = INVALID_SOCKET;
	}
	FD_ZERO(&(sp->master));
	FD_ZERO(&(sp->read_fds));
	id_sock_assoc.clear();
	// sock_id_assoc.clear();
	sock_id_assoc1.clear();
	while (!netio_queue.empty()) {
		one_netio_buffer = netio_queue.front();
		delete one_netio_buffer;
		netio_queue.pop();
	}
	return 0;
}

// ---------------------------------------------------------------------------
__fastcall TForm2::TForm2(TComponent* Owner) : TForm(Owner) {
}

// ---------------------------------------------------------------------------
void __fastcall TForm2::FormCreate(TObject *Sender) {
	use_winsock(2, 2);
	IdHTTPServer1->DefaultPort = 52727;
	IdHTTPServer1->Bindings->Add()->IP = "127.0.0.1";
	NetHTTPRequest1->Asynchronous = true;
	Button2->Enabled = false;
}

// ---------------------------------------------------------------------------
void __fastcall TForm2::FormDestroy(TObject *Sender) {
	drop_winsock();
}

// ---------------------------------------------------------------------------
void __fastcall TForm2::Button1Click(TObject *Sender) {
	http_port = Edit1->Text.Trim();
	init_thread_selecting2(&ts2, "6100");
	init_spin_lock(&netio_queue_lock);
	init_spin_lock(&lock_clean_dead_sock);
	begin_thread_selecting2(&ts2, comm_slcting);
	IdHTTPServer1->Active = true;
	timer_conn->Enabled = true;
	Button1->Enabled = false;
	Button2->Enabled = true;
}
// ---------------------------------------------------------------------------

void __fastcall TForm2::Button2Click(TObject *Sender) {
	end_thread_selecting2(&ts2);
	timer_conn->Enabled = false;
	IdHTTPServer1->Active = false;
	Button2->Enabled = false;
	Button1->Enabled = true;
}
// ---------------------------------------------------------------------------

void __fastcall TForm2::FormClose(TObject *Sender, TCloseAction &Action) {
	end_thread_selecting2(&ts2);
	IdHTTPServer1->Active = false;
}

// ---------------------------------------------------------------------------
void __fastcall TForm2::IdHTTPServer1CommandGet(TIdContext *AContext,
	TIdHTTPRequestInfo *ARequestInfo, TIdHTTPResponseInfo *AResponseInfo) {
	netio_buffer *one_net_buf;
	AnsiString cmd_value_str;
	if ("/water_puri" == ARequestInfo->Document) {
		if ("cmd" == ARequestInfo->Params->Names[0]) {
			cmd_value_str =
				AnsiString(ARequestInfo->Params->Values
				[ARequestInfo->Params->Names[0]]);
			one_net_buf = new netio_buffer;
			SecureZeroMemory(one_net_buf, sizeof *one_net_buf);
			one_net_buf->direction = FROMMNG;
			MoveMemory(one_net_buf->buf, cmd_value_str.c_str(),
				cmd_value_str.Length());
			one_net_buf->real_len = cmd_value_str.Length();
			lock(&netio_queue_lock);
			netio_queue.push(one_net_buf);
			unlock(&netio_queue_lock);
			active_thread_common(&tc1);
			AResponseInfo->ResponseNo = 200;
		}
	}
}

// ---------------------------------------------------------------------------
void __fastcall TForm2::Timer1Timer(TObject *Sender) {
	Memo1->Lines->Clear();
}

// ---------------------------------------------------------------------------
void __fastcall TForm2::timer_connTimer(TObject *Sender) {
	AnsiString tmp_id("");
	std::map<SOCKET, id_flow*>::iterator it_stoid;
	std::map<AnsiString, SOCKET>::iterator it;
	lock(&lock_clean_dead_sock);
	it_stoid = sock_id_assoc1.begin();
	for (; sock_id_assoc1.end() != it_stoid; it_stoid++) {
		if (it_stoid->second->count > it_stoid->second->pre_count) {
			it_stoid->second->pre_count = it_stoid->second->count = 0;
		}
		else if (it_stoid->second->count == it_stoid->second->pre_count) {
			closesocket(it_stoid->first);
			tmp_id.SetLength(0);
			tmp_id.operator += (it_stoid->second->id);
			delete it_stoid->second;
			sock_id_assoc1.erase(it_stoid);
			it = id_sock_assoc.find(tmp_id);
			if (id_sock_assoc.end() != it) {
				id_sock_assoc.erase(it);
			}
		}
	}
	unlock(&lock_clean_dead_sock);
}
// ---------------------------------------------------------------------------
