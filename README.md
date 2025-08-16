# DHT11 Reader (C 語言版本)

使用 Raspberry Pi 與 **C 語言 (wiringPi 函式庫)** 撰寫的 DHT11 溫濕度感測器讀取程式。  
支援命令列參數設定 GPIO 腳位、讀取間隔，並可輸出為 JSON 格式，方便與其他系統整合。

---

## 📌 功能特色
- 使用 **C 語言** 直接操作 GPIO，依 DHT11 時序協定讀取資料
- 支援 **錯誤重試機制**（預設最多 5 次）
- 可選擇輸出格式：
  - 預設：中文輸出
  - `--json`：JSON 格式，方便串接資料庫 / MQTT
- **命令列參數**
  - `--pin <wpiPin>`：指定 wiringPi 腳位（預設 7 = GPIO4）
  - `--interval <ms>`：讀取間隔（毫秒，預設 2000ms）
  - `--json`：啟用 JSON 輸出
- **優雅退出**：支援 `Ctrl+C` 結束，會統計成功與失敗次數

---

## 🛠️ 硬體需求
- Raspberry Pi (任何支援 GPIO 的型號，例如 Pi 3 / 4)
- DHT11 溫濕度感測器
- 接線方式：
  - VCC → 3.3V
  - GND → GND
  - DATA → GPIO4 (對應 wiringPi pin 7)

---

## 📦 安裝與編譯與執行

先安裝 wiringPi（如果系統沒有內建）：
```bash
sudo apt-get update
sudo apt-get install wiringpi

## 📦 編譯

```bash
gcc dht11_reader.c -lwiringPi -o dht11


##    執行
./dht11 --pin 7 --interval 2000 --json
