
// 送信するデータ形式（構造体）を定義
// このファイルはESPNowEz.hと同一フォルダにあること
// ArduinoIDEではスケッチファイルより上位の階層にあるファイルはincludeできない制約があるため、ここで定義
// 一意であるべき定義が書かれたファイルが通信対象のスケッチがある場所にコピーされるが、現状ではやむを得ず

// プロジェクトごとに必要な構造体を作る
// Device側が複数存在する場合は、idを含めておくことを推奨

#ifndef __ESPNOW_SEND_DATA_H_

#define __ESPNOW_SEND_DATA_H_

#define COLOR_UPDATE 1
#define SOUND_UPDATE 2

typedef struct struct_esp_now_d2c_data {
    int8_t id; // 必ず被らないこと
    uint8_t sw;
} ESPNOW_Dev2ConData;

typedef struct struct_esp_now_c2d_data {
    uint8_t id; // 必ず被らないこと
    uint8_t updateFlag; // 0:更新なし、1:色のみ、2:音のみ、3:色・音
    uint8_t led[3];
    uint8_t soundFolderNo;
    uint8_t soundFileNo;
    uint8_t soundVol;
    int16_t score;
} ESPNOW_Con2DevData;

#endif