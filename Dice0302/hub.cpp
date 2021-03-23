// ---------------------------------------------------------------------------

#pragma hdrstop

#include "hub.h"
#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include "CJsonObject.hpp"

// ---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma classgroup "Vcl.Controls.TControl"
#pragma link "DAScript"
#pragma link "DBAccess"
#pragma link "MemDS"
#pragma link "MySQLUniProvider"
#pragma link "Uni"
#pragma link "UniProvider"
#pragma link "UniScript"
#pragma link "CRSSHIOHandler"
#pragma link "CRVio"
#pragma link "ScBridge"
#pragma link "ScSSHClient"
#pragma link "SQLiteUniProvider"
#pragma resource "*.dfm"
TDataModule1 *DataModule1;

const String s_doc[e_doc_end] = {L"/raffle"};

const String s_dice[e_dice_end] = {
	L"anchorman_id", L"show_id", L"gift_id", L"gift_floor", L"gift_ceiling", L"award_quota", L"award_type",
	L"raffle_condition", L"utimestamp", L"raffle_id", L"unit_price", L"action"};

const String s_dice_result[e_dice_result_end] = {L"crt_total", L"lucky_dog", L"actor_num", L"bystander_num"};

bool db_ready = true;
int conn_pool_min_size = 0;
int mysql_max_conn = 0;
volatile __int64 global_id = 100001;

CRITICAL_SECTION cs_index_dices;
long index_dices;
dice dices[1024];

#define UNICONN(uniconn, uniq, sqlstr)\
do {\
	(uniconn) = new TUniConnection(0);\
	(uniconn)->ConnectString =\
		L"Provider Name=MySQL;User ID=root;Password=\"wFIk9n0=Jqs2\";Data Source=10.11.178.103;Database=live;Port=3306;Login Prompt=False";\
	(uniconn)->SpecificOptions->Values[L"Charset"] = L"utf8mb4";\
	(uniconn)->SpecificOptions->Values[L"UseUnicode"] = L"True";\
	(uniq) = new TUniQuery(0);\
	(uniconn)->IOHandler = DataModule1->CRSSHIOHandler1;\
	(uniq)->Connection = (uniconn);\
	(uniq)->SQL->Clear();\
	(uniq)->SQL->Add((sqlstr));\
	(uniconn)->Connect();\
}\
while (0)

#define UNUNICONN(uniconn, uniq)\
do {\
	delete (uniq);\
	(uniconn)->Disconnect();\
	delete (uniconn);\
}\
while (0)

#define FIND_DICE_RID(k, r_id)\
do {\
	int j;\
	for (j = 0; j < 1024; ++j) {\
		if (dices[j].c[raffle_id] == (r_id))\
			break;\
	}\
	if (1024 == j) {\
		(k) = -1;\
	}\
	else {\
		(k) = j;\
	}\
}\
while (0)

// #define NEXT_DICE(i) do{long cr = InterlockedIncrement(&index_dices);(i) = cr % 2048;}while(0)

#define NEXT_DICE(i)\
	do {\
		bool x = false;\
		long origin;\
		EnterCriticalSection(&cs_index_dices);\
		origin = (i) = (++index_dices) % 1024;\
		do {\
			if (WAIT_OBJECT_0 != WaitForSingleObject(dices[(i)].recycle, 0)) {\
				(i) = (++index_dices) % 1024;\
			}\
			else {\
				x = true;\
				break;\
			}\
		}\
		while ((i) != origin);\
		if (x) {\
			ResetEvent(dices[(i)].recycle);\
		}\
		else {\
			(i) = -1;\
		}\
		LeaveCriticalSection(&cs_index_dices);\
	}\
	while (0)

#define CONVERT_ERR_RESP(str, i_str, err_resp)\
do{err=TryStrToInt64((str),(i_str));if(err);else{\
err_code=90;err_str=(err_resp);goto ERR_END;}}while(0)

void __fastcall init_dice(dice *);
void __fastcall reset_dice(dice *);
void __fastcall draw(dice *);
int __fastcall phase(dice *);
void __fastcall anchorman_gambling(TIdHTTPResponseInfo *, __int64);
void __fastcall anchorman_polling(TIdHTTPResponseInfo *, __int64);
void __fastcall anchorman_droping(TIdHTTPResponseInfo *, __int64);
void __fastcall anchorman_continue(TIdHTTPResponseInfo *, __int64);
void __fastcall audience_join(TIdHTTPResponseInfo *, __int64, __int64, __int64);
void __fastcall audience_polling(TIdHTTPResponseInfo *, __int64, __int64);
void __fastcall audience_querying(TIdHTTPResponseInfo *, __int64);
void __fastcall audience_verifying(TIdHTTPResponseInfo *, __int64, __int64);

// ---------------------------------------------------------------------------
__fastcall TDataModule1::TDataModule1(TComponent* Owner) : TDataModule(Owner) {
}

// ---------------------------------------------------------------------------
void __fastcall TDataModule1::IdHTTPServer1CommandGet(TIdContext *AContext, TIdHTTPRequestInfo *ARequestInfo,
	TIdHTTPResponseInfo *AResponseInfo)

{
	bool err;
	int err_code;
	__int64 i, j, k, cmd;

	String tmp_str, err_str;
	std::string key, value;
	neb::CJsonObject oJson;

	if (hcPOST != ARequestInfo->CommandType)
		goto ERR_END;
	for (i = doc_raffle; i < e_doc_end; ++i)
		if (s_doc[(int)i] == ARequestInfo->Document)
			break;
	if (e_doc_end != i);
	else
		goto ERR_END;

	AResponseInfo->ContentType = L"text/plain";
	AResponseInfo->CharSet = L"utf-8";
	CONVERT_ERR_RESP(ARequestInfo->Params->Values[L"cmd"], cmd, L"POST参数错误: cmd");
	if (doc_raffle != i)
		goto ERR_END;

	if (1000 == cmd) {
		NEXT_DICE(i);
		if (-1 != i) {
			for (j = anchorman_id; j < utimestamp; ++j) {
				tmp_str = Sysutils::Format("POST参数错误: %s", ARRAYOFCONST((s_dice[(int)j])));
				CONVERT_ERR_RESP(ARequestInfo->Params->Values[s_dice[(int)j]], dices[i].c[j], tmp_str);
			}
			anchorman_gambling(AResponseInfo, i);
		}
		else {
			err_code = 101;
			err_str = L"超过同时存在的抽奖活动数量";
		}
	}
	else if (1001 == cmd) {
		tmp_str = Sysutils::Format("POST参数错误: %s", ARRAYOFCONST((s_dice[raffle_id])));
		CONVERT_ERR_RESP(ARequestInfo->Params->Values[s_dice[raffle_id]], i, tmp_str);
		tmp_str = Sysutils::Format("POST参数错误: %s", ARRAYOFCONST((s_dice[action])));
		CONVERT_ERR_RESP(ARequestInfo->Params->Values[s_dice[action]], j, tmp_str);

		anchorman_polling(AResponseInfo, i);
	}
	else if (1002 == cmd) {
		tmp_str = Sysutils::Format("POST参数错误: %s", ARRAYOFCONST((s_dice[raffle_id])));
		CONVERT_ERR_RESP(ARequestInfo->Params->Values[s_dice[raffle_id]], i, tmp_str);
		tmp_str = Sysutils::Format("POST参数错误: %s", ARRAYOFCONST((s_dice[action])));
		CONVERT_ERR_RESP(ARequestInfo->Params->Values[s_dice[action]], j, tmp_str);

		anchorman_droping(AResponseInfo, i);
	}
	else if (1003 == cmd) {
		tmp_str = Sysutils::Format("POST参数错误: %s", ARRAYOFCONST((s_dice[anchorman_id])));
		CONVERT_ERR_RESP(ARequestInfo->Params->Values[s_dice[anchorman_id]], i, tmp_str);

		anchorman_continue(AResponseInfo, i);
	}
	else if (2000 == cmd) {
		tmp_str = Sysutils::Format("POST参数错误: %s", ARRAYOFCONST((L"audience_id")));
		CONVERT_ERR_RESP(ARequestInfo->Params->Values[L"audience_id"], i, tmp_str);
		tmp_str = Sysutils::Format("POST参数错误: %s", ARRAYOFCONST((L"raffle_id")));
		CONVERT_ERR_RESP(ARequestInfo->Params->Values[L"raffle_id"], j, tmp_str);
		tmp_str = Sysutils::Format("POST参数错误: %s", ARRAYOFCONST((L"gift_amount")));
		CONVERT_ERR_RESP(ARequestInfo->Params->Values[L"gift_amount"], k, tmp_str);

		audience_join(AResponseInfo, j, i, k);
	}
	else if (2001 == cmd) {
		tmp_str = Sysutils::Format("POST参数错误: %s", ARRAYOFCONST((s_dice[raffle_id])));
		CONVERT_ERR_RESP(ARequestInfo->Params->Values[s_dice[raffle_id]], i, tmp_str);
		tmp_str = Sysutils::Format("POST参数错误: %s", ARRAYOFCONST((L"audience_id")));
		CONVERT_ERR_RESP(ARequestInfo->Params->Values[L"audience_id"], j, tmp_str);
		tmp_str = Sysutils::Format("POST参数错误: %s", ARRAYOFCONST((s_dice[action])));
		CONVERT_ERR_RESP(ARequestInfo->Params->Values[s_dice[action]], k, tmp_str);

		audience_polling(AResponseInfo, i, j);
	}
	else if (2002 == cmd) {
		tmp_str = Sysutils::Format("POST参数错误: %s", ARRAYOFCONST((s_dice[anchorman_id])));
		CONVERT_ERR_RESP(ARequestInfo->Params->Values[s_dice[anchorman_id]], i, tmp_str);
		tmp_str = Sysutils::Format("POST参数错误: %s", ARRAYOFCONST((L"audience_id")));
		CONVERT_ERR_RESP(ARequestInfo->Params->Values[L"audience_id"], j, tmp_str);

		audience_verifying(AResponseInfo, i, j);
	}
	else if (2003 == cmd) {
		tmp_str = Sysutils::Format("POST参数错误: %s", ARRAYOFCONST((L"audience_id")));
		CONVERT_ERR_RESP(ARequestInfo->Params->Values[L"audience_id"], i, tmp_str);

		audience_querying(AResponseInfo, i);
	}
ERR_END:
	switch (err_code) {
	case 90:
	case 101:
		key = std::string(AnsiString("http_status_code").c_str());
		oJson.Add(key, AResponseInfo->ResponseNo);
		key = std::string(AnsiString("err_code").c_str());
		oJson.Add(key, err_code);
		key = std::string(AnsiString("err_msg").c_str());
		value = std::string(AnsiString(err_str).c_str());
		oJson.Add(key, value);
		AResponseInfo->ContentText = String(oJson.ToString().c_str());
		break;
	default:
		break;
	}
}

// ---------------------------------------------------------------------------
void __fastcall audience_verifying(TIdHTTPResponseInfo *AResponseInfo, __int64 anchorman_id2, __int64 audi_id2) {
	int j, k, l, m, err_code, crt_total2, lucky_dog2;
	TUniConnection *conn;
	TUniQuery *uniq1;
	String err_str, tmp_str;
	std::string key, value;
	neb::CJsonObject oJson;

	ind_ts its;
	its.i = 0;
	its.ts = 0;

	for (j = 0; j < 1024; ++j) {
		SetEvent(dices[j].doing);
		k = phase(&dices[j]);
		if (1 == k)
			continue;

		if (2 == k) {
			if (dices[j].c[anchorman_id] == anchorman_id2) {

				for (l = 0; l < dices[j].v[actor_num]; ++l) {
					if (dices[j].actors[l].id == audi_id2) {
						if (its.ts < dices[j].c[utimestamp]) {
							its.ts = dices[j].c[utimestamp];
							its.i = j;
						}
					}
				}

			}
		}

		if (3 == k) {
			if (dices[j].c[anchorman_id] == anchorman_id2) {

				for (l = 0; l < dices[j].v[actor_num]; ++l) {
					if (dices[j].actors[l].id == audi_id2) {
						key = std::string(AnsiString("http_status_code").c_str());
						oJson.Add(key, AResponseInfo->ResponseNo);
						key = std::string(AnsiString("err_code").c_str());
						oJson.Add(key, 83);
						key = std::string(AnsiString("err_msg").c_str());
						value = std::string(AnsiString(L"允许参加抽奖").c_str());
						oJson.Add(key, value);
						key = std::string(AnsiString("raffle_id").c_str());
						oJson.Add(key, dices[j].c[raffle_id]);
						key = std::string(AnsiString(s_dice[award_type]).c_str());
						oJson.Add(key, dices[j].c[award_type]);
						key = std::string(AnsiString(s_dice[award_quota]).c_str());
						oJson.Add(key, dices[j].c[award_quota]);
						key = std::string(AnsiString(s_dice[gift_floor]).c_str());
						oJson.Add(key, dices[j].c[gift_floor]);
						key = std::string(AnsiString(s_dice[gift_ceiling]).c_str());
						oJson.Add(key, dices[j].c[gift_ceiling]);
						key = std::string(AnsiString(s_dice_result[actor_num]).c_str());
						oJson.Add(key, dices[j].v[actor_num]);
						key = std::string(AnsiString("gift_name").c_str());
						oJson.Add(key, AnsiString(dices[j].gift_name).c_str());
						key = std::string(AnsiString("gifticon").c_str());
						oJson.Add(key, AnsiString(dices[j].gifticon).c_str());
						key = std::string(AnsiString(s_dice_result[lucky_dog]).c_str());
						oJson.Add(key, dices[j].v[lucky_dog]);
						key = std::string(AnsiString(s_dice_result[crt_total]).c_str());
						oJson.Add(key, dices[j].v[crt_total]);
						key = std::string(AnsiString(L"gift_amount").c_str());
						oJson.Add(key, dices[j].actors[l].gift_amount);
						AResponseInfo->ContentText = String(oJson.ToString().c_str());
						return;

					}
				}

				if (l == dices[j].v[actor_num]) {
					if (dices[j].c[raffle_condition]) {
						tmp_str = Sysutils::Format("select uid from cmf_users_attention where touid = %d;",
							ARRAYOFCONST((anchorman_id2)));
						UNICONN(conn, uniq1, tmp_str);

						uniq1->Execute();
						if (uniq1->RecordCount) {
							uniq1->First();
							for (m = 0; m < uniq1->RecordCount; ++m) {
								if (uniq1->FieldByName(L"uid")->AsInteger == audi_id2) {
									key = std::string(AnsiString("http_status_code").c_str());
									oJson.Add(key, AResponseInfo->ResponseNo);
									key = std::string(AnsiString("err_code").c_str());
									oJson.Add(key, 83);
									key = std::string(AnsiString("err_msg").c_str());
									value = std::string(AnsiString(L"允许参加抽奖").c_str());
									oJson.Add(key, value);
									key = std::string(AnsiString("raffle_id").c_str());
									oJson.Add(key, dices[j].c[raffle_id]);
									key = std::string(AnsiString(s_dice[award_type]).c_str());
									oJson.Add(key, dices[j].c[award_type]);
									key = std::string(AnsiString(s_dice[award_quota]).c_str());
									oJson.Add(key, dices[j].c[award_quota]);
									key = std::string(AnsiString(s_dice[gift_ceiling]).c_str());
									oJson.Add(key, dices[j].c[gift_ceiling]);
									key = std::string(AnsiString(s_dice_result[actor_num]).c_str());
									oJson.Add(key, dices[j].v[actor_num]);
									key = std::string(AnsiString("gifticon").c_str());
									oJson.Add(key, AnsiString(dices[j].gifticon).c_str());
									key = std::string(AnsiString(s_dice_result[lucky_dog]).c_str());
									oJson.Add(key, dices[j].v[lucky_dog]);
									key = std::string(AnsiString(s_dice_result[crt_total]).c_str());
									oJson.Add(key, dices[j].v[crt_total]);
									key = std::string(AnsiString(L"gift_name").c_str());
									oJson.Add(key, AnsiString(dices[j].gift_name).c_str());
									key = std::string(AnsiString(L"gift_amount").c_str());
									oJson.Add(key, dices[j].actors[l].gift_amount);
									AResponseInfo->ContentText = String(oJson.ToString().c_str());
									break;
								}
								uniq1->Next();
							}
							if (m != uniq1->RecordCount);
							else {
								key = std::string(AnsiString("http_status_code").c_str());
								oJson.Add(key, AResponseInfo->ResponseNo);
								key = std::string(AnsiString("err_code").c_str());
								oJson.Add(key, 82);
								key = std::string(AnsiString("err_msg").c_str());
								value = std::string(AnsiString(L"不符合参与抽奖条件，不允许参加抽奖").c_str());
								oJson.Add(key, value);
								key = std::string(AnsiString("raffle_id").c_str());
								oJson.Add(key, dices[j].c[raffle_id]);
								key = std::string(AnsiString(s_dice[award_type]).c_str());
								oJson.Add(key, dices[j].c[award_type]);
								key = std::string(AnsiString(s_dice[award_quota]).c_str());
								oJson.Add(key, dices[j].c[award_quota]);
								key = std::string(AnsiString(s_dice[gift_ceiling]).c_str());
								oJson.Add(key, dices[j].c[gift_ceiling]);
								key = std::string(AnsiString(s_dice_result[actor_num]).c_str());
								oJson.Add(key, dices[j].v[actor_num]);
								key = std::string(AnsiString("gifticon").c_str());
								oJson.Add(key, AnsiString(dices[j].gifticon).c_str());
								key = std::string(AnsiString(s_dice_result[lucky_dog]).c_str());
								oJson.Add(key, dices[j].v[lucky_dog]);
								key = std::string(AnsiString(s_dice_result[crt_total]).c_str());
								oJson.Add(key, dices[j].v[crt_total]);
								key = std::string(AnsiString(L"gift_name").c_str());
								oJson.Add(key, AnsiString(dices[j].gift_name).c_str());
								key = std::string(AnsiString(L"gift_amount").c_str());
								oJson.Add(key, dices[j].actors[l].gift_amount);
								AResponseInfo->ContentText = String(oJson.ToString().c_str());
							}
						}
						else {
							key = std::string(AnsiString("http_status_code").c_str());
							oJson.Add(key, AResponseInfo->ResponseNo);
							key = std::string(AnsiString("err_code").c_str());
							oJson.Add(key, 82);
							key = std::string(AnsiString("err_msg").c_str());
							value = std::string(AnsiString(L"不符合参与抽奖条件，不允许参加抽奖").c_str());
							oJson.Add(key, value);
							key = std::string(AnsiString("raffle_id").c_str());
							oJson.Add(key, dices[j].c[raffle_id]);
							key = std::string(AnsiString(s_dice[award_type]).c_str());
							oJson.Add(key, dices[j].c[award_type]);
							key = std::string(AnsiString(s_dice[award_quota]).c_str());
							oJson.Add(key, dices[j].c[award_quota]);
							key = std::string(AnsiString(s_dice[gift_ceiling]).c_str());
							oJson.Add(key, dices[j].c[gift_ceiling]);
							key = std::string(AnsiString(s_dice_result[actor_num]).c_str());
							oJson.Add(key, dices[j].v[actor_num]);
							key = std::string(AnsiString("gifticon").c_str());
							oJson.Add(key, AnsiString(dices[j].gifticon).c_str());
							key = std::string(AnsiString(s_dice_result[lucky_dog]).c_str());
							oJson.Add(key, dices[j].v[lucky_dog]);
							key = std::string(AnsiString(s_dice_result[crt_total]).c_str());
							oJson.Add(key, dices[j].v[crt_total]);
							key = std::string(AnsiString(L"gift_name").c_str());
							oJson.Add(key, AnsiString(dices[j].gift_name).c_str());
							key = std::string(AnsiString(L"gift_amount").c_str());
							oJson.Add(key, dices[j].actors[l].gift_amount);
							AResponseInfo->ContentText = String(oJson.ToString().c_str());
						}

						UNUNICONN(conn, uniq1);
					}
					else {
						key = std::string(AnsiString("http_status_code").c_str());
						oJson.Add(key, AResponseInfo->ResponseNo);
						key = std::string(AnsiString("err_code").c_str());
						oJson.Add(key, 83);
						key = std::string(AnsiString("err_msg").c_str());
						value = std::string(AnsiString(L"允许参加抽奖").c_str());
						oJson.Add(key, value);
						key = std::string(AnsiString("raffle_id").c_str());
						oJson.Add(key, dices[j].c[raffle_id]);
						key = std::string(AnsiString(s_dice[award_type]).c_str());
						oJson.Add(key, dices[j].c[award_type]);
						key = std::string(AnsiString(s_dice[award_quota]).c_str());
						oJson.Add(key, dices[j].c[award_quota]);
						key = std::string(AnsiString(s_dice[gift_floor]).c_str());
						oJson.Add(key, dices[j].c[gift_floor]);
						key = std::string(AnsiString(s_dice[gift_ceiling]).c_str());
						oJson.Add(key, dices[j].c[gift_ceiling]);
						key = std::string(AnsiString(s_dice_result[actor_num]).c_str());
						oJson.Add(key, dices[j].v[actor_num]);
						key = std::string(AnsiString("gift_name").c_str());
						value = std::string(AnsiString(dices[j].gift_name).c_str());
						oJson.Add(key, value);
						key = std::string(AnsiString("gifticon").c_str());
						oJson.Add(key, AnsiString(dices[j].gifticon).c_str());
						key = std::string(AnsiString(s_dice_result[lucky_dog]).c_str());
						oJson.Add(key, dices[j].v[lucky_dog]);
						key = std::string(AnsiString(s_dice_result[crt_total]).c_str());
						oJson.Add(key, dices[j].v[crt_total]);
						key = std::string(AnsiString(L"gift_amount").c_str());
						oJson.Add(key, dices[j].actors[l].gift_amount);
						AResponseInfo->ContentText = String(oJson.ToString().c_str());
					}
				}
				dices[j].bystanders[dices[j].v[bystander_num]++] = audi_id2;
				return;
			}
		}
	}

	if (1024 == j) {

		if (its.ts) {
			key = std::string(AnsiString("http_status_code").c_str());
			oJson.Add(key, AResponseInfo->ResponseNo);
			key = std::string(AnsiString("err_code").c_str());
			oJson.Add(key, 78);
			key = std::string(AnsiString("err_msg").c_str());
			value = std::string(AnsiString(L"主播完成或取消抽奖活动").c_str());
			oJson.Add(key, value);
			key = std::string(AnsiString("raffle_id").c_str());
			oJson.Add(key, dices[its.i].c[raffle_id]);
			key = std::string(AnsiString(s_dice[award_type]).c_str());
			oJson.Add(key, dices[its.i].c[award_type]);
			key = std::string(AnsiString(s_dice[award_quota]).c_str());
			oJson.Add(key, dices[its.i].c[award_quota]);
			key = std::string(AnsiString(s_dice[gift_floor]).c_str());
			oJson.Add(key, dices[its.i].c[gift_floor]);
			key = std::string(AnsiString(s_dice[gift_ceiling]).c_str());
			oJson.Add(key, dices[its.i].c[gift_ceiling]);
			key = std::string(AnsiString(s_dice_result[actor_num]).c_str());
			oJson.Add(key, dices[its.i].v[actor_num]);
			key = std::string(AnsiString("gifticon").c_str());
			oJson.Add(key, AnsiString(dices[its.i].gifticon).c_str());
			key = std::string(AnsiString(s_dice_result[lucky_dog]).c_str());
			oJson.Add(key, dices[its.i].v[lucky_dog]);
			key = std::string(AnsiString(s_dice_result[crt_total]).c_str());
			oJson.Add(key, dices[its.i].v[crt_total]);
			AResponseInfo->ContentText = String(oJson.ToString().c_str());
		}
		else {
			key = std::string(AnsiString("http_status_code").c_str());
			oJson.Add(key, AResponseInfo->ResponseNo);
			key = std::string(AnsiString("err_code").c_str());
			oJson.Add(key, 77);
			key = std::string(AnsiString("err_msg").c_str());
			value = std::string(AnsiString(L"没有启动抽奖活动").c_str());
			oJson.Add(key, value);
			key = std::string(AnsiString("raffle_id").c_str());
			oJson.Add(key, dices[j - 1].c[raffle_id]);
			key = std::string(AnsiString(s_dice[award_type]).c_str());
			oJson.Add(key, dices[j - 1].c[award_type]);
			key = std::string(AnsiString(s_dice[award_quota]).c_str());
			oJson.Add(key, dices[j - 1].c[award_quota]);
			key = std::string(AnsiString(s_dice[gift_floor]).c_str());
			oJson.Add(key, dices[j - 1].c[gift_floor]);
			key = std::string(AnsiString(s_dice[gift_ceiling]).c_str());
			oJson.Add(key, dices[j - 1].c[gift_ceiling]);
			key = std::string(AnsiString(s_dice_result[actor_num]).c_str());
			oJson.Add(key, dices[j - 1].v[actor_num]);
			key = std::string(AnsiString("gifticon").c_str());
			oJson.Add(key, AnsiString(dices[j - 1].gifticon).c_str());
			key = std::string(AnsiString(s_dice_result[lucky_dog]).c_str());
			oJson.Add(key, dices[j - 1].v[lucky_dog]);
			key = std::string(AnsiString(s_dice_result[crt_total]).c_str());
			oJson.Add(key, dices[j - 1].v[crt_total]);
			AResponseInfo->ContentText = String(oJson.ToString().c_str());
		}

	}
}
// ---------------------------------------------------------------------------

void __fastcall audience_querying(TIdHTTPResponseInfo *AResponseInfo, __int64 audi_id2) {
	int k, i_raffle_status, i_gift_id, i_gift_amount, i_raffle_timestamp, i_award_type, i_award_quota, i_is_lucky_dog;
	String s_giftname, s_gifticon;
	TUniConnection *conn;
	TUniQuery *uniq1;
	String err_str, tmp_str;
	std::string key, value;
	neb::CJsonObject oJson;
	neb::CJsonObject ary_elem;

	tmp_str = Sysutils::Format
		("select raffle_status, gift_id, giftname, gift_amount, gifticon, is_lucky_dog, raffle_timestamp, award_type, award_quota from cmf_win_raffle_personal where audience_id = %d;",
		ARRAYOFCONST((audi_id2)));

	UNICONN(conn, uniq1, tmp_str);

	uniq1->Execute();

	if (uniq1->RecordCount) {
		key = std::string(AnsiString("http_status_code").c_str());
		oJson.Add(key, AResponseInfo->ResponseNo);
		key = std::string(AnsiString("err_code").c_str());
		oJson.Add(key, 0);
		key = std::string(AnsiString("err_msg").c_str());
		value = std::string(AnsiString(L"有历史抽奖记录").c_str());
		oJson.AddEmptySubArray("records");
		uniq1->First();
		for (k = 0; k < uniq1->RecordCount; ++k) {
			i_raffle_status = uniq1->FieldByName(L"raffle_status")->AsInteger;
			i_gift_id = uniq1->FieldByName(L"gift_id")->AsInteger;
			i_gift_amount = uniq1->FieldByName(L"gift_amount")->AsInteger;
			i_raffle_timestamp = uniq1->FieldByName(L"raffle_timestamp")->AsInteger;
			i_award_type = uniq1->FieldByName(L"award_type")->AsInteger;
			i_award_quota = uniq1->FieldByName(L"award_quota")->AsInteger;
			i_is_lucky_dog = uniq1->FieldByName(L"is_lucky_dog")->AsInteger;
			s_giftname = uniq1->FieldByName(L"giftname")->AsString;
			s_gifticon = uniq1->FieldByName(L"gifticon")->AsString;

			ary_elem.Clear();
			ary_elem.Add("raffle_status", i_raffle_status);
			ary_elem.Add("gift_id", i_gift_id);
			ary_elem.Add("giftname", std::string(AnsiString(s_giftname).c_str()));
			ary_elem.Add("gifticon", std::string(AnsiString(s_gifticon).c_str()));
			ary_elem.Add("gift_amount", i_gift_amount);
			ary_elem.Add("is_lucky_dog", i_is_lucky_dog);
			ary_elem.Add("raffle_timestamp", i_raffle_timestamp);
			ary_elem.Add("award_type", i_award_type);
			ary_elem.Add("award_quota", i_award_quota);
			oJson.operator[]("records").Add(ary_elem);
			uniq1->Next();
		}
		AResponseInfo->ContentText = String(oJson.ToString().c_str());
	}
	else {
		key = std::string(AnsiString("http_status_code").c_str());
		oJson.Add(key, AResponseInfo->ResponseNo);
		key = std::string(AnsiString("err_code").c_str());
		oJson.Add(key, 71);
		key = std::string(AnsiString("err_msg").c_str());
		value = std::string(AnsiString(L"无历史抽奖记录").c_str());
		oJson.Add(key, value);
		AResponseInfo->ContentText = String(oJson.ToString().c_str());
	}

	UNUNICONN(conn, uniq1);
}

// ---------------------------------------------------------------------------
void __fastcall audience_polling(TIdHTTPResponseInfo *AResponseInfo, __int64 raffle_id2, __int64 audi_id2) {
	int j, k, l;
	TUniConnection *conn;
	TUniQuery *uniq1;
	String err_str, tmp_str;
	std::string key, value;
	neb::CJsonObject oJson;

	ind_ts its;
	its.i = 0;
	its.ts = 0;

	for (j = 0; j < 1024; ++j) {
		SetEvent(dices[j].doing);
		k = phase(&dices[j]);
		if (1 == k)
			continue;

		if (2 == k) {
			if (dices[j].c[raffle_id] == raffle_id2) {
				if (its.ts < dices[j].c[utimestamp]) {
					its.ts = dices[j].c[utimestamp];
					its.i = j;
				}
			}
		}

		if (3 == k) {

			if (raffle_id2 == dices[j].c[raffle_id]) {
				key = std::string(AnsiString("http_status_code").c_str());
				oJson.Add(key, AResponseInfo->ResponseNo);
				key = std::string(AnsiString("err_code").c_str());
				oJson.Add(key, pending);
				key = std::string(AnsiString("err_msg").c_str());
				value = std::string(AnsiString(L"抽奖活动进行中").c_str());
				oJson.Add(key, value);
				key = std::string(AnsiString("raffle_id").c_str());
				oJson.Add(key, dices[j].c[raffle_id]);
				key = std::string(AnsiString(s_dice[award_type]).c_str());
				oJson.Add(key, dices[j].c[award_type]);
				key = std::string(AnsiString(s_dice[award_quota]).c_str());
				oJson.Add(key, dices[j].c[award_quota]);
				key = std::string(AnsiString(s_dice[gift_floor]).c_str());
				oJson.Add(key, dices[j].c[gift_floor]);
				key = std::string(AnsiString(s_dice[gift_ceiling]).c_str());
				oJson.Add(key, dices[j].c[gift_ceiling]);
				key = std::string(AnsiString(s_dice_result[actor_num]).c_str());
				oJson.Add(key, dices[j].v[actor_num]);
				key = std::string(AnsiString("gifticon").c_str());
				oJson.Add(key, AnsiString(dices[j].gifticon).c_str());
				key = std::string(AnsiString(s_dice_result[lucky_dog]).c_str());
				oJson.Add(key, dices[j].v[lucky_dog]);
				key = std::string(AnsiString(s_dice_result[crt_total]).c_str());
				oJson.Add(key, dices[j].v[crt_total]);
				AResponseInfo->ContentText = String(oJson.ToString().c_str());
			}

			return;
		}
	}

	if (1024 == j) {
		if (its.ts) {
			for (l = 0; l < dices[its.i].v[actor_num]; ++l) {
				if (dices[its.i].actors[l].id == audi_id2) {
					if (dices[its.i].v[lucky_dog] == -1) {
						tmp_str = Sysutils::Format(L"insert into cmf_win_raffle_personal(audience_id, raffle_id, raffle_status, gift_id, giftname,\
						 gifticon, gift_amount, gems_consumed, is_lucky_dog, award_type, award_quota, raffle_timestamp) \
						 values(%d, %d, -1, %d, '%s', '%s', %d, %d, 0, %d, %d, %d);",
							ARRAYOFCONST((audi_id2, dices[its.i].c[raffle_id], dices[its.i].c[gift_id],
							dices[its.i].gift_name, dices[its.i].gifticon, dices[its.i].actors[l].gift_amount,
							(dices[its.i].c[unit_price] * dices[its.i].actors[l].gift_amount),
							dices[its.i].c[award_type], dices[its.i].c[award_quota], dices[its.i].c[utimestamp])));
						UNICONN(conn, uniq1, tmp_str);

						if (conn->InTransaction);
						else {
							conn->StartTransaction();
							try {
								uniq1->Execute();
								conn->Commit();
							}
							catch (...) {
								conn->Rollback();
							}
						}

						UNUNICONN(conn, uniq1);

						key = std::string(AnsiString("http_status_code").c_str());
						oJson.Add(key, AResponseInfo->ResponseNo);
						key = std::string(AnsiString("err_code").c_str());
						oJson.Add(key, 86);
						key = std::string(AnsiString("err_msg").c_str());
						value = std::string(AnsiString(L"主播取消抽奖").c_str());
						oJson.Add(key, value);
						key = std::string(AnsiString("raffle_id").c_str());
						oJson.Add(key, dices[its.i].c[raffle_id]);
						key = std::string(AnsiString(s_dice[award_type]).c_str());
						oJson.Add(key, dices[its.i].c[award_type]);
						key = std::string(AnsiString(s_dice[award_quota]).c_str());
						oJson.Add(key, dices[its.i].c[award_quota]);
						key = std::string(AnsiString(s_dice[gift_floor]).c_str());
						oJson.Add(key, dices[its.i].c[gift_floor]);
						key = std::string(AnsiString(s_dice[gift_ceiling]).c_str());
						oJson.Add(key, dices[its.i].c[gift_ceiling]);
						key = std::string(AnsiString(s_dice_result[actor_num]).c_str());
						oJson.Add(key, dices[its.i].v[actor_num]);
						key = std::string(AnsiString("gifticon").c_str());
						oJson.Add(key, AnsiString(dices[its.i].gifticon).c_str());
						key = std::string(AnsiString(s_dice_result[lucky_dog]).c_str());
						oJson.Add(key, dices[its.i].v[lucky_dog]);
						key = std::string(AnsiString(s_dice_result[crt_total]).c_str());
						oJson.Add(key, dices[its.i].v[crt_total]);
						AResponseInfo->ContentText = String(oJson.ToString().c_str());
					}
					else if (dices[its.i].v[lucky_dog] > 0) {
						tmp_str = Sysutils::Format(L"insert into cmf_win_raffle_personal(audience_id, raffle_id, raffle_status, gift_id, giftname,\
						 gifticon, gift_amount, gems_consumed, is_lucky_dog, award_type, award_quota, raffle_timestamp)\
						  values(%d, %d, 1, %d, '%s', '%s', %d, %d, %d, %d, %d, %d);",
							ARRAYOFCONST((audi_id2, dices[its.i].c[raffle_id], dices[its.i].c[gift_id],
							dices[its.i].gift_name, dices[its.i].gifticon, dices[its.i].actors[l].gift_amount,
							(dices[its.i].c[unit_price] * dices[its.i].actors[l].gift_amount), 1,
							dices[its.i].c[award_type], dices[its.i].c[award_quota], dices[its.i].c[utimestamp])));
						UNICONN(conn, uniq1, tmp_str);

						if (conn->InTransaction);
						else {
							conn->StartTransaction();
							try {
								uniq1->Execute();
								conn->Commit();
							}
							catch (...) {
								conn->Rollback();
							}
						}

						UNUNICONN(conn, uniq1);

						key = std::string(AnsiString("http_status_code").c_str());
						oJson.Add(key, AResponseInfo->ResponseNo);
						key = std::string(AnsiString("err_code").c_str());
						oJson.Add(key, 85);
						key = std::string(AnsiString("err_msg").c_str());
						value = std::string(AnsiString(L"主播完成抽奖").c_str());
						oJson.Add(key, value);
						key = std::string(AnsiString("raffle_id").c_str());
						oJson.Add(key, dices[its.i].c[raffle_id]);
						key = std::string(AnsiString(s_dice[award_type]).c_str());
						oJson.Add(key, dices[its.i].c[award_type]);
						key = std::string(AnsiString(s_dice[award_quota]).c_str());
						oJson.Add(key, dices[its.i].c[award_quota]);
						key = std::string(AnsiString(s_dice[gift_floor]).c_str());
						oJson.Add(key, dices[its.i].c[gift_floor]);
						key = std::string(AnsiString(s_dice[gift_ceiling]).c_str());
						oJson.Add(key, dices[its.i].c[gift_ceiling]);
						key = std::string(AnsiString(s_dice_result[actor_num]).c_str());
						oJson.Add(key, dices[its.i].v[actor_num]);
						key = std::string(AnsiString("gifticon").c_str());
						oJson.Add(key, AnsiString(dices[its.i].gifticon).c_str());
						key = std::string(AnsiString(s_dice_result[lucky_dog]).c_str());
						oJson.Add(key, dices[its.i].v[lucky_dog]);
						key = std::string(AnsiString(s_dice_result[crt_total]).c_str());
						oJson.Add(key, dices[its.i].v[crt_total]);
						AResponseInfo->ContentText = String(oJson.ToString().c_str());
					}

					dices[its.i].actors[l].gift_amount = 0;
					dices[its.i].actors[l].id = 0;
					--dices[its.i].v[actor_num];
					if (dices[its.i].v[actor_num]);
					else {
						reset_dice(&(dices[its.i]));
					}
				}
			}

			if (l == dices[its.i].v[actor_num]) {
				key = std::string(AnsiString("http_status_code").c_str());
				oJson.Add(key, AResponseInfo->ResponseNo);
				key = std::string(AnsiString("err_code").c_str());
				oJson.Add(key, 84);
				key = std::string(AnsiString("err_msg").c_str());
				value = std::string(AnsiString(L"未找到对应观众ID").c_str());
				oJson.Add(key, value);
				key = std::string(AnsiString("raffle_id").c_str());
				oJson.Add(key, dices[its.i].c[raffle_id]);
				key = std::string(AnsiString(s_dice[award_type]).c_str());
				oJson.Add(key, dices[its.i].c[award_type]);
				key = std::string(AnsiString(s_dice[award_quota]).c_str());
				oJson.Add(key, dices[its.i].c[award_quota]);
				key = std::string(AnsiString(s_dice[gift_floor]).c_str());
				oJson.Add(key, dices[its.i].c[gift_floor]);
				key = std::string(AnsiString(s_dice[gift_ceiling]).c_str());
				oJson.Add(key, dices[its.i].c[gift_ceiling]);
				key = std::string(AnsiString(s_dice_result[actor_num]).c_str());
				oJson.Add(key, dices[its.i].v[actor_num]);
				key = std::string(AnsiString("gifticon").c_str());
				oJson.Add(key, AnsiString(dices[its.i].gifticon).c_str());
				key = std::string(AnsiString(s_dice_result[lucky_dog]).c_str());
				oJson.Add(key, dices[its.i].v[lucky_dog]);
				key = std::string(AnsiString(s_dice_result[crt_total]).c_str());
				oJson.Add(key, dices[its.i].v[crt_total]);
				AResponseInfo->ContentText = String(oJson.ToString().c_str());
			}
		}
		else {
			key = std::string(AnsiString("http_status_code").c_str());
			oJson.Add(key, AResponseInfo->ResponseNo);
			key = std::string(AnsiString("err_code").c_str());
			oJson.Add(key, 83);
			key = std::string(AnsiString("err_msg").c_str());
			value = std::string(AnsiString(L"未找到对应抽奖活动ID或观众ID").c_str());
			oJson.Add(key, value);
			key = std::string(AnsiString("raffle_id").c_str());
			oJson.Add(key, dices[its.i].c[raffle_id]);
			key = std::string(AnsiString(s_dice[award_type]).c_str());
			oJson.Add(key, dices[its.i].c[award_type]);
			key = std::string(AnsiString(s_dice[award_quota]).c_str());
			oJson.Add(key, dices[its.i].c[award_quota]);
			key = std::string(AnsiString(s_dice[gift_floor]).c_str());
			oJson.Add(key, dices[its.i].c[gift_floor]);
			key = std::string(AnsiString(s_dice[gift_ceiling]).c_str());
			oJson.Add(key, dices[its.i].c[gift_ceiling]);
			key = std::string(AnsiString(s_dice_result[actor_num]).c_str());
			oJson.Add(key, dices[its.i].v[actor_num]);
			key = std::string(AnsiString("gifticon").c_str());
			oJson.Add(key, AnsiString(dices[its.i].gifticon).c_str());
			key = std::string(AnsiString(s_dice_result[lucky_dog]).c_str());
			oJson.Add(key, dices[its.i].v[lucky_dog]);
			key = std::string(AnsiString(s_dice_result[crt_total]).c_str());
			oJson.Add(key, dices[its.i].v[crt_total]);
			AResponseInfo->ContentText = String(oJson.ToString().c_str());
		}
	}
}

// ---------------------------------------------------------------------------
void __fastcall audience_join(TIdHTTPResponseInfo *AResponseInfo, __int64 raffle_id2, __int64 audi_id2,
	__int64 gift_amount2) {
	int j, k, l, m;
	TUniConnection *conn;
	TUniQuery *uniq1;
	String tmp_str;
	std::string key, value;
	neb::CJsonObject oJson;

	ind_ts its;
	its.i = 0;
	its.ts = 0;

	for (j = 0; j < 1024; ++j) {
		SetEvent(dices[j].doing);
		k = phase(&dices[j]);
		if (1 == k)
			continue;

		if (2 == k) {
			if (dices[j].c[raffle_id] == raffle_id2) {
				if (its.ts < dices[j].c[utimestamp]) {
					its.ts = dices[j].c[utimestamp];
					its.i = j;
				}
			}
		}

		if (3 == k) {
			if (dices[j].c[raffle_id] == raffle_id2) {
				l = (gift_amount2 * dices[j].c[unit_price]);
				tmp_str = Sysutils::Format("select coin from cmf_users where id = %d;", ARRAYOFCONST((audi_id2)));
				UNICONN(conn, uniq1, tmp_str);

				uniq1->Execute();

				if (uniq1->RecordCount) {
					uniq1->First();
					m = uniq1->FieldByName("coin")->AsInteger;
					if (m < l) {
						key = std::string(AnsiString("http_status_code").c_str());
						oJson.Add(key, AResponseInfo->ResponseNo);
						key = std::string(AnsiString("err_code").c_str());
						oJson.Add(key, 103);
						key = std::string(AnsiString("err_msg").c_str());
						value = std::string(AnsiString(L"钻石数不足").c_str());
						oJson.Add(key, value);
						key = std::string(AnsiString("raffle_id").c_str());
						oJson.Add(key, dices[j].c[raffle_id]);
						key = std::string(AnsiString(s_dice[award_type]).c_str());
						oJson.Add(key, dices[j].c[award_type]);
						key = std::string(AnsiString(s_dice[award_quota]).c_str());
						oJson.Add(key, dices[j].c[award_quota]);
						key = std::string(AnsiString(s_dice[gift_floor]).c_str());
						oJson.Add(key, dices[j].c[gift_floor]);
						key = std::string(AnsiString(s_dice[gift_ceiling]).c_str());
						oJson.Add(key, dices[j].c[gift_ceiling]);
						key = std::string(AnsiString(s_dice_result[actor_num]).c_str());
						oJson.Add(key, dices[j].v[actor_num]);
						key = std::string(AnsiString("gifticon").c_str());
						oJson.Add(key, AnsiString(dices[j].gifticon).c_str());
						key = std::string(AnsiString(s_dice_result[lucky_dog]).c_str());
						oJson.Add(key, dices[j].v[lucky_dog]);
						key = std::string(AnsiString(s_dice_result[crt_total]).c_str());
						oJson.Add(key, dices[j].v[crt_total]);
						AResponseInfo->ContentText = String(oJson.ToString().c_str());
						UNUNICONN(conn, uniq1);
						return;
					}
				}
				else {
					key = std::string(AnsiString("http_status_code").c_str());
					oJson.Add(key, AResponseInfo->ResponseNo);
					key = std::string(AnsiString("err_code").c_str());
					oJson.Add(key, 104);
					key = std::string(AnsiString("err_msg").c_str());
					value = std::string(AnsiString(L"无法通过观众ID获得钻石信息").c_str());
					oJson.Add(key, value);
					key = std::string(AnsiString("raffle_id").c_str());
					oJson.Add(key, dices[j].c[raffle_id]);
					key = std::string(AnsiString(s_dice[award_type]).c_str());
					oJson.Add(key, dices[j].c[award_type]);
					key = std::string(AnsiString(s_dice[award_quota]).c_str());
					oJson.Add(key, dices[j].c[award_quota]);
					key = std::string(AnsiString(s_dice[gift_floor]).c_str());
					oJson.Add(key, dices[j].c[gift_floor]);
					key = std::string(AnsiString(s_dice[gift_ceiling]).c_str());
					oJson.Add(key, dices[j].c[gift_ceiling]);
					key = std::string(AnsiString(s_dice_result[actor_num]).c_str());
					oJson.Add(key, dices[j].v[actor_num]);
					key = std::string(AnsiString("gifticon").c_str());
					oJson.Add(key, AnsiString(dices[j].gifticon).c_str());
					key = std::string(AnsiString(s_dice_result[lucky_dog]).c_str());
					oJson.Add(key, dices[j].v[lucky_dog]);
					key = std::string(AnsiString(s_dice_result[crt_total]).c_str());
					oJson.Add(key, dices[j].v[crt_total]);
					AResponseInfo->ContentText = String(oJson.ToString().c_str());
					UNUNICONN(conn, uniq1);
					return;
				}

				uniq1->SQL->Clear();
				tmp_str = Sysutils::Format("update cmf_users set coin = %d where id = %d;",
					ARRAYOFCONST((m - l, audi_id2)));
				tmp_str +=
					Sysutils::Format
					("insert into cmf_users_coinrecord(type, action, uid, touid, giftid, giftcount, totalcoin, showid, addtime, game_banker, game_action, mark) values('%s', '%s', %d, %d, %d, %d, %d, %d, %d, 0, 0, 0);",
					ARRAYOFCONST((String(L"expend"), String(L"dice"), audi_id2, dices[j].c[anchorman_id],
					dices[j].c[gift_id], gift_amount2, l, dices[j].c[show_id],
					System::Dateutils::DateTimeToUnix(Now()))));
				uniq1->SQL->Add(tmp_str);

				if (conn->InTransaction);
				else {
					conn->StartTransaction();
					try {
						uniq1->Execute();
						conn->Commit();
					}
					catch (...) {
						conn->Rollback();
					}
				}

				UNUNICONN(conn, uniq1);

				for (l = 0; l < dices[j].v[actor_num]; ++l) {
					if (dices[j].actors[l].id == audi_id2) {
						dices[j].actors[l].gift_amount += gift_amount2;
						dices[j].v[crt_total] += gift_amount2;
						break;
					}
				}

				if (dices[j].v[actor_num] == l) {

					for (m = 0; m < dices[j].v[bystander_num]; ++m) {
						if (dices[j].bystanders[m] == audi_id2) {
							dices[j].bystanders[m] = 0;
							if (dices[j].v[bystander_num] > 0) {
								dices[j].bystanders[m] = dices[j].bystanders[dices[j].v[bystander_num] - 1];
								--dices[j].v[bystander_num];
							}
							break;
						}
					}

					dices[j].actors[dices[j].v[actor_num]].id = audi_id2;
					dices[j].actors[dices[j].v[actor_num]].gift_amount = gift_amount2;
					dices[j].v[crt_total] += gift_amount2;
					++dices[j].v[actor_num];
				}

				key = std::string(AnsiString("http_status_code").c_str());
				oJson.Add(key, AResponseInfo->ResponseNo);
				key = std::string(AnsiString("err_code").c_str());
				oJson.Add(key, pending);
				key = std::string(AnsiString("err_msg").c_str());
				value = std::string(AnsiString(L"成功参与抽奖").c_str());
				oJson.Add(key, value);
				key = std::string(AnsiString("raffle_id").c_str());
				oJson.Add(key, dices[j].c[raffle_id]);
				key = std::string(AnsiString(s_dice[award_type]).c_str());
				oJson.Add(key, dices[j].c[award_type]);
				key = std::string(AnsiString(s_dice[award_quota]).c_str());
				oJson.Add(key, dices[j].c[award_quota]);
				key = std::string(AnsiString(s_dice[gift_floor]).c_str());
				oJson.Add(key, dices[j].c[gift_floor]);
				key = std::string(AnsiString(s_dice[gift_ceiling]).c_str());
				oJson.Add(key, dices[j].c[gift_ceiling]);
				key = std::string(AnsiString(s_dice_result[actor_num]).c_str());
				oJson.Add(key, dices[j].v[actor_num]);
				key = std::string(AnsiString("gifticon").c_str());
				oJson.Add(key, AnsiString(dices[j].gifticon).c_str());
				key = std::string(AnsiString(s_dice_result[lucky_dog]).c_str());
				oJson.Add(key, dices[j].v[lucky_dog]);
				key = std::string(AnsiString(s_dice_result[crt_total]).c_str());
				oJson.Add(key, dices[j].v[crt_total]);
				AResponseInfo->ContentText = String(oJson.ToString().c_str());

				return;
			}
		}
	}

	if (1024 == j) {
		if (its.ts) {
			key = std::string(AnsiString("http_status_code").c_str());
			oJson.Add(key, AResponseInfo->ResponseNo);
			key = std::string(AnsiString("err_code").c_str());

			if (-1 == dices[its.i].v[lucky_dog]) {
				oJson.Add(key, 93);
				key = std::string(AnsiString("err_msg").c_str());
				value = std::string(AnsiString(L"主播已经取消抽奖").c_str());
				oJson.Add(key, value);
			}
			else if (dices[its.i].v[lucky_dog] > 0) {
				oJson.Add(key, 94);
				key = std::string(AnsiString("err_msg").c_str());
				value = std::string(AnsiString(L"抽奖活动已完成").c_str());
				oJson.Add(key, value);
			}
			key = std::string(AnsiString(s_dice[raffle_id]).c_str());
			oJson.Add(key, dices[its.i].c[raffle_id]);
			key = std::string(AnsiString(s_dice[utimestamp]).c_str());
			oJson.Add(key, dices[its.i].c[utimestamp]);
			key = std::string(AnsiString(s_dice[gift_ceiling]).c_str());
			oJson.Add(key, dices[its.i].c[gift_ceiling]);
			key = std::string(AnsiString(s_dice_result[utimestamp]).c_str());
			oJson.Add(key, dices[its.i].v[crt_total]);
			key = std::string(AnsiString(s_dice_result[actor_num]).c_str());
			oJson.Add(key, dices[its.i].v[actor_num]);
			key = std::string(AnsiString(s_dice_result[lucky_dog]).c_str());
			oJson.Add(key, dices[its.i].v[lucky_dog]);
			AResponseInfo->ContentText = String(oJson.ToString().c_str());
		}
		else {
			key = std::string(AnsiString("http_status_code").c_str());
			oJson.Add(key, AResponseInfo->ResponseNo);
			key = std::string(AnsiString("err_code").c_str());
			oJson.Add(key, 78);
			key = std::string(AnsiString("err_msg").c_str());
			value = std::string(AnsiString(L"未找到主播相关的抽奖信息").c_str());
			oJson.Add(key, value);
			key = std::string(AnsiString("raffle_id").c_str());
			oJson.Add(key, dices[its.i].c[raffle_id]);
			key = std::string(AnsiString(s_dice[award_type]).c_str());
			oJson.Add(key, dices[its.i].c[award_type]);
			key = std::string(AnsiString(s_dice[award_quota]).c_str());
			oJson.Add(key, dices[its.i].c[award_quota]);
			key = std::string(AnsiString(s_dice[gift_floor]).c_str());
			oJson.Add(key, dices[its.i].c[gift_floor]);
			key = std::string(AnsiString(s_dice[gift_ceiling]).c_str());
			oJson.Add(key, dices[its.i].c[gift_ceiling]);
			key = std::string(AnsiString(s_dice_result[actor_num]).c_str());
			oJson.Add(key, dices[its.i].v[actor_num]);
			key = std::string(AnsiString("gifticon").c_str());
			oJson.Add(key, AnsiString(dices[its.i].gifticon).c_str());
			key = std::string(AnsiString(s_dice_result[lucky_dog]).c_str());
			oJson.Add(key, dices[its.i].v[lucky_dog]);
			key = std::string(AnsiString(s_dice_result[crt_total]).c_str());
			oJson.Add(key, dices[its.i].v[crt_total]);
			AResponseInfo->ContentText = String(oJson.ToString().c_str());
		}
	}
}

// ---------------------------------------------------------------------------
void __fastcall anchorman_continue(TIdHTTPResponseInfo *AResponseInfo, __int64 anchorman_id2) {
	int j, k;
	TUniConnection *conn;
	TUniQuery *uniq1;
	ind_ts its;
	its.i = 0;
	its.ts = 0;
	std::string key, value;
	neb::CJsonObject oJson;

	for (j = 0; j < 1024; ++j) {
		SetEvent(dices[j].doing);
		k = phase(&dices[j]);
		if (1 == k)
			continue;

		if (2 == k) {
			if (dices[j].c[anchorman_id] == anchorman_id2) {
				if (its.ts < dices[j].c[utimestamp]) {
					its.ts = dices[j].c[utimestamp];
					its.i = j;
				}
			}
		}

		if (3 == k) {
			if (dices[j].c[anchorman_id] == anchorman_id2) {
				key = std::string(AnsiString("http_status_code").c_str());
				oJson.Add(key, AResponseInfo->ResponseNo);
				key = std::string(AnsiString("err_code").c_str());
				oJson.Add(key, pending);
				key = std::string(AnsiString("err_msg").c_str());
				value = std::string(AnsiString(L"抽奖正在进行中").c_str());
				oJson.Add(key, value);
				key = std::string(AnsiString("raffle_id").c_str());
				oJson.Add(key, dices[j].c[raffle_id]);
				key = std::string(AnsiString(s_dice[award_type]).c_str());
				oJson.Add(key, dices[j].c[award_type]);
				key = std::string(AnsiString(s_dice[award_quota]).c_str());
				oJson.Add(key, dices[j].c[award_quota]);
				key = std::string(AnsiString(s_dice[gift_floor]).c_str());
				oJson.Add(key, dices[j].c[gift_floor]);
				key = std::string(AnsiString(s_dice[gift_ceiling]).c_str());
				oJson.Add(key, dices[j].c[gift_ceiling]);
				key = std::string(AnsiString(s_dice_result[actor_num]).c_str());
				oJson.Add(key, dices[j].v[actor_num]);
				key = std::string(AnsiString("gifticon").c_str());
				oJson.Add(key, AnsiString(dices[j].gifticon).c_str());
				key = std::string(AnsiString(s_dice_result[lucky_dog]).c_str());
				oJson.Add(key, dices[j].v[lucky_dog]);
				key = std::string(AnsiString(s_dice_result[crt_total]).c_str());
				oJson.Add(key, dices[j].v[crt_total]);
				AResponseInfo->ContentText = String(oJson.ToString().c_str());

				return;
			}
		}
	}
	if (1024 == j) {

		if (its.ts) {
			key = std::string(AnsiString("http_status_code").c_str());
			oJson.Add(key, AResponseInfo->ResponseNo);
			key = std::string(AnsiString("err_code").c_str());

			if (-1 == dices[its.i].v[lucky_dog]) {
				oJson.Add(key, 78);
				key = std::string(AnsiString("err_msg").c_str());
				value = std::string(AnsiString(L"未找到主播相关的抽奖信息").c_str());
				oJson.Add(key, value);
			}
			else if (dices[its.i].v[lucky_dog] > 0) {
				oJson.Add(key, 78);
				key = std::string(AnsiString("err_msg").c_str());
				value = std::string(AnsiString(L"抽奖活动已完成").c_str());
				oJson.Add(key, value);
			}
			key = std::string(AnsiString("raffle_id").c_str());
			oJson.Add(key, dices[its.i].c[raffle_id]);
			key = std::string(AnsiString(s_dice[award_type]).c_str());
			oJson.Add(key, dices[its.i].c[award_type]);
			key = std::string(AnsiString(s_dice[award_quota]).c_str());
			oJson.Add(key, dices[its.i].c[award_quota]);
			key = std::string(AnsiString(s_dice[gift_floor]).c_str());
			oJson.Add(key, dices[its.i].c[gift_floor]);
			key = std::string(AnsiString(s_dice[gift_ceiling]).c_str());
			oJson.Add(key, dices[its.i].c[gift_ceiling]);
			key = std::string(AnsiString(s_dice_result[actor_num]).c_str());
			oJson.Add(key, dices[its.i].v[actor_num]);
			key = std::string(AnsiString("gifticon").c_str());
			oJson.Add(key, AnsiString(dices[its.i].gifticon).c_str());
			key = std::string(AnsiString(s_dice_result[lucky_dog]).c_str());
			oJson.Add(key, dices[its.i].v[lucky_dog]);
			key = std::string(AnsiString(s_dice_result[crt_total]).c_str());
			oJson.Add(key, dices[its.i].v[crt_total]);
			AResponseInfo->ContentText = String(oJson.ToString().c_str());
		}
		else {
			key = std::string(AnsiString("http_status_code").c_str());
			oJson.Add(key, AResponseInfo->ResponseNo);
			key = std::string(AnsiString("err_code").c_str());
			oJson.Add(key, 78);
			key = std::string(AnsiString("err_msg").c_str());
			value = std::string(AnsiString(L"未找到主播相关的抽奖信息").c_str());
			oJson.Add(key, value);
			key = std::string(AnsiString("raffle_id").c_str());
			oJson.Add(key, dices[its.i].c[raffle_id]);
			key = std::string(AnsiString(s_dice[award_type]).c_str());
			oJson.Add(key, dices[its.i].c[award_type]);
			key = std::string(AnsiString(s_dice[award_quota]).c_str());
			oJson.Add(key, dices[its.i].c[award_quota]);
			key = std::string(AnsiString(s_dice[gift_floor]).c_str());
			oJson.Add(key, dices[its.i].c[gift_floor]);
			key = std::string(AnsiString(s_dice[gift_ceiling]).c_str());
			oJson.Add(key, dices[its.i].c[gift_ceiling]);
			key = std::string(AnsiString(s_dice_result[actor_num]).c_str());
			oJson.Add(key, dices[its.i].v[actor_num]);
			key = std::string(AnsiString("gifticon").c_str());
			oJson.Add(key, AnsiString(dices[its.i].gifticon).c_str());
			key = std::string(AnsiString(s_dice_result[lucky_dog]).c_str());
			oJson.Add(key, dices[its.i].v[lucky_dog]);
			key = std::string(AnsiString(s_dice_result[crt_total]).c_str());
			oJson.Add(key, dices[its.i].v[crt_total]);
			AResponseInfo->ContentText = String(oJson.ToString().c_str());
		}
	}
}

// ---------------------------------------------------------------------------
void __fastcall anchorman_droping(TIdHTTPResponseInfo *AResponseInfo, __int64 raffle_id2) {
	int k, j, l, t_votes_total, err_code, crt_total2, lucky_dog2;
	TUniConnection *conn;
	TUniQuery *uniq1;
	double t_votes;
	String err_str, tmp_str;
	std::string key, value;
	neb::CJsonObject oJson;

	ind_ts its;
	its.i = 0;
	its.ts = 0;

	for (j = 0; j < 1024; ++j) {
		SetEvent(dices[j].doing);
		k = phase(&dices[j]);
		if (1 == k)
			continue;

		if (2 == k) {
			if (dices[j].c[raffle_id] == raffle_id2) {
				if (its.ts < dices[j].c[utimestamp]) {
					its.ts = dices[j].c[utimestamp];
					its.i = j;
				}
			}
		}

		if (3 == k) {
			if (dices[j].c[raffle_id] == raffle_id2) {
				if (dices[j].c[gift_ceiling] <= dices[j].v[crt_total]) {
					key = std::string(AnsiString("http_status_code").c_str());
					oJson.Add(key, AResponseInfo->ResponseNo);
					key = std::string(AnsiString("err_code").c_str());
					oJson.Add(key, 80);
					key = std::string(AnsiString("err_msg").c_str());
					value = std::string(AnsiString(L"已达成抽奖条件，锁定").c_str());
					oJson.Add(key, value);
					key = std::string(AnsiString(s_dice[raffle_id]).c_str());
					oJson.Add(key, dices[j].c[raffle_id]);
					key = std::string(AnsiString(s_dice[utimestamp]).c_str());
					oJson.Add(key, dices[j].c[utimestamp]);
					key = std::string(AnsiString(s_dice[gift_ceiling]).c_str());
					oJson.Add(key, dices[j].c[gift_ceiling]);
					key = std::string(AnsiString(s_dice_result[utimestamp]).c_str());
					oJson.Add(key, dices[j].v[crt_total]);
					key = std::string(AnsiString(s_dice_result[actor_num]).c_str());
					oJson.Add(key, dices[j].v[actor_num]);
					key = std::string(AnsiString(s_dice_result[lucky_dog]).c_str());
					oJson.Add(key, dices[j].v[lucky_dog]);
					AResponseInfo->ContentText = String(oJson.ToString().c_str());
				}
				else {

                    // add the raffle's result
					dices[j].v[lucky_dog] = -1;
					SetEvent(dices[j].work_out);
					tmp_str =
						Sysutils::Format
						("insert into cmf_win_raffle_result (anchorman_id, raffle_id, raffle_status, lucky_dog, award_quota, raffle_timestamp) values (%d, %d, %d, %d, %d, %d);",
						ARRAYOFCONST((dices[j].c[anchorman_id], dices[j].c[raffle_id], -1, -1, dices[j].c[award_quota],
						dices[j].c[utimestamp])));

					UNICONN(conn, uniq1, tmp_str);

					if (conn->InTransaction);
					else {
						conn->StartTransaction();
						try {
							uniq1->Execute();
							conn->Commit();
						}
						catch (...) {
							conn->Rollback();
						}
					}

                    // retrive anchorman's votes, votestotal information
					tmp_str = Sysutils::Format("select votes, votestotal from cmf_users where id = %d;",
						ARRAYOFCONST((dices[j].c[anchorman_id])));
					uniq1->SQL->Clear();
					uniq1->SQL->Add(tmp_str);
					uniq1->Execute();

					if (uniq1->RecordCount) {
						uniq1->First();
						t_votes = uniq1->FieldByName(L"votes")->AsFloat;
						t_votes_total = uniq1->FieldByName(L"votestotal")->AsInteger;
					}
					else {
						key = std::string(AnsiString("http_status_code").c_str());
						oJson.Add(key, AResponseInfo->ResponseNo);
						key = std::string(AnsiString("err_code").c_str());
						oJson.Add(key, 104);
						key = std::string(AnsiString("err_msg").c_str());
						value = std::string(AnsiString(L"数据库取不到主播的votes和votestotal数据").c_str());
						oJson.Add(key, value);
						AResponseInfo->ContentText = String(oJson.ToString().c_str());
						UNUNICONN(conn, uniq1);
                        return ;
					}

                    // update anchorman's votes, votes_total
					l = dices[j].c[unit_price] * dices[j].v[crt_total];
					t_votes += l;
					t_votes_total += l;
					tmp_str = Sysutils::Format("update cmf_users set votes = %f, votestotal = %d  where id = %d;",
						ARRAYOFCONST((t_votes, t_votes_total, dices[j].c[anchorman_id])));
					uniq1->SQL->Clear();
					uniq1->SQL->Add(tmp_str);

					if (conn->InTransaction);
					else {
						conn->StartTransaction();
						try {
							uniq1->Execute();
							conn->Commit();
						}
						catch (...) {
							conn->Rollback();
						}
					}

					UNUNICONN(conn, uniq1);

					key = std::string(AnsiString("http_status_code").c_str());
					oJson.Add(key, AResponseInfo->ResponseNo);
					key = std::string(AnsiString("err_code").c_str());
					oJson.Add(key, 0);
					key = std::string(AnsiString("err_msg").c_str());
					value = std::string(AnsiString(L"主播取消成功").c_str());
					oJson.Add(key, value);
					key = std::string(AnsiString(s_dice[raffle_id]).c_str());
					oJson.Add(key, dices[j].c[raffle_id]);
					key = std::string(AnsiString(s_dice[utimestamp]).c_str());
					oJson.Add(key, dices[j].c[utimestamp]);
					key = std::string(AnsiString(s_dice[gift_ceiling]).c_str());
					oJson.Add(key, dices[j].c[gift_ceiling]);
					key = std::string(AnsiString(s_dice_result[utimestamp]).c_str());
					oJson.Add(key, dices[j].v[crt_total]);
					key = std::string(AnsiString(s_dice_result[actor_num]).c_str());
					oJson.Add(key, dices[j].v[actor_num]);
					key = std::string(AnsiString(s_dice_result[lucky_dog]).c_str());
					oJson.Add(key, dices[j].v[lucky_dog]);
					AResponseInfo->ContentText = String(oJson.ToString().c_str());
				}
				return;
			}
		}
	}

	if (1024 == j) {

		if (its.ts) {
			key = std::string(AnsiString("http_status_code").c_str());
			oJson.Add(key, AResponseInfo->ResponseNo);
			key = std::string(AnsiString("err_code").c_str());

			if (-1 == dices[its.i].v[lucky_dog]) {
				oJson.Add(key, 86);
				key = std::string(AnsiString("err_msg").c_str());
				value = std::string(AnsiString(L"主播已经取消抽奖").c_str());
				oJson.Add(key, value);
			}
			else if (dices[its.i].v[lucky_dog] > 0) {
				oJson.Add(key, completed);
				key = std::string(AnsiString("err_msg").c_str());
				value = std::string(AnsiString(L"抽奖活动已完成").c_str());
				oJson.Add(key, value);
			}
			key = std::string(AnsiString(s_dice[raffle_id]).c_str());
			oJson.Add(key, dices[its.i].c[raffle_id]);
			key = std::string(AnsiString(s_dice[utimestamp]).c_str());
			oJson.Add(key, dices[its.i].c[utimestamp]);
			key = std::string(AnsiString(s_dice[gift_ceiling]).c_str());
			oJson.Add(key, dices[its.i].c[gift_ceiling]);
			key = std::string(AnsiString(s_dice_result[utimestamp]).c_str());
			oJson.Add(key, dices[its.i].v[crt_total]);
			key = std::string(AnsiString(s_dice_result[actor_num]).c_str());
			oJson.Add(key, dices[its.i].v[actor_num]);
			key = std::string(AnsiString(s_dice_result[lucky_dog]).c_str());
			oJson.Add(key, dices[its.i].v[lucky_dog]);
			AResponseInfo->ContentText = String(oJson.ToString().c_str());
		}
		else {
			key = std::string(AnsiString("http_status_code").c_str());
			oJson.Add(key, AResponseInfo->ResponseNo);
			key = std::string(AnsiString("err_code").c_str());
			oJson.Add(key, 98);
			key = std::string(AnsiString("err_msg").c_str());
			value = std::string(AnsiString(L"找不到对应的抽奖活动ID").c_str());
			oJson.Add(key, value);
			key = std::string(AnsiString(s_dice[raffle_id]).c_str());
			oJson.Add(key, dices[j - 1].c[raffle_id]);
			key = std::string(AnsiString(s_dice[utimestamp]).c_str());
			oJson.Add(key, dices[j - 1].c[utimestamp]);
			key = std::string(AnsiString(s_dice[gift_ceiling]).c_str());
			oJson.Add(key, dices[j - 1].c[gift_ceiling]);
			key = std::string(AnsiString(s_dice_result[utimestamp]).c_str());
			oJson.Add(key, dices[j - 1].v[crt_total]);
			key = std::string(AnsiString(s_dice_result[actor_num]).c_str());
			oJson.Add(key, dices[j - 1].v[actor_num]);
			key = std::string(AnsiString(s_dice_result[lucky_dog]).c_str());
			oJson.Add(key, dices[j - 1].v[lucky_dog]);
			AResponseInfo->ContentText = String(oJson.ToString().c_str());
		}

	}
}

// ---------------------------------------------------------------------------
void __fastcall anchorman_polling(TIdHTTPResponseInfo *AResponseInfo, __int64 raffle_id2) {
	int k, j, l, m, t_votes_total, t_coin;
	TUniConnection *conn;
	TUniQuery *uniq1;
	String err_str, tmp_str;
	double t_votes;
	std::string key, value;
	neb::CJsonObject oJson;

	ind_ts its;
	its.i = 0;
	its.ts = 0;

	for (j = 0; j < 1024; ++j) {
		SetEvent(dices[j].doing);
		k = phase(&dices[j]);
		if (1 == k)
			continue;

		if (2 == k) {
			if (dices[j].c[raffle_id] == raffle_id2) {
				if (its.ts < dices[j].c[utimestamp]) {
					its.ts = dices[j].c[utimestamp];
					its.i = j;
				}
			}
		}

		if (3 == k) {
			if (dices[j].c[raffle_id] != raffle_id2);
			else {

				if (dices[j].c[gift_ceiling] <= dices[j].v[crt_total]) {
					draw(&(dices[j]));
					SetEvent(dices[j].work_out);
					tmp_str =
						Sysutils::Format
						("insert into cmf_win_raffle_result (anchorman_id, raffle_id, raffle_status, lucky_dog, award_quota, raffle_timestamp) values (%d, %d, %d, %d, %d, %d);",
						ARRAYOFCONST((dices[j].c[anchorman_id], dices[j].c[raffle_id], completed, dices[j].v[lucky_dog],
						dices[j].c[award_quota], dices[j].c[utimestamp])));

					UNICONN(conn, uniq1, tmp_str);

					if (conn->InTransaction);
					else {
						conn->StartTransaction();
						try {
							uniq1->Execute();
							conn->Commit();
						}
						catch (...) {
							conn->Rollback();
						}
					}

					// retieve anchorman' s current votes, votestotal
					tmp_str = Sysutils::Format("select coin, votes, votestotal from cmf_users where id = %d;",
						ARRAYOFCONST((dices[j].c[anchorman_id])));
					uniq1->SQL->Clear();
					uniq1->SQL->Add(tmp_str);

					uniq1->Execute();

					if (uniq1->RecordCount) {
						uniq1->First();
						t_votes = uniq1->FieldByName(L"votes")->AsFloat;
						t_votes_total = uniq1->FieldByName(L"votestotal")->AsInteger;
						t_coin = uniq1->FieldByName(L"coin")->AsInteger;
					}
					else {
						key = std::string(AnsiString("http_status_code").c_str());
						oJson.Add(key, AResponseInfo->ResponseNo);
						key = std::string(AnsiString("err_code").c_str());
						oJson.Add(key, 104);
						key = std::string(AnsiString("err_msg").c_str());
						value = std::string(AnsiString(L"数据库取不到主播votes、votestotal数据").c_str());
						oJson.Add(key, value);
						AResponseInfo->ContentText = String(oJson.ToString().c_str());
						UNUNICONN(conn, uniq1);
						return;
					}

					// add (gift number * gift price) to anchorman's votes, votestotal
					l = dices[j].c[unit_price] * dices[j].v[crt_total];
					t_votes += l;
					t_votes_total += l;
					tmp_str = Sysutils::Format("update cmf_users set votes = %f, votestotal = %d where id = %d;",
						ARRAYOFCONST((t_votes, t_votes_total, dices[j].c[anchorman_id])));
					uniq1->SQL->Clear();
					uniq1->SQL->Add(tmp_str);
					if (conn->InTransaction);
					else {
						conn->StartTransaction();
						try {
							uniq1->Execute();
							conn->Commit();
						}
						catch (...) {
							conn->Rollback();
						}
					}

					// if the award is gem,
					if (dices[j].c[award_type]) {
						// retrieve lucky dog's current coin
						tmp_str = Sysutils::Format("select coin from cmf_users where id = %d;",
							ARRAYOFCONST((dices[j].v[lucky_dog])));
						uniq1->SQL->Clear();
						uniq1->SQL->Add(tmp_str);

						uniq1->Execute();
						if (uniq1->RecordCount) {
							uniq1->First();
							m = uniq1->FieldByName(L"coin")->AsInteger;
						}
						else {
							key = std::string(AnsiString("http_status_code").c_str());
							oJson.Add(key, AResponseInfo->ResponseNo);
							key = std::string(AnsiString("err_code").c_str());
							oJson.Add(key, 104);
							key = std::string(AnsiString("err_msg").c_str());
							value = std::string(AnsiString(L"数据库取不到lucy dog的钻石数").c_str());
							oJson.Add(key, value);
							AResponseInfo->ContentText = String(oJson.ToString().c_str());
							UNUNICONN(conn, uniq1);
							return;
						}

						// add the award to the lucky dog's coins
						m += dices[j].c[award_quota];
						uniq1->SQL->Clear();
						tmp_str = Sysutils::Format("update cmf_users set coin = %d where id = %d;",
							ARRAYOFCONST((m, dices[j].v[lucky_dog])));
                        // add lucy dog's income record
						tmp_str +=
							Sysutils::Format
							("insert into cmf_users_coinrecord(type, action, uid, touid, giftid, giftcount, totalcoin, showid, addtime, game_banker, game_action, mark) values('%s', '%s', %d, %d, %d, %d, %d, %d, %d, 0, 0, 0);",
							ARRAYOFCONST((String(L"income"), String(L"dice"), dices[j].c[anchorman_id],
							dices[j].v[lucky_dog], dices[j].c[award_type], dices[j].c[award_quota],
							dices[j].c[award_quota], dices[j].c[show_id], System::Dateutils::DateTimeToUnix(Now()))));

						// reduce anchorman's coins
						t_coin -= dices[j].c[award_quota];
						tmp_str += Sysutils::Format("update cmf_users set coin = %d where id = %d;",
							ARRAYOFCONST((t_coin, dices[j].c[anchorman_id])));
                        // add anchorman's expend record
						tmp_str +=
							Sysutils::Format
							("insert into cmf_users_coinrecord(type, action, uid, touid, giftid, giftcount, totalcoin, showid, addtime, game_banker, game_action, mark) values('%s', '%s', %d, %d, %d, %d, %d, %d, %d, 0, 0, 0);",
							ARRAYOFCONST((String(L"expend"), String(L"dice"), dices[j].c[anchorman_id],
							dices[j].v[lucky_dog], dices[j].c[award_type], dices[j].c[award_quota],
							dices[j].c[award_quota], dices[j].c[show_id], System::Dateutils::DateTimeToUnix(Now()))));
						uniq1->SQL->Add(tmp_str);

						if (conn->InTransaction);
						else {
							conn->StartTransaction();
							try {
								uniq1->Execute();
								conn->Commit();
							}
							catch (...) {
								conn->Rollback();
							}
						}
					}
					else {

					}

					UNUNICONN(conn, uniq1);
					key = std::string(AnsiString("http_status_code").c_str());
					oJson.Add(key, AResponseInfo->ResponseNo);
					key = std::string(AnsiString("err_code").c_str());
					oJson.Add(key, completed);
					key = std::string(AnsiString("err_msg").c_str());
					value = std::string(AnsiString(L"获得抽奖结果").c_str());
					oJson.Add(key, value);
					key = std::string(AnsiString(s_dice[raffle_id]).c_str());
					oJson.Add(key, dices[j].c[raffle_id]);
					key = std::string(AnsiString(s_dice[utimestamp]).c_str());
					oJson.Add(key, dices[j].c[utimestamp]);
					key = std::string(AnsiString(s_dice[gift_ceiling]).c_str());
					oJson.Add(key, dices[j].c[gift_ceiling]);
					key = std::string(AnsiString(s_dice_result[crt_total]).c_str());
					oJson.Add(key, dices[j].v[crt_total]);
					key = std::string(AnsiString(s_dice_result[actor_num]).c_str());
					oJson.Add(key, dices[j].v[actor_num]);
					key = std::string(AnsiString(s_dice_result[lucky_dog]).c_str());
					oJson.Add(key, dices[j].v[lucky_dog]);
					AResponseInfo->ContentText = String(oJson.ToString().c_str());

				}
				else {

					key = std::string(AnsiString("http_status_code").c_str());
					oJson.Add(key, AResponseInfo->ResponseNo);
					key = std::string(AnsiString("err_code").c_str());
					oJson.Add(key, pending);
					key = std::string(AnsiString("err_msg").c_str());
					value = std::string(AnsiString(L"抽奖进行中").c_str());
					oJson.Add(key, value);
					key = std::string(AnsiString(s_dice[raffle_id]).c_str());
					oJson.Add(key, dices[j].c[raffle_id]);
					key = std::string(AnsiString(s_dice[utimestamp]).c_str());
					oJson.Add(key, dices[j].c[utimestamp]);
					key = std::string(AnsiString(s_dice[gift_ceiling]).c_str());
					oJson.Add(key, dices[j].c[gift_ceiling]);
					key = std::string(AnsiString(s_dice_result[utimestamp]).c_str());
					oJson.Add(key, dices[j].v[crt_total]);
					key = std::string(AnsiString(s_dice_result[actor_num]).c_str());
					oJson.Add(key, dices[j].v[actor_num]);
					key = std::string(AnsiString(s_dice_result[lucky_dog]).c_str());
					oJson.Add(key, dices[j].v[lucky_dog]);
					AResponseInfo->ContentText = String(oJson.ToString().c_str());
				}

				return;
			}
		}
	}

	if (1024 == j) {
		if (its.ts) {
			key = std::string(AnsiString("http_status_code").c_str());
			oJson.Add(key, AResponseInfo->ResponseNo);
			key = std::string(AnsiString("err_code").c_str());

			if (-1 == dices[its.i].v[lucky_dog]) {
				oJson.Add(key, 86);
				key = std::string(AnsiString("err_msg").c_str());
				value = std::string(AnsiString(L"主播已经取消抽奖").c_str());
				oJson.Add(key, value);
			}
			else if (dices[its.i].v[lucky_dog] > 0) {
				oJson.Add(key, completed);
				key = std::string(AnsiString("err_msg").c_str());
				value = std::string(AnsiString(L"抽奖活动已完成").c_str());
				oJson.Add(key, value);
			}
			key = std::string(AnsiString(s_dice[raffle_id]).c_str());
			oJson.Add(key, dices[its.i].c[raffle_id]);
			key = std::string(AnsiString(s_dice[utimestamp]).c_str());
			oJson.Add(key, dices[its.i].c[utimestamp]);
			key = std::string(AnsiString(s_dice[gift_ceiling]).c_str());
			oJson.Add(key, dices[its.i].c[gift_ceiling]);
			key = std::string(AnsiString(s_dice[utimestamp]).c_str());
			oJson.Add(key, dices[its.i].v[crt_total]);
			key = std::string(AnsiString(s_dice_result[actor_num]).c_str());
			oJson.Add(key, dices[its.i].v[actor_num]);
			key = std::string(AnsiString(s_dice_result[lucky_dog]).c_str());
			oJson.Add(key, dices[its.i].v[lucky_dog]);
			AResponseInfo->ContentText = String(oJson.ToString().c_str());
		}
		else {
			key = std::string(AnsiString("http_status_code").c_str());
			oJson.Add(key, AResponseInfo->ResponseNo);
			key = std::string(AnsiString("err_code").c_str());
			oJson.Add(key, 98);
			key = std::string(AnsiString("err_msg").c_str());
			value = std::string(AnsiString(L"找不到对应的抽奖活动ID").c_str());
			oJson.Add(key, value);
			key = std::string(AnsiString(s_dice[raffle_id]).c_str());
			oJson.Add(key, dices[j].c[raffle_id]);
			key = std::string(AnsiString(s_dice[utimestamp]).c_str());
			oJson.Add(key, dices[j].c[utimestamp]);
			key = std::string(AnsiString(s_dice[gift_ceiling]).c_str());
			oJson.Add(key, dices[j].c[gift_ceiling]);
			key = std::string(AnsiString(s_dice_result[utimestamp]).c_str());
			oJson.Add(key, dices[j].v[crt_total]);
			key = std::string(AnsiString(s_dice_result[actor_num]).c_str());
			oJson.Add(key, dices[j].v[actor_num]);
			key = std::string(AnsiString(s_dice_result[lucky_dog]).c_str());
			oJson.Add(key, dices[j].v[lucky_dog]);
			AResponseInfo->ContentText = String(oJson.ToString().c_str());
		}
	}
}

// ---------------------------------------------------------------------------
void __fastcall anchorman_gambling(TIdHTTPResponseInfo *AResponseInfo, __int64 i) {
	bool err;
	int err_code;
	int j, k;
	TUniConnection *conn;
	TUniQuery *uniq1;
	String err_str, tmp_str;
	std::string key, value;
	neb::CJsonObject oJson;
	dice & new_dice = dices[i];
	ind_ts its;
	its.i = 0;
	its.ts = 0;

	for (j = 0; j < 1024; ++j) {
		SetEvent(dices[j].doing);
		k = phase(&dices[j]);
		if (1 == k)
			continue;

		if (2 == k) {
			if (dices[j].c[anchorman_id] == new_dice.c[anchorman_id]) {
				if (its.ts < dices[j].c[utimestamp]) {
					its.ts = dices[j].c[utimestamp];
					its.i = j;
				}
			}
		}

		if (3 == k) {
			if (dices[j].c[anchorman_id] != new_dice.c[anchorman_id]);
			else {
				if (j != i) {
					key = std::string(AnsiString("http_status_code").c_str());
					oJson.Add(key, AResponseInfo->ResponseNo);
					key = std::string(AnsiString("err_code").c_str());
					oJson.Add(key, 1);
					key = std::string(AnsiString("err_msg").c_str());
					value = std::string(AnsiString(L"该主播已发起抽奖活动！").c_str());
					oJson.Add(key, value);
					key = std::string(AnsiString(s_dice[raffle_id]).c_str());
					oJson.Add(key, dices[j].c[raffle_id]);
					key = std::string(AnsiString(s_dice[utimestamp]).c_str());
					oJson.Add(key, dices[j].c[utimestamp]);
					AResponseInfo->ContentText = String(oJson.ToString().c_str());
					reset_dice(&new_dice);
					return;
				}
			}
		}
	}

	if (1024 == j) {

		new_dice.c[utimestamp] = System::Dateutils::DateTimeToUnix(Now());
		new_dice.c[raffle_id] = InterlockedIncrement64(&global_id);
		tmp_str = Sysutils::Format(L"select giftname, needcoin, gifticon from cmf_gift where id = %d;",
			ARRAYOFCONST((new_dice.c[gift_id])));
		UNICONN(conn, uniq1, tmp_str);
		uniq1->Execute();

		new_dice.c[unit_price] = uniq1->FieldByName(L"needcoin")->AsInteger;
		new_dice.gift_name = uniq1->FieldByName(L"giftname")->AsString;
		new_dice.gifticon = uniq1->FieldByName(L"gifticon")->AsString;

		key = std::string(AnsiString("http_status_code").c_str());
		oJson.Add(key, AResponseInfo->ResponseNo);
		key = std::string(AnsiString("err_code").c_str());
		oJson.Add(key, 0);
		key = std::string(AnsiString("err_msg").c_str());
		value = std::string(AnsiString(L"成功启动抽奖活动").c_str());
		oJson.Add(key, value);
		key = std::string(AnsiString(s_dice[raffle_id]).c_str());
		oJson.Add(key, new_dice.c[raffle_id]);
		key = std::string(AnsiString(s_dice[utimestamp]).c_str());
		oJson.Add(key, new_dice.c[utimestamp]);
		AResponseInfo->ContentText = String(oJson.ToString().c_str());

		UNUNICONN(conn, uniq1);
	}
}

// ---------------------------------------------------------------------------
void __fastcall TDataModule1::ScSSHClient1ServerKeyValidate(TObject *Sender, TScKey *NewServerKey, bool &Accept) {
	TScKey* key;
	String fp, msg;

	// key = ScFileStorage1->Keys->FindKey(ScSSHClient1->HostKeyName);
	key = ScMemoryStorage1->Keys->FindKey(ScSSHClient1->HostKeyName);
	if (!key || !(key->Ready) || (key && !NewServerKey->Equals(key))) {
		NewServerKey->GetFingerprint(Scutils::haMD5, fp);
		NewServerKey->KeyName = ScSSHClient1->HostName;
		// ScFileStorage1->Keys->Add(NewServerKey);
		ScMemoryStorage1->Keys->Add(NewServerKey);
	}
	Accept = true;

	/*
	 if(key && key->Ready && NewServerKey->Equals(key));
	 else{

	 }
	 */
}

// ---------------------------------------------------------------------------
void __fastcall TDataModule1::DataModuleCreate(TObject *Sender) {

	int i;
	ScSSHClient1->HostName = L"152.32.234.119";
	ScSSHClient1->Authentication = atPassword;
	ScSSHClient1->Port = 22;
	ScSSHClient1->User = L"root";
	ScSSHClient1->Password = L"07Ph50Dv#+U";
	ScSSHClient1->KeyStorage = ScMemoryStorage1;
	CRSSHIOHandler1->Client = ScSSHClient1;
	UniConnection1->ProviderName = L"MySQL";
	UniConnection1->Server = L"10.11.178.103";
	UniConnection1->IOHandler = CRSSHIOHandler1;
	UniConnection1->Port = 3306;
	UniConnection1->Username = L"root";
	UniConnection1->Password = L"wFIk9n0=Jqs2";
	UniConnection1->Database = L"live";
	UniConnection1->LoginPrompt = false;
	UniConnection1->SpecificOptions->Values[L"Charset"] = L"utf8mb4";
	UniConnection1->SpecificOptions->Values[L"UseUnicode"] = L"True";

	UniConnection2->ProviderName = L"MySQL";
	UniConnection2->Server = L"10.11.178.103";
	UniConnection2->IOHandler = CRSSHIOHandler1;
	UniConnection2->Port = 3306;
	UniConnection2->Username = L"root";
	UniConnection2->Password = L"wFIk9n0=Jqs2";
	UniConnection2->Database = L"live";
	UniConnection2->LoginPrompt = false;
	UniConnection2->SpecificOptions->Values[L"Charset"] = L"utf8mb4";
	UniConnection2->SpecificOptions->Values[L"UseUnicode"] = L"True";

	UniScript1->Connection = UniConnection1;
	UniScript1->SQL->Clear();
	UniScript1->SQL->Add(L"create table if not exists cmf_win_raffle_anchorman(\
anchorman_id int(20) unsigned not null,\
show_id int(12) not null,\
gift_id int(11) unsigned not null,\
gift_least tinyint default 1,\
award_type tinyint default 0,\
award_quota tinyint not null,\
raffle_condition tinyint default 0,\
raffle_id bigint not null,\
cts datetime default current_timestamp,\
rowid_2th integer primary key auto_increment,\
unique(anchorman_id, show_id)\
)engine=InnoDB default charset=utf8mb4 auto_increment=10001;create table if not exists cmf_win_raffle_result(\
anchorman_id int(20) unsigned not null,\
raffle_id bigint,\
raffle_status tinyint,\
lucky_dog int(20),\
award_type tinyint default 0,\
award_quota tinyint,\
raffle_timestamp int,\
rowid_2th integer primary key auto_increment,\
unique(raffle_id)\
)engine=InnoDB default charset=utf8mb4 auto_increment=11001;create table if not exists cmf_win_raffle_personal(\
audience_id int(20) unsigned not null,\
raffle_id bigint,\
raffle_status tinyint,\
gift_id int(11) unsigned,\
giftname varchar(50) default '',\
gift_amount int,\
gifticon varchar(255) default '',\
gems_consumed int,\
is_lucky_dog tinyint default 0,\
award_type tinyint default 0,\
award_quota tinyint,\
raffle_timestamp int,\
rowid_2th integer primary key auto_increment,\
unique(audience_id,raffle_id)\
)engine=InnoDB default charset=utf8mb4 auto_increment=12001;create table if not exists cmf_win_raffle_dice(\
raffle_id bigint,\
from__ int(20) unsigned,\
to__ int(20) unsigned,\
gift_id int(11) unsigned,\
gift_amount int,\
gems_consumed int,\
gems_awarded int,\
rowid_2th integer primary key auto_increment\
)engine=InnoDB default charset=utf8mb4 auto_increment=13001;");
	UniQuery1->Connection = UniConnection1;
	UniQuery1->SQL->Clear();
	UniQuery1->SQL->Add(L"select raffle_id from cmf_win_raffle_result order by raffle_id desc limit 1;");

	UniQuery2->Connection = UniConnection1;
	UniQuery2->SQL->Clear();
	UniQuery2->SQL->Add(L"show variables like'%max_connections%';");

	try {
		ScSSHClient1->Connect();
		UniConnection1->Connect();
		UniConnection2->Connect();
		UniScript1->Execute();
		UniQuery1->Execute();
		UniQuery2->Execute();
	}
	catch (...) {
		OutputDebugString(L"connect to mysql failed");
		return;
	}

	if (UniQuery1->RecordCount) {
		global_id = UniQuery1->FieldByName(L"raffle_id")->AsLargeInt;
	}
	else {
		global_id = 11001;
	}

	mysql_max_conn = UniQuery2->FieldByName(L"Value")->AsInteger;
	IdHTTPServer1->DefaultPort = 27757;

	index_dices = -1;

	for (i = 0; i < 1024; ++i) {
		init_dice(&(dices[i]));
	}

	IdHTTPServer1->Active = true;

	// uniconn_local->ProviderName = "SQLite";
	// uniconn_local->SpecificOptions->Values["Direct"] = "True";
	// uniconn_local->SpecificOptions->Values["ForceCreateDatabase"] = "True";
	// uniconn_local->Database = ExtractFilePath(ParamStr(0)) + "Dice0302.db";
	// uniconn_local->Connect();
	//
	// uniscript_local->SQL->Clear();
	// uniscript_local->SQL->Add("create table if not exists playdice (\
	// raffle_id integer,\
	// audience_id integer,\
	// anchorman_id integer,\
	// gift_id integer,\
	// gift_amount integer,\
	// gems_consumed integer,\
	// rowid_2th integer primary key autoincrement\
	// );create index if not exists idx_anchorman on playdice (anchorman_id);\
	// create index if not exists idx_audience on playdice (audience_id);\
	// create index if not exists idx_raffle on playdice (raffle_id);");
	// uniscript_local->Execute();
}
// ---------------------------------------------------------------------------

void __fastcall init_dice(dice *d) {
	SecureZeroMemory(d->c, sizeof(__int64)* e_dice_end);
	SecureZeroMemory(d->v, sizeof(__int64) * e_dice_result_end);
	SecureZeroMemory(d->actors, sizeof(audi) * 1024);
	SecureZeroMemory(d->bystanders, sizeof(__int64) * 2048);
	d->work_out = CreateEvent(0, 1, 0, 0);
	d->doing = CreateEvent(0, 0, 0, 0);
	d->recycle = CreateEvent(0, 1, 1, 0);
}

void __fastcall reset_dice(dice *d) {
	EnterCriticalSection(&cs_index_dices);
	SecureZeroMemory(d->c, sizeof(__int64)* e_dice_end);
	SecureZeroMemory(d->v, sizeof(__int64) * e_dice_result_end);
	SecureZeroMemory(d->actors, sizeof(audi) * 1024);
	SecureZeroMemory(d->bystanders, sizeof(__int64) * 2048);
	ResetEvent(d->work_out);
	ResetEvent(d->doing);
	SetEvent(d->recycle);
	LeaveCriticalSection(&cs_index_dices);
}

void __fastcall TDataModule1::DataModuleDestroy(TObject *Sender) {
	IdHTTPServer1->Active = false;
}

// ---------------------------------------------------------------------------

void __fastcall draw(dice *d) {
	int i;
	int j;
	int actor_num1;
	__int64 total = 0;
	__int64 *actors = 0;

	actor_num1 = d->v[actor_num];
	actors = new __int64[actor_num1];

	for (i = 0; i < actor_num1; ++i) {
		total += d->actors[i].gift_amount;
		actors[i] = total;
	}

	srand(time(0));
	j = rand() % ((int)total);

	for (i = 0; i < actor_num1; ++i) {
		if (j < actors[i])
			break;
	}

	d->v[lucky_dog] = d->actors[i].id;
	delete[]actors;
}

// ---------------------------------------------------------------------------
/*
 recycle有信号：1、未被使用；2、用完并且参与者得到消息因此可以被回收；
 -------无信号：被占用；
 -------------workout有信号：1、已有计算结果
 --------------------无信号：
 --------------------------获得doing进行计算
 */
int __fastcall phase(dice *d) {
	int r;
	// if (WAIT_OBJECT_0 == WaitForSingleObject(d->recycle, 0))
	// return 1;
	//
	// if (WAIT_OBJECT_0 == WaitForSingleObject(d->work_out, 0))
	// return 2;
	//
	// if (WAIT_OBJECT_0 == WaitForSingleObject(d->doing, INFINITE))
	// return 3;

	// if (WAIT_OBJECT_0 == WaitForSingleObject(d->recycle, 0)) {
	// r = 1;
	// }
	// else {
	// if (WAIT_OBJECT_0 == WaitForSingleObject(d->work_out, 0)) {
	// r = 2;
	// }
	// else {
	// if (WAIT_OBJECT_0 == WaitForSingleObject(d->doing, INFINITE)) {
	// r = 3;
	// }
	// else {
	// r = -1;
	// }
	// }
	// }

	if (WAIT_OBJECT_0 != WaitForSingleObject(d->recycle, 0)) {
		if (WAIT_OBJECT_0 != WaitForSingleObject(d->work_out, 0)) {
			if (WAIT_OBJECT_0 != WaitForSingleObject(d->doing, INFINITE)) {
				r = -1;
			}
			else {
				r = 3;
			}
		}
		else {
			r = 2;
		}
	}
	else {
		r = 1;
	}

	return r;
}

// ---------------------------------------------------------------------------
void __fastcall TDataModule1::timer_sweeperTimer(TObject *Sender) {
#define DEADLINE 10800
	int j, k, l, m, t_votes_total;
	__int64 ts;
	TUniConnection *conn;
	TUniQuery *uniq1;
	double t_votes;
	String tmp_str;
	for (j = 0; j < 1024; ++j) {
		SetEvent(dices[j].doing);
		k = phase(&dices[j]);
		if (1 == k)
			continue;

        // work_out is set
		if (2 == k) {
			ts = System::Dateutils::DateTimeToUnix(Now());
            // if period is beyond DEADLINE, and there are some actors
			if (((ts - dices[j].c[utimestamp]) >= DEADLINE) && dices[j].v[actor_num]) {

				if (canceled == dices[j].v[lucky_dog]) {
					UNICONN(conn, uniq1, "");
					for (l = 0; l < dices[j].v[actor_num]; ++l) {
						m = dices[j].actors[l].id;
						if (m) {
							tmp_str = Sysutils::Format(L"insert into cmf_win_raffle_personal(audience_id, raffle_id, raffle_status, gift_id, giftname,\
						 gifticon, gift_amount, gems_consumed, is_lucky_dog, award_type, award_quota, raffle_timestamp) \
						 values(%d, %d, -1, %d, '%s', '%s', %d, %d, 0, %d, %d, %d);",
								ARRAYOFCONST((m, dices[j].c[raffle_id], dices[j].c[gift_id], dices[j].gift_name,
								dices[j].gifticon, dices[j].actors[l].gift_amount,
								(dices[j].c[unit_price] * dices[j].actors[l].gift_amount), dices[j].c[award_type],
								dices[j].c[award_quota], dices[j].c[utimestamp])));
							uniq1->SQL->Clear();
							uniq1->SQL->Add(tmp_str);

							if (conn->InTransaction);
							else {
								conn->StartTransaction();
								try {
									uniq1->Execute();
									conn->Commit();
								}
								catch (...) {
									conn->Rollback();
								}
							}

						}
					}
					UNUNICONN(conn, uniq1);
				}
				else if (0 < dices[j].v[lucky_dog]) {
					UNICONN(conn, uniq1, "");
					for (l = 0; l < dices[j].v[actor_num]; ++l) {
						m = dices[j].actors[l].id;
						if (m) {
							tmp_str = Sysutils::Format(L"insert into cmf_win_raffle_personal(audience_id, raffle_id, raffle_status, gift_id, giftname,\
						 gifticon, gift_amount, gems_consumed, is_lucky_dog, award_type, award_quota, raffle_timestamp)\
						  values(%d, %d, 1, %d, '%s', '%s', %d, %d, %d, %d, %d, %d);",
								ARRAYOFCONST((m, dices[j].c[raffle_id], dices[j].c[gift_id], dices[j].gift_name,
								dices[j].gifticon, dices[j].actors[l].gift_amount,
								(dices[j].c[unit_price] * dices[j].actors[l].gift_amount), 1, dices[j].c[award_type],
								dices[j].c[award_quota], dices[j].c[utimestamp])));
							uniq1->SQL->Clear();
							uniq1->SQL->Add(tmp_str);

							if (conn->InTransaction);
							else {
								conn->StartTransaction();
								try {
									uniq1->Execute();
									conn->Commit();
								}
								catch (...) {
									conn->Rollback();
								}
							}
						}
					}
					UNUNICONN(conn, uniq1);
				}
			}
			reset_dice(&(dices[j]));
		}

		if (3 == k) {
			ts = System::Dateutils::DateTimeToUnix(Now());

			if ((DEADLINE <= (ts - dices[j].c[utimestamp])) && dices[j].v[actor_num]) {
				dices[j].v[lucky_dog] = -1;
				SetEvent(dices[j].work_out);
				tmp_str =
					Sysutils::Format
					("insert into cmf_win_raffle_result (anchorman_id, raffle_id, raffle_status, lucky_dog, award_quota, raffle_timestamp) values (%d, %d, %d, %d, %d, %d);",
					ARRAYOFCONST((dices[j].c[anchorman_id], dices[j].c[raffle_id], -1, -1, dices[j].c[award_quota],
					dices[j].c[utimestamp])));

				UNICONN(conn, uniq1, tmp_str);

				if (conn->InTransaction);
				else {
					conn->StartTransaction();
					try {
						uniq1->Execute();
						conn->Commit();
					}
					catch (...) {
						conn->Rollback();
					}
				}

				tmp_str = Sysutils::Format("select votes, votestotal from cmf_users where id = %d;",
					ARRAYOFCONST((dices[j].c[anchorman_id])));
				uniq1->SQL->Clear();
				uniq1->SQL->Add(tmp_str);
				uniq1->Execute();

				if (uniq1->RecordCount) {
					uniq1->First();
					t_votes = uniq1->FieldByName(L"votes")->AsFloat;
					t_votes_total = uniq1->FieldByName(L"votestotal")->AsInteger;
				}
				else {
					UNUNICONN(conn, uniq1);
				}

				l = dices[j].c[unit_price] * dices[j].v[crt_total];
				t_votes += l;
				t_votes_total += l;
				tmp_str = Sysutils::Format("update cmf_users set votes = %f, votestotal = %d  where id = %d;",
					ARRAYOFCONST((t_votes, t_votes_total, dices[j].c[anchorman_id])));
				uniq1->SQL->Clear();
				uniq1->SQL->Add(tmp_str);

				if (conn->InTransaction);
				else {
					conn->StartTransaction();
					try {
						uniq1->Execute();
						conn->Commit();
					}
					catch (...) {
						conn->Rollback();
					}
				}

				for (l = 0; l < dices[j].v[actor_num]; ++l) {
					m = dices[j].actors[l].id;
					if (m) {
						tmp_str = Sysutils::Format(L"insert into cmf_win_raffle_personal(audience_id, raffle_id, raffle_status, gift_id, giftname,\
						 gifticon, gift_amount, gems_consumed, is_lucky_dog, award_type, award_quota, raffle_timestamp) \
						 values(%d, %d, -1, %d, '%s', '%s', %d, %d, 0, %d, %d, %d);",
							ARRAYOFCONST((m, dices[j].c[raffle_id], dices[j].c[gift_id], dices[j].gift_name,
							dices[j].gifticon, dices[j].actors[l].gift_amount,
							(dices[j].c[unit_price] * dices[j].actors[l].gift_amount), dices[j].c[award_type],
							dices[j].c[award_quota], dices[j].c[utimestamp])));
						uniq1->SQL->Clear();
						uniq1->SQL->Add(tmp_str);

						if (conn->InTransaction);
						else {
							conn->StartTransaction();
							try {
								uniq1->Execute();
								conn->Commit();
							}
							catch (...) {
								conn->Rollback();
							}
						}

					}
				}

				UNUNICONN(conn, uniq1);

				reset_dice(&(dices[j]));
			}
		}
	}
}
// ---------------------------------------------------------------------------
