object Form2: TForm2
  Left = 0
  Top = 0
  Caption = 'PipeSrv'
  ClientHeight = 437
  ClientWidth = 985
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -13
  Font.Name = 'Tahoma'
  Font.Style = []
  Font.Quality = fqClearTypeNatural
  OldCreateOrder = False
  Position = poDesktopCenter
  OnClose = FormClose
  OnCreate = FormCreate
  OnDestroy = FormDestroy
  DesignSize = (
    985
    437)
  PixelsPerInch = 96
  TextHeight = 16
  object Label1: TLabel
    Left = 756
    Top = 14
    Width = 91
    Height = 16
    Anchors = [akTop, akRight]
    Caption = 'HTTP'#31471#21475#21495#65306
  end
  object Button1: TButton
    Left = 16
    Top = 8
    Width = 75
    Height = 25
    Caption = #21551#21160#26381#21153
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = []
    Font.Quality = fqClearTypeNatural
    ParentFont = False
    TabOrder = 0
    OnClick = Button1Click
  end
  object Button2: TButton
    Left = 104
    Top = 8
    Width = 75
    Height = 25
    Caption = #20572#27490#26381#21153
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = []
    Font.Quality = fqClearTypeNatural
    ParentFont = False
    TabOrder = 1
    OnClick = Button2Click
  end
  object StatusBar1: TStatusBar
    Left = 0
    Top = 418
    Width = 985
    Height = 19
    Panels = <>
  end
  object Memo1: TMemo
    Left = 16
    Top = 39
    Width = 953
    Height = 372
    Anchors = [akLeft, akTop, akRight, akBottom]
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = []
    Font.Quality = fqClearTypeNatural
    Lines.Strings = (
      '')
    ParentFont = False
    ScrollBars = ssVertical
    TabOrder = 3
  end
  object Edit1: TEdit
    Left = 848
    Top = 9
    Width = 121
    Height = 24
    Anchors = [akTop, akRight]
    TabOrder = 4
    Text = '80'
  end
  object NetHTTPClient1: TNetHTTPClient
    Asynchronous = False
    ConnectionTimeout = 60000
    ResponseTimeout = 60000
    AllowCookies = True
    HandleRedirects = True
    UserAgent = 'Embarcadero URI Client/1.0'
    Left = 280
    Top = 65528
  end
  object NetHTTPRequest1: TNetHTTPRequest
    Asynchronous = True
    ConnectionTimeout = 60000
    ResponseTimeout = 60000
    Client = NetHTTPClient1
    Left = 320
    Top = 65528
  end
  object IdHTTPServer1: TIdHTTPServer
    Bindings = <>
    DefaultPort = 52727
    UseNagle = False
    TerminateWaitTime = 100
    OnCommandGet = IdHTTPServer1CommandGet
    Left = 360
    Top = 65528
  end
  object Timer1: TTimer
    Interval = 3600000
    OnTimer = Timer1Timer
    Left = 240
    Top = 65528
  end
  object timer_conn: TTimer
    Enabled = False
    OnTimer = timer_connTimer
    Left = 440
    Top = 65528
  end
end
