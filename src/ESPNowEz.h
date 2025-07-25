#ifndef __ESPNOWEZ_H_

#define __ESPNOWEZ_H_

#include <esp_now.h>
#include <WiFi.h>

#include "ESPNOW_SendData.h"

#define ESPNOW_CHANNEL 0

#define ERR_INITIALIZE_FAIL 1
#define ERR_RECV_CALLBACK_REGISTER_FAIL 2
#define ERR_SEND_CALLBACK_REGISTER_FAIL 4

#define MAX_DEVICE_NUM 20

#define MAC_ADDR_LEN 6
#define OWN_MAC_ADDR_CHAR_LEN 18

class CESPNowEZ
{
  uint8_t id_;
  uint8_t deviceId_; // デバイス用のID、デバイスに送信しない場合は使わない

  uint8_t controllerMacAddr_[MAC_ADDR_LEN];
  uint8_t ownMacAddr_[MAC_ADDR_LEN];

  uint8_t deviceMacAddr_[MAX_DEVICE_NUM][MAC_ADDR_LEN];

  char ownMacAddrChar_[OWN_MAC_ADDR_CHAR_LEN]; // xx:xx:xx:xx:xx:xx

public:
  CESPNowEZ(uint8_t id);

  uint8_t Initialize(esp_now_recv_cb_t funcRecv, esp_now_send_cb_t funcSent = nullptr);

  const char* GetMacAddrChar() { return ownMacAddrChar_; };
  int ID() { return id_; };

  esp_err_t SetControllerMacAddr(uint8_t macAddr0, uint8_t macAddr1, uint8_t macAddr2, uint8_t macAddr3, uint8_t macAddr4, uint8_t macAddr5);
  esp_err_t SetControllerMacAddr(const uint8_t* macAddr);

  // 登録順に1、2、3、…とidが割り当てられる
  esp_err_t SetDeviceMacAddr(uint8_t macAddr0, uint8_t macAddr1, uint8_t macAddr2, uint8_t macAddr3, uint8_t macAddr4, uint8_t macAddr5);
  esp_err_t SetDeviceMacAddr(const uint8_t* macAddr);

  esp_err_t Send(ESPNOW_Dev2ConData* data, int dataSize);
  esp_err_t Send(uint8_t id, ESPNOW_Con2DevData* data, int dataSize);

};

#endif