// dht11_reader.c  
// 編譯：gcc dht11_reader.c -lwiringPi -o dht11
// 執行：./dht11 --pin 7 --interval 2000 --json

#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>

#define MAXTIMINGS   85
#define DEFAULT_PIN  7     // 預設用 wiringPi pin 7 (對應 GPIO4)
#define DEFAULT_INT  2000  // 預設 2 秒讀一次
#define RETRIES      5     // 失敗最多重試 5 次

static volatile int g_running = 1;
static void on_sigint(int sig){ (void)sig; g_running = 0; }

// 儲存讀到的溫濕度
typedef struct {
    float humidity;   // 濕度 %
    float temp_c;     // 溫度 °C
} dht11_t;

// 實際讀一次 DHT11
static int dht11_read_once(int wpi_pin, dht11_t *out){
    int data[5] = {0,0,0,0,0};
    uint8_t laststate = HIGH;
    uint8_t counter = 0;
    uint8_t j = 0, i;

    // 發啟動訊號（先拉低一段時間）
    pinMode(wpi_pin, OUTPUT);
    digitalWrite(wpi_pin, LOW);
    delay(18);

    // 拉高再切換成輸入，等 DHT11 回應
    digitalWrite(wpi_pin, HIGH);
    delayMicroseconds(40);
    pinMode(wpi_pin, INPUT);

    // 開始讀資料位元（DHT11 總共傳 40 bits）
    for (i = 0; i < MAXTIMINGS; i++) {
        counter = 0;
        while (digitalRead(wpi_pin) == laststate) {
            counter++;
            delayMicroseconds(1);
            if (counter == 255) break;
        }
        laststate = digitalRead(wpi_pin);
        if (counter == 255) break;

        // 前面幾個信號不用，之後才是資料
        if ((i >= 4) && (i % 2 == 0)) {
            data[j/8] <<= 1;
            if (counter > 16) data[j/8] |= 1; // 高電平時間長 = 1
            j++;
        }
    }

    // 如果資料位數不夠，表示讀失敗
    if (j < 40) return -2;

    // 檢查最後一位校驗碼，驗證資料正確
    if ((data[4] != ((data[0]+data[1]+data[2]+data[3]) & 0xFF))) return -3;

    // 轉換數值（DHT11 溫度跟濕度基本上是整數）
    float h = (float)data[0] + (float)data[1]/10.0f;
    float c = (float)data[2] + (float)data[3]/10.0f;
    if (data[2] > 127) c = -c; // 處理負溫度

    out->humidity = h;
    out->temp_c   = c;
    return 0;
}

// 包一層，失敗會重試幾次
static int dht11_read_with_retry(int pin, dht11_t *out){
    for (int k=0; k<RETRIES; ++k){
        int rc = dht11_read_once(pin, out);
        if (rc == 0) return 0;
        delay(100); // 等一下再讀
    }
    return -1;
}

// 顯示使用方式
static void print_usage(const char *prog){
    fprintf(stderr,
        "用法: %s [--pin <wpiPin>] [--interval <ms>] [--json]\n"
        "預設 pin=7 (GPIO4), interval=2000ms\n", prog);
}

int main(int argc, char **argv){
    int pin = DEFAULT_PIN;
    int interval_ms = DEFAULT_INT;
    int json = 0;

    // 處理參數
    for (int i=1; i<argc; ++i){
        if (!strcmp(argv[i], "--pin") && i+1<argc) { pin = atoi(argv[++i]); }
        else if (!strcmp(argv[i], "--interval") && i+1<argc) { interval_ms = atoi(argv[++i]); }
        else if (!strcmp(argv[i], "--json")) { json = 1; }
        else { print_usage(argv[0]); return 2; }
    }

    // 初始化 wiringPi
    if (wiringPiSetup() == -1) {
        fprintf(stderr, "wiringPi 初始化失敗\n");
        return 1;
    }
    // 支援 Ctrl+C 結束
    signal(SIGINT, on_sigint);

    int ok_cnt = 0, err_cnt = 0;
    while (g_running){
        dht11_t val;
        int rc = dht11_read_with_retry(pin, &val);
        if (rc == 0){
            ok_cnt++;
            if (json){
                printf("{\"humidity\":%.1f,\"temp_c\":%.1f}\n", val.humidity, val.temp_c);
            } else {
                printf("濕度: %.1f %%  溫度: %.1f C\n", val.humidity, val.temp_c);
            }
            fflush(stdout);
        }else{
            err_cnt++;
            fprintf(stderr, "[WARN] 讀取失敗 (%d/%d)\n", err_cnt, ok_cnt);
        }
        delay(interval_ms);
    }

    fprintf(stderr, "結束：成功=%d 失敗=%d\n", ok_cnt, err_cnt);
    return (ok_cnt>0)? 0 : 3;
}

