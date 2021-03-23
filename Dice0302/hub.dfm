object DataModule1: TDataModule1
  OldCreateOrder = False
  OnCreate = DataModuleCreate
  OnDestroy = DataModuleDestroy
  Height = 519
  Width = 779
  object MySQLUniProvider1: TMySQLUniProvider
    Left = 96
    Top = 48
  end
  object UniConnection1: TUniConnection
    IOHandler = CRSSHIOHandler1
    Left = 216
    Top = 48
  end
  object UniScript1: TUniScript
    Connection = UniConnection1
    Left = 312
    Top = 48
  end
  object UniQuery1: TUniQuery
    Connection = UniConnection1
    Left = 392
    Top = 48
  end
  object UniQuery2: TUniQuery
    Connection = UniConnection1
    Left = 488
    Top = 48
  end
  object UniConnection2: TUniConnection
    IOHandler = CRSSHIOHandler1
    Left = 216
    Top = 128
  end
  object UniQuery3: TUniQuery
    Connection = UniConnection2
    Left = 304
    Top = 128
  end
  object IdHTTPServer1: TIdHTTPServer
    Bindings = <>
    OnCommandGet = IdHTTPServer1CommandGet
    Left = 680
    Top = 72
  end
  object ScMemoryStorage1: TScMemoryStorage
    Left = 136
    Top = 240
  end
  object ScSSHClient1: TScSSHClient
    KeyStorage = ScMemoryStorage1
    OnServerKeyValidate = ScSSHClient1ServerKeyValidate
    Left = 248
    Top = 240
  end
  object CRSSHIOHandler1: TCRSSHIOHandler
    Client = ScSSHClient1
    Left = 376
    Top = 240
  end
  object timer_sweeper: TTimer
    Interval = 300000
    OnTimer = timer_sweeperTimer
    Left = 528
    Top = 312
  end
end
