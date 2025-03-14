#include "common_task.h"
#include "lib.h"
#include "debug.h"
#include <Arduino.h>
#include <Ticker.h>

Ticker blinker1;
Ticker blinker2;
Ticker blinker3;

namespace cmn_task
{
#ifdef CONFIG_IDF_TARGET_ESP32S3
    void IRAM_ATTR blink()
    {
        digitalWrite(led::LED, !digitalRead(led::LED));
    }
    void IRAM_ATTR blink_pitot()
    {
        digitalWrite(led::LED_PITOT, !digitalRead(led::LED_PITOT));
    }
    void IRAM_ATTR blink_sd()
    {
        digitalWrite(led::LED_SD, !digitalRead(led::LED_SD));
    }

    // LEDを点滅させる
    // LEDには1or２or3を指定可能で2と3はそれぞれpitotとsd用、blinkIntervalはms指定で0だと連続点灯になる
    void blinkLED_start(int led, int blinkInterval)
    {
        switch (led)
        {
        case 1:
            if (blinker1.active())
                blinkLED_stop(led);
            if (!blinkInterval)
            {
                digitalWrite(led::LED, HIGH);
                break;
            }
            blinker1.attach_ms(blinkInterval, blink);
            break;
        case 2:
            if (blinker2.active())
                blinkLED_stop(led);
            if (!blinkInterval)
            {
                digitalWrite(led::LED_PITOT, HIGH);
                break;
            }
            blinker2.attach_ms(blinkInterval, blink_pitot);
            break;
        case 3:
            if (blinker3.active())
                blinkLED_stop(led);
            if (!blinkInterval)
            {
                digitalWrite(led::LED_SD, HIGH);
                break;
            }
            blinker3.attach_ms(blinkInterval, blink_sd);
            break;
        default:
            pr_debug("wrong led name specified!!!");
            break;
        }
    }

    // ledの点滅をやめる
    void blinkLED_stop(int led)
    {
        if (led == 1)
        {
            if (blinker1.active())
                blinker1.detach();
            digitalWrite(led::LED, LOW);
        }
        else if (led == 2)
        {
            if (blinker2.active())
                blinker2.detach();
            digitalWrite(led::LED_PITOT, LOW);
        }
        else if (led == 3)
        {
            if (blinker3.active())
                blinker3.detach();
            digitalWrite(led::LED_SD, LOW);
        }
        else
        {
            pr_debug("Wrong led name specified!!!")
        }
    }
#elif CONFIG_IDF_TARGET_ESP32
    void IRAM_ATTR blink1()
    {
        digitalWrite(led::LED1, !digitalRead(led::LED1));
    }

    void IRAM_ATTR blink2()
    {
        digitalWrite(led::LED2, !digitalRead(led::LED2));
    }

    // LEDを点滅させる
    // LEDには1or2を指定可能で、blinkIntervalはms指定で0だと連続点灯となる。
    void blinkLED_start(int led, int blinkInterval)
    {
        if (led == 1)
        {

            if (blinker1.active())
                blinkLED_stop(led);
            if (!blinkInterval)
            {
                digitalWrite(led::LED1, HIGH);
                return;
            }
            blinker1.attach_ms(blinkInterval, blink1);
        }
        else if (led == 2)
        {

            if (blinker2.active())
                blinkLED_stop(led);
            if (!blinkInterval)
            {
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
            if (blinker1.active())
                blinker1.detach();
            digitalWrite(led::LED1, LOW);
        }
        else if (led == 2)
        {
            if (blinker2.active())
                blinker2.detach();
            digitalWrite(led::LED2, LOW);
        }
        else
        {
            pr_debug("Wrong led name specified!!!")
        }
    }
#endif

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

        int offset = 0;

        for (int i = 0; i < numof_maxData; ++i)
        {
            // snprintfを使って1行をフォーマット
            int written = snprintf(
                buffer + offset,
                AllbufferSize - offset,
                "%14lld,%8g,%8g\n",
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