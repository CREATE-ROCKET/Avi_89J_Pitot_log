#include <Arduino.h>
#include "CANCREATE 1.0.0/CANCREATE.cpp"

// ピトー管のデータの保存するstruct
struct Data
{
    uint32_t time; // ESPタイマ初期化時からの経過時間 ms
    float pa;      // 圧力
    float temp;    // 温度
};

constexpr int numof_maxData = sizeof(Data) / sizeof(uint8_t);
// CANで得られたデータを変換する union
union PitotDataUnion
{
    Data pitotData;
    uint8_t Uint8Data[numof_maxData];
};

constexpr int CAN_RX = 21;
constexpr int CAN_TX = 22;

CAN_CREATE CAN(true);
bool is_receiving;
int received_data_counter;
PitotDataUnion pitot_data;

void setup()
{
    Serial.begin(115200);
    if (CAN.begin(100E3, CAN_RX, CAN_TX, 0))
    {
        Serial.println("failed to init can");
        while (true)
            ;
    }
    is_receiving = false;
}

void loop()
{
    if (CAN.available())
    {
        can_return_t Data;
        if (!CAN.readWithDetail(&Data))
        {
            if (Data.id == 2)
            { // idが2の場合 ピトー管のデータを送信している
                if (!is_receiving)
                { // データを送信中でない場合
                    if (Data.size == 1 &&
                        *(Data.data) == '<')
                    {
                        is_receiving = true;
                        received_data_counter = 0;
                        Serial.println("receiving data...");
                    }
                }
                else
                { // データの受信中
                    if (received_data_counter == 0 &&
                        Data.size == 8)
                    { // 最初のデータは8byte分あるはず
                        memcpy(pitot_data.Uint8Data, Data.data, 8);
                        received_data_counter++;
                    }
                    else if (received_data_counter == 1 &&
                             Data.size == 4)
                    { // 次のデータは4byte分あるはず
                        memcpy(pitot_data.Uint8Data + 8, Data.data, 4);
                        received_data_counter++;
                    }
                    else if (received_data_counter != 2 &&
                             Data.size == 1)
                    {
                        if (*(Data.data) == '>')
                        {
                            Serial.println("success to receive");
                            Serial.printf("time:%u ,pa: %g, temp: %g",
                                          pitot_data.pitotData.time,
                                          pitot_data.pitotData.pa,
                                          pitot_data.pitotData.temp);
                        }
                        is_receiving = false;
                    }
                    else
                        is_receiving = false;
                }
            }
            if (Data.id == 1)
            { // ピトー管基板の情報(生きてるかとか)用
                Serial.println("Display pitot log board info...");
                // TODO
            }
        }
    }
}