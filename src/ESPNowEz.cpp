#include "ESPNowEz.h"

CESPNowEZ::CESPNowEZ(uint8_t id)
{
  id_ = id;
  deviceId_ = 0;
  
  for(int i = 0; i < MAC_ADDR_LEN; i++){
    ownMacAddr_[i] = 0;
    controllerMacAddr_[i] = 0;
  }
  esp_read_mac(ownMacAddr_, ESP_MAC_WIFI_STA); // これがBASEアドレス

  sprintf(ownMacAddrChar_, "%02x:%02x:%02x:%02x:%02x:%02x", ownMacAddr_[0], ownMacAddr_[1], ownMacAddr_[2], ownMacAddr_[3], ownMacAddr_[4], ownMacAddr_[5]);
}

uint8_t CESPNowEZ::Initialize(esp_now_recv_cb_t funcRecv, esp_now_send_cb_t funcSent)
{
  WiFi.mode(WIFI_STA);

  // ESP_OK : succeed
  // ESP_ERR_ESPNOW_INTERNAL : internal error
  esp_err_t initErr = esp_now_init();

  if(initErr != ESP_OK){
    return ERR_INITIALIZE_FAIL;
  }

  uint8_t err = 0;

  // 受信用コールバック関数の登録
  if(funcRecv != nullptr)
  {
    // ESP_OK : succeed
    // ESP_ERR_ESPNOW_NOT_INIT : ESPNOW is not initialized
    // ESP_ERR_ESPNOW_INTERNAL : internal error
    if(esp_now_register_recv_cb(esp_now_recv_cb_t(funcRecv)) != ESP_OK)
    {
      err |= ERR_RECV_CALLBACK_REGISTER_FAIL;
    }
  }

  // 送信用コールバック関数の登録
  if(funcSent != nullptr)
  {
    // ESP_OK : succeed
    // ESP_ERR_ESPNOW_NOT_INIT : ESPNOW is not initialized
    // ESP_ERR_ESPNOW_INTERNAL : internal error
    if(esp_now_register_send_cb(esp_now_send_cb_t(funcSent)) != ESP_OK)
    {
      err |= ERR_SEND_CALLBACK_REGISTER_FAIL;
    }
  }
  
  return err;
}

// 動作はSetControllerMacAddr(const uint8_t* macAddr)を見てね
esp_err_t CESPNowEZ::SetControllerMacAddr(uint8_t macAddr0, uint8_t macAddr1, uint8_t macAddr2, uint8_t macAddr3, uint8_t macAddr4, uint8_t macAddr5)
{
  uint8_t tmp[MAC_ADDR_LEN] = {macAddr0, macAddr1, macAddr2, macAddr3, macAddr4, macAddr5};
  return SetControllerMacAddr(tmp);
}

esp_err_t CESPNowEZ::SetControllerMacAddr(const uint8_t* macAddr)
{
  esp_now_peer_info_t peerInfo = {};

  memcpy(controllerMacAddr_, macAddr, MAC_ADDR_LEN);
  memcpy(peerInfo.peer_addr, macAddr, MAC_ADDR_LEN);
  peerInfo.channel = ESPNOW_CHANNEL;  
  peerInfo.encrypt = false; // 暗号化はしない
  
  // ESP_OK : succeed
  // ESP_ERR_ESPNOW_NOT_INIT : ESPNOW is not initialized
  // ESP_ERR_ESPNOW_ARG : invalid argument
  // ESP_ERR_ESPNOW_FULL : peer list is full
  // ESP_ERR_ESPNOW_NO_MEM : out of memory
  // ESP_ERR_ESPNOW_EXIST : peer has existed
  return esp_now_add_peer(&peerInfo);
}

esp_err_t CESPNowEZ::SetDeviceMacAddr(uint8_t macAddr0, uint8_t macAddr1, uint8_t macAddr2, uint8_t macAddr3, uint8_t macAddr4, uint8_t macAddr5)
{
  uint8_t tmp[MAC_ADDR_LEN] = {macAddr0, macAddr1, macAddr2, macAddr3, macAddr4, macAddr5};
  return SetDeviceMacAddr(tmp);
}

esp_err_t CESPNowEZ::SetDeviceMacAddr(const uint8_t* macAddr)
{
  if(deviceId_ >= MAX_DEVICE_NUM)
  {
    return ESP_ERR_ESPNOW_NO_MEM; // 20個より多く登録しようとする場合はこのエラーを返すのが良いだろう
  }

  esp_now_peer_info_t peerInfo = {};

  memcpy(deviceMacAddr_[deviceId_], macAddr, MAC_ADDR_LEN);
  memcpy(peerInfo.peer_addr, macAddr, MAC_ADDR_LEN);

  peerInfo.channel = ESPNOW_CHANNEL;  
  peerInfo.encrypt = false; // 暗号化はしない
  
  // ESP_OK : succeed
  // ESP_ERR_ESPNOW_NOT_INIT : ESPNOW is not initialized
  // ESP_ERR_ESPNOW_ARG : invalid argument
  // ESP_ERR_ESPNOW_FULL : peer list is full
  // ESP_ERR_ESPNOW_NO_MEM : out of memory
  // ESP_ERR_ESPNOW_EXIST : peer has existed

  esp_err_t err = esp_now_add_peer(&peerInfo);
  if(err == ESP_OK)
  {
    deviceId_++;
  }

  return err;
}

esp_err_t CESPNowEZ::Send(ESPNOW_Dev2ConData* data, int dataSize)
{
  // ESP_OK : succeed
  // ESP_ERR_ESPNOW_NOT_INIT : ESPNOW is not initialized
  // ESP_ERR_ESPNOW_ARG : invalid argument
  // ESP_ERR_ESPNOW_INTERNAL : internal error
  // ESP_ERR_ESPNOW_NO_MEM : out of memory, when this happens, you can delay a while before sending the next data
  // ESP_ERR_ESPNOW_NOT_FOUND : peer is not found
  // ESP_ERR_ESPNOW_IF : current Wi-Fi interface doesn't match that of peer
  // ESP_ERR_ESPNOW_CHAN: current Wi-Fi channel doesn't match that of peer
  // ※ ESP_ERR_ESPNOW_CHANは公式ドキュメントに載っているがビルドするとnot declaredになる（20240819時点）
  return esp_now_send(controllerMacAddr_, (uint8_t *) data, dataSize);
}

esp_err_t CESPNowEZ::Send(uint8_t id, ESPNOW_Con2DevData* data, int dataSize)
{
  int index = id - 1;
  if(index >= 0 && index < MAX_DEVICE_NUM )
  {
    return esp_now_send(deviceMacAddr_[index], (uint8_t *) data, dataSize);
  }
  else
  {
    return ESP_ERR_ESPNOW_INTERNAL;
  }
}
