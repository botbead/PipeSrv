// ---------------------------------------------------------------------------

#ifndef hubH
#define hubH
// ---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include "DAScript.hpp"
#include "DBAccess.hpp"
#include "MemDS.hpp"
#include "MySQLUniProvider.hpp"
#include "Uni.hpp"
#include "UniProvider.hpp"
#include "UniScript.hpp"
#include <Data.DB.hpp>
#include <IdBaseComponent.hpp>
#include <IdComponent.hpp>
#include <IdCustomHTTPServer.hpp>
#include <IdCustomTCPServer.hpp>
#include <IdHTTPServer.hpp>
#include "CRSSHIOHandler.hpp"
#include "CRVio.hpp"
#include "ScBridge.hpp"
#include "ScSSHClient.hpp"
#include <IdContext.hpp>
#include "SQLiteUniProvider.hpp"
#include <Vcl.ExtCtrls.hpp>

enum e_doc {
	doc_raffle, e_doc_end
};

enum e_dice {
	anchorman_id, show_id, gift_id, gift_floor, gift_ceiling, award_quota, award_type, raffle_condition, utimestamp,
	raffle_id, unit_price, action, e_dice_end
};

enum e_dice_result {
	crt_total, lucky_dog, actor_num, bystander_num, e_dice_result_end
};

enum e_ctrl {
	canceled = -1, pending, completed, e_ctrl_end
};

struct ind_ts {
	int i;
	__int64 ts;
};

struct audi {
	int gift_amount;
	__int64 id;
};

struct dice {
	void *work_out, *doing, *recycle;
	__int64 c[e_dice_end];
	__int64 v[e_dice_result_end];
	String gift_name, gifticon;
	audi actors[1024];
	__int64 bystanders[2048];
};

// ---------------------------------------------------------------------------
class TDataModule1 : public TDataModule {
__published: // IDE-managed Components
	TMySQLUniProvider *MySQLUniProvider1;
	TUniConnection *UniConnection1;
	TUniScript *UniScript1;
	TUniQuery *UniQuery1;
	TUniQuery *UniQuery2;
	TUniConnection *UniConnection2;
	TUniQuery *UniQuery3;
	TIdHTTPServer *IdHTTPServer1;
	TScMemoryStorage *ScMemoryStorage1;
	TScSSHClient *ScSSHClient1;
	TCRSSHIOHandler *CRSSHIOHandler1;
	TTimer *timer_sweeper;

	void __fastcall ScSSHClient1ServerKeyValidate(TObject *Sender, TScKey *NewServerKey, bool &Accept);
	void __fastcall DataModuleCreate(TObject *Sender);
	void __fastcall DataModuleDestroy(TObject *Sender);
	void __fastcall IdHTTPServer1CommandGet(TIdContext *AContext, TIdHTTPRequestInfo *ARequestInfo,
		TIdHTTPResponseInfo *AResponseInfo);
	void __fastcall timer_sweeperTimer(TObject *Sender);

private: // User declarations
public: // User declarations
	__fastcall TDataModule1(TComponent* Owner);
};

// ---------------------------------------------------------------------------
extern PACKAGE TDataModule1 *DataModule1;
// ---------------------------------------------------------------------------
#endif
