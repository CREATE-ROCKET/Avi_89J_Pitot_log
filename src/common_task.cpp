#include "common_task.h"
#include "lib.h"
#include "debug.h"
#include <Arduino.h>
#include <Ticker.h>

Ticker blinker1;
Ticker blinker2;

namespace cmn_task
{
    void blink1()
    {
        digitalWrite(led::LED1, !digitalRead(led::LED1));
    }

    void blink2()
    {
        digitalWrite(led::LED2, !digitalRead(led::LED2));
    }

    // LEDを点滅させる
    // ledには1or2を指定可能で、blinkIntervalはms指定で0だと連続点灯となる。
    void blinkLED_start(int led, int blinkInterval)
    {
        if (led == 1)
        {
            if (!blinkInterval)
            {
                blinkLED_stop(led);
                digitalWrite(led::LED1, HIGH);
                return;
            }
            blinker1.attach_ms(blinkInterval, blink1);
        }
        else if (led == 2)
        {
            if (!blinkInterval)
            {
                blinkLED_stop(led);
                digitalWrite(led::LED2, HIGH);
                return;
            }
            blinker2.attach_ms(blinkInterval, blink2);
        }
        else
        {
            pr_debug("Wrong led name specified!!!");
        }
    }

    // LEDの点滅を停止させる
    void blinkLED_stop(int led)
    {
        if (led == 1)
        {
            blinker1.detach();
            digitalWrite(led::LED1, LOW);
        }
        else if (led == 2)
        {
            blinker2.detach();
            digitalWrite(led::LED2, LOW);
        }
        else
        {
            pr_debug("Wrong led name specified!!!")
        }
    }

    // Data型の配列をchar型に変換する
    // newしているのでdelete[]等必要
    char *DataToChar(Data pitotData[numof_maxData])
    {
        // バッファを動的に確保
        char *buffer = new char[AllbufferSize];
        if (buffer == nullptr)
        {
            return nullptr; // メモリ確保失敗時はnullptrを返す
        }

        size_t offset = 0;

        for (int i = 0; i < numof_maxData; ++i)
        {
            // snprintfを使って1行をフォーマット
            int written = snprintf(
                buffer + offset,
                bufferSize,
                "%u, %g, %g\n",
                pitotData[i].time,
                pitotData[i].pa,
                pitotData[i].temp);

            // バッファオーバーフローを防止
            if (written < 0 || offset + written >= AllbufferSize)
            {
                delete[] buffer; // メモリを解放して失敗を返す
                return nullptr;
            }

            offset += written;
        }
        return buffer;
    }

}