// GroupB用コントローラ

#include <Arduino.h>
#include <M5Stack.h>

#include "ESPNowEz.h"

#define LGFX_AUTODETECT
#include <LovyanGFX.hpp>
#include <LGFX_AUTODETECT.hpp>

#define UNITY_SEND_TIMING 10 // 10ms、Unityに送るタイミング
#define HEART_BEAT_TIMING 1000 // 1000ms
#define TIMER_RESET_TIMING 60000 // 1min. = 60000ms

CESPNowEZ espnow(0); // コントローラは必ず0

// 2025/02/19現在3個使用
uint8_t device1MacAddr[] = { 0xa0, 0xdd, 0x6c, 0x67, 0x88, 0x10 }; // ID1
uint8_t device2MacAddr[] = { 0xa0, 0xdd, 0x6c, 0x69, 0xad, 0xf4 }; // ID2
uint8_t device3MacAddr[] = { 0xa0, 0xdd, 0x6c, 0x67, 0x87, 0xf8 }; // ID3
uint8_t device4MacAddr[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // ID4
uint8_t device5MacAddr[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // ID5
ESPNOW_Con2DevData controllerData; // コントローラ→デバイスのデータ
ESPNOW_Dev2ConData deviceData[5]; // デバイス→コントローラのデータ、デバイス5台分

static LGFX screen;
static LGFX_Sprite canvas(&screen);

int outputFlag = 0; // シリアルモニタで出力するためのフラグ
char outputStr[100];
char drawStr[100];
char messageStr[100];

char MACAddrStr[5][18]; // デバイス5台、xx:xx:xx:xx:xx:xx + '\0'

char toUnityData[8]; // スイッチ5個、例えばS11111Eが送られる
uint8_t targetID;

void Draw();
void OnDataReceived(const uint8_t* mac_addr, const uint8_t* data, int data_len);
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void IRAM_ATTR onTimer();

void ConvertNum2Hex(char* str, uint8_t value);
void MakeMACAddrStr();

void UnitySend();
void HeartBeatProcess();

int h2d(char h);

// タイマーはデバッグ用
hw_timer_t* timer = nullptr;
uint32_t timerCount = 0;
bool debugSendQueue = false;
bool deviceSendQueue = false;
bool drawQueue = false;
bool unitySendQueue = false;
bool heartBeatQueue_ = false;

uint16_t heartBeat_ = 0;

// デバッグ用変数
uint32_t debugCount = 0;

void setup() {
  // put your setup code here, to run once:
  M5.begin(); // 呼ばないと画面の大きさ等が正しく取得できない
  M5.Power.begin();

  // LCD関連
  screen.init();
  screen.setRotation(1);
  canvas.setColorDepth(8);
  canvas.setTextWrap(false);
  canvas.setTextSize(1);
  canvas.createSprite(screen.width(), screen.height());

  Serial.begin(115200);
  //Serial.println(espnow.GetMacAddrChar());

  espnow.Initialize(OnDataReceived, OnDataSent); // 引数は受信用コールバック関数、送信用コールバック関数の順

  espnow.SetDeviceMacAddr(device1MacAddr);
  espnow.SetDeviceMacAddr(device2MacAddr);
  espnow.SetDeviceMacAddr(device3MacAddr);

  //randomSeed(analogRead(0)); // デバッグ用なので毎回同じ乱数列でもOK

  timer = timerBegin(0, getApbFrequency() / 1000000, true); // 1usでカウントアップ
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1000, true);  // 1000us=1msごとにonTimer関数が呼ばれる
  timerAlarmEnable(timer);

  // MACアドレスの文字列を生成
  MakeMACAddrStr();

  // Unityに送る命令を初期化しておく
  strncpy(toUnityData, "S11111E", sizeof(toUnityData));

  drawQueue = true;
}

void loop() {
  // put your main code here, to run repeatedly:
  // Unity -> Controller
  if(Serial.available() > 0)
  {
    char inputChar = Serial.read();
    // コマンドはcxRRGGBBm、またはbxddfffvvの9文字
    if(inputChar == 'c')
    {
      char tmp[2];

      // 残り8字
      // ID
      tmp[0] = Serial.read();
      uint8_t id = tmp[0] - '0';

      // Red
      tmp[1] = Serial.read();
      tmp[0] = Serial.read();
      int r = h2d(tmp[0]) * 16 + h2d(tmp[1]);
      //r /= 8; // 8ビットから5ビットに落とす：自動でやってくれるようで不要だった
      
      // Green
      tmp[1] = Serial.read();
      tmp[0] = Serial.read();
      int g = h2d(tmp[0]) * 16 + h2d(tmp[1]);
      //g /= 4; // 8ビットから6ビットに落とす：自動でやってくれるようで不要だった

      // Blue
      tmp[1] = Serial.read();
      tmp[0] = Serial.read();
      int b = h2d(tmp[0]) * 16 + h2d(tmp[1]);
      //b /= 8; // 8ビットから5ビットに落とす：自動でやってくれるようで不要だった

      // 表示するメッセージ
      tmp[0] = Serial.read();
      uint8_t result = tmp[0] - '0';

      snprintf(messageStr, sizeof(messageStr), "%d %d %d %d %d", id, r, g, b, result);

      targetID = id; // 1～5

      controllerData.id = espnow.ID();
      controllerData.updateFlag = COLOR_UPDATE;
      controllerData.led[0] = r;
      controllerData.led[1] = g;
      controllerData.led[2] = b;
      controllerData.result = result;

      // 値を変更する必要はないが、とりあえず0にしておいた
      controllerData.soundFolderNo = 0;
      controllerData.soundFileNo = 0;
      controllerData.soundVol = 0;

      drawQueue = true;
      deviceSendQueue = true;
    }
    else if(inputChar == 'b')
    {
      char tmp[3];

      // 残り8字
      // ID
      tmp[0] = Serial.read();
      uint8_t id = tmp[0] - '0';

      // フォルダ番号
      tmp[1] = Serial.read();
      tmp[0] = Serial.read();
      int folderNum = (tmp[1] - '0') * 10 + (tmp[0] - '0');
      
      // ファイル番号
      tmp[2] = Serial.read();
      tmp[1] = Serial.read();
      tmp[0] = Serial.read();
      int fileNum = (tmp[2] - '0') * 100 + (tmp[1] - '0') * 10 + (tmp[0] - '0');

      // ボリューム
      tmp[1] = Serial.read();
      tmp[0] = Serial.read();
      int vol = (tmp[1] - '0') * 10 + (tmp[0] - '0');

      snprintf(messageStr, sizeof(messageStr), "%d %d %d %d", id, folderNum, fileNum, vol);

      targetID = id; // 1～5

      controllerData.id = espnow.ID();
      controllerData.updateFlag = SOUND_UPDATE;
      controllerData.led[1] = 0; // 値を変更する必要はないが、とりあえず0にしておいた
      controllerData.led[2] = 0; // 値を変更する必要はないが、とりあえず0にしておいた
      controllerData.led[0] = 0; // 値を変更する必要はないが、とりあえず0にしておいた

      controllerData.soundFolderNo = folderNum;
      controllerData.soundFileNo = fileNum;
      controllerData.soundVol = vol;

      drawQueue = true;
      deviceSendQueue = true;
    }
  }

  if(unitySendQueue)
  {
    unitySendQueue = false;

    UnitySend();
  }

  if(deviceSendQueue)
  {
    deviceSendQueue = false;

    espnow.Send(targetID, &controllerData, sizeof(controllerData));
    drawQueue = true;
  }

  if(heartBeatQueue_)
  {
    heartBeatQueue_ = false;
    HeartBeatProcess();
  }

  if(debugSendQueue)
  {
    debugSendQueue = false;

    controllerData.id = espnow.ID();
    controllerData.led[0] = random(0, 256);
    controllerData.led[1] = random(0, 256);
    controllerData.led[2] = random(0, 256);
    controllerData.soundFolderNo = random(1, 3);
    if(controllerData.soundFolderNo == 1)
      controllerData.soundFileNo = 1;
    else
      controllerData.soundFileNo = random(2, 4);
    controllerData.soundVol = random(1, 10);
    
    espnow.Send(1, &controllerData, sizeof(controllerData));
    espnow.Send(2, &controllerData, sizeof(controllerData));
    espnow.Send(3, &controllerData, sizeof(controllerData));
    drawQueue = true;
  }

  if(outputFlag > 0)
  {
    int index = outputFlag - 1; // デバイスから送られるidの最小値は1
    //snprintf(outputStr, sizeof(outputStr), "%d %d", deviceData[index].id, deviceData[index].sw);
    //Serial.println(outputStr);

    outputFlag = 0;
  }

  if(drawQueue)
  {
    drawQueue = false;

    Draw();
    canvas.pushSprite(0, 0);
  }
}

void OnDataReceived(const uint8_t* mac_addr, const uint8_t* data, int data_len)
{
  // 受信時の処理
  ESPNOW_Dev2ConData tmp;
  memcpy(&tmp, data, data_len);
  uint8_t id = tmp.id;
  uint8_t index = tmp.id - 1;
  deviceData[index] = tmp; // デバイスから送られるidの最小値は1
  outputFlag = 1;

  // デバイス1のデータはdeviceData[0]、デバイス1のスイッチ入力はtoUnityData[1]
  if(deviceData[index].sw)
    toUnityData[id] = '1';
  else
    toUnityData[id] = '0';
  
  snprintf(messageStr, sizeof(messageStr), "dev%d, sw:%d", id, deviceData[index].sw);
  drawQueue = true;
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  // 送信完了時の処理
}

void IRAM_ATTR onTimer()
{
  if(timerCount % UNITY_SEND_TIMING == 0)
  {
    unitySendQueue = true;
  }

  if(timerCount % HEART_BEAT_TIMING == 0)
  {
    heartBeatQueue_ = true;
  }

  timerCount++;
  if(timerCount == TIMER_RESET_TIMING) // 60000で1分、3600000で1時間
  {
    timerCount = 0;
  }
}

void Draw()
{
  canvas.fillScreen(TFT_NAVY);

  canvas.setTextColor(TFT_WHITE);
  canvas.setFont(&Font4); // 幅14高さ26
  canvas.setTextSize(1);  // 倍率1

  for(int id = 0; id < 5; id++)
  {
    canvas.setCursor(0, 30 * id, &Font4);
    canvas.print(MACAddrStr[id]);
  }

  canvas.setCursor(0, 200, &Font4);
  canvas.print(messageStr);

  canvas.setCursor(290, 232, &Font0);
  canvas.printf("%5d", heartBeat_);
}

void MakeMACAddrStr()
{
  for(int id = 0; id < 5; id++)
  {
    uint8_t* macAddr = nullptr;
    switch (id)
    {
    case 0:
      macAddr = device1MacAddr;
      break;
    case 1:
      macAddr = device2MacAddr;
      break;
    case 2:
      macAddr = device3MacAddr;
      break;
    case 3:
      macAddr = device4MacAddr;
      break;
    case 4:
      macAddr = device5MacAddr;
      break;
    // default: は不要
    }

    //char MACAddrStr[5][18]; // デバイス5台、xx:xx:xx:xx:xx:xx
    for(int i = 0; i < 6; i++) // 6はMACアドレスの6つの数字
    {
      ConvertNum2Hex(&MACAddrStr[id][3 * i], macAddr[i]); // 3 * iは「xx:」の3文字
      MACAddrStr[id][3 * i + 2] = ':';
    }
    MACAddrStr[id][17] = '\0'; // 最後の「:」は「\0」で置き換え 
  }
}

void ConvertNum2Hex(char* str, uint8_t value)
{
  int digit1 = value % 16;
  int digit16 = value / 16;

  // 16^1から格納
  if(digit16 >= 10)
    str[0] = 'A' + (digit16 - 10); 
  else
    str[0] = '0' + digit16;

  if(digit1 >= 10)
    str[1] = 'A' + (digit1 - 10); 
  else
    str[1] = '0' + digit1;
}

int h2d(char h)
{
  if(h >= 'A' && h <= 'F')
  {
    return h - 'A' + 10;
  }
  else if(h >= '0' && h <= '9')
  {
    return h - '0';
  }
  else
  {
    return 0; // 意味不明文字は0を返す
  }
}

void UnitySend()
{
  Serial.println(toUnityData);
}

void HeartBeatProcess()
{
  heartBeat_++;
  drawQueue = true;
}
