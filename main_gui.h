// ---------------------------------------------------------------------------

#ifndef main_guiH
#define main_guiH
// ---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.ComCtrls.hpp>
#include <System.Net.HttpClient.hpp>
#include <System.Net.HttpClientComponent.hpp>
#include <System.Net.URLClient.hpp>
#include <Soap.SOAPHTTPTrans.hpp>
#include <IdBaseComponent.hpp>
#include <IdComponent.hpp>
#include <IdCustomHTTPServer.hpp>
#include <IdCustomTCPServer.hpp>
#include <IdHTTPServer.hpp>
#include <IdContext.hpp>
#include <IdCoder.hpp>
#include <IdCoder3to4.hpp>
#include <IdCoderMIME.hpp>
#include <System.JSON.hpp>
#include <System.JSON.Types.hpp>
#include <System.JSON.Builders.hpp>
#include <System.JSON.Writers.hpp>
#include <System.JSON.Readers.hpp>
#include <IdTCPServer.hpp>
#include <Vcl.ExtCtrls.hpp>

typedef unsigned char BUF44[44];

struct comm_cmd {
	unsigned char head[2];
	unsigned char length;
	unsigned char cTrl;

	unsigned char ID[4];

	float price;
	float balance;
	float gross;
	float amount;
	float purityTDS;
	float rawTDS;
	float ready1;
	float ready2;

	unsigned char chargingMode;
	unsigned char Family;
	unsigned char eState;
	unsigned char check;
};

// ---------------------------------------------------------------------------
class TForm2 : public TForm {
__published: // IDE-managed Components
	TButton *Button1;
	TButton *Button2;
	TStatusBar *StatusBar1;
	TMemo *Memo1;
	TNetHTTPClient *NetHTTPClient1;
	TNetHTTPRequest *NetHTTPRequest1;
	TIdHTTPServer *IdHTTPServer1;
	TTimer *Timer1;
	TLabel *Label1;
	TEdit *Edit1;
	TTimer *timer_conn;

	void __fastcall FormCreate(TObject *Sender);
	void __fastcall FormDestroy(TObject *Sender);
	void __fastcall Button1Click(TObject *Sender);
	void __fastcall Button2Click(TObject *Sender);
	void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
	void __fastcall IdHTTPServer1CommandGet(TIdContext *AContext,
		TIdHTTPRequestInfo *ARequestInfo, TIdHTTPResponseInfo *AResponseInfo);
	void __fastcall Timer1Timer(TObject *Sender);
	void __fastcall timer_connTimer(TObject *Sender);

private: // User declarations
	// TJsonWriter *json_writer;

	// _di_IAsyncResult
public: // User declarations
	__fastcall TForm2(TComponent* Owner);
};

// ---------------------------------------------------------------------------
extern PACKAGE TForm2 *Form2;
// ---------------------------------------------------------------------------
#endif
