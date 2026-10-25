# 進階 UART 功能（1.0.1.022）

由主選單 **UART** 開啟 Offline replay、Profiles、Chart analysis 或 Automation。
原四個入口與新增 Extraction／Channels／File TX 共用可捲動的獨立視窗；主畫面仍是 Messages／Chart／4 Charts。
新解析、顯示、圖表操作、終端及檔案 TX 詳見 [UART 改善說明](UART_OPTIMIZATION_GUIDE.md)。
關閉進階視窗會暫停重播、停止自動化／檔案 TX 並要求取消檔案工作，不會關閉已連線的串口。
若檔案工作尚未結束，視窗會保留取消／等待訊息，完成後可再關閉；主視窗仍可要求關閉，不會在 UI thread 等待未完成的工作。

## 離線重播

先 Disconnect，停止 recording／export，再 Browse → Load。成功載入後停在暫停狀態；Play 才開始播放。
若現有 capture／Messages 有資料，載入前會提醒先 Save As。失敗或取消載入不取代原本資料。

- 完成錄製的 `.bin` 與相鄰同名 `.idx`：使用本工具記錄的 PC 收訊時間間隔，原始 bytes 不變。
  索引不吻合、未完整涵蓋 raw 檔或格式錯誤會拒絕，不偷偷改用假時間。編號 rotation segments 現在依數字順序一起載入，缺段會拒絕。
  PC 時鐘往回跳的間隔會夾為零；索引出現 input gap 會顯示警告。這不是 UART 線路／MCU 時間。
- `.log`／`.txt`／無索引 `.bin`：逐行／分塊，採 Synthetic interval（1..60000 ms）。
  不猜測檔案內時間戳格式，不將檔案內容傳送出去；UTF-8／ASCII telemetry 可沿用既有 parser。
  synthetic 來源會關閉 Messages 與其 Save As 的時鐘標籤，避免誤認為原始收訊時間。
- Speed：0.1..100 倍。Pause 保留位置；Seek 使用 0-based record 編號，清除目前檢視並暫停在該筆，Play 後繼續。
  Seek 不是還原整個舊 session；新 capture 的表格時間以定位後第一筆接受的資料重新起算。
- 背景分塊讀取／磁碟分頁索引取代舊版整份載入：合計最多 64 GiB／5000 萬排程 records／4096 segments，最多 4096 筆索引留在 RAM。
  每次背景預讀最多 4 個 record，可取消／跨檔搜尋／依時間定位；速度是盡力追上排程，不是精準即時儀器。
  Messages／capture 仍受既有保留上限限制；Load 不代表整份檔案會永久顯示在表格中。
- 數值 parser 遇到超過 64 KiB 的單行會略過該行直到 LF 並累計 Skipped，不把截斷後的尾端當成完整訊息。
  raw bytes 仍可在 Messages／HEX 檢視，受該頁保留上限限制。
- Unload 取消載入、清除離線檢視；沒有離線來源時不清除 live capture。離線來源載入期間不能 Connect 或啟用 live recording。

重播使用既有 Include／Exclude、資料表、Chart、4 Charts 與 Save As，不增加假的實體 RX／TX 統計，不寫入 live recording。

## 具名 Profile

名稱限 1..48 個英數、`_`、`-`，不接受 Windows 保留裝置名稱。檔案放在 EXE 旁的 `profiles/<name>.sccprofile`。

- Save named：儲存目前**已套用**的 filter／parser／Channel 顯示偏好、目前 serial 選項、4 Charts 欄位、Messages 顯示／換行／編碼／字型／wrap、啟動頁；workspace v2 也包含允許清單內的 B port、雙埠解析／pacing／framing／view、離線 binary schema、衍生公式、儀表與 XY／comparison 偏好。詳見 [分析功能說明](UART_ANALYSIS_GUIDE.md)。
- Load named／Import：背景讀取、驗證並暫存，顯示主要設定供確認，尚不改動目前功能。
- Export staged：將暫存的 Profile 匯出到自選位置。替換既有檔案需要確認。
- Apply staged：再次確認後套用；必須 disconnected、無 replay／recording／export。會重設 capture，啟動頁仍於下次啟動生效。
  Profile 的 port 是使用者明確選定的偏好，不表示本次連線成功；仍須按 Connect，並以目前 OS 列舉的裝置路徑開啟。

格式具版本與固定 allowlist，拒絕重複／未知欄位、無效數字／編碼／filter 與任意硬體開啟路徑。
舊版 v1 匯入最多 64 KiB、workspace v2 最多 512 KiB；INI 合併最多 1 MiB。Apply 在背景準備完整 INI，保留其他 key，再原子替換；期間鎖定主畫面設定輸入，進階視窗仍可關閉以要求取消。
取消若已越過儲存提交點，完成的儲存不會被倒轉；完成結果會反映在畫面。失敗不以部分 INI 取代原檔。

不包含 TX 歷史／Preset、命令、錄製路徑、log 或自動化啟動狀態。不自動連線、不傳送、不改變現場 pin。
Profile 可能仍含裝置 ID／私人 filter，並非加密；`.sccprofile` 已加入 Git ignore，分享前自行確認內容。

## 圖表分析

使用可搜尋的 Channel 選單選原 key，設定 Cursor A／B、View end 與 Window，按 Analyze。
分析使用**保留的 capture 快照**，不受即時 rolling window 限制，也不改動主畫面選取／軸線。

- 左鍵設 A、右鍵設 B，可持續拖曳或直接輸入秒數。A／B 是從該 capture 起點算的時間；數值採最近的保留樣本。
- 顯示 dT、dY、區間樣本數、min／max／平均／RMS／母體標準差。A..B 包含端點，缺值略過；空區間顯示 n/a，不補零。
- Reference Y 為選用水平參考線；在目前 Y 範圍內才繪出，不自動壓縮訊號以容納遠方的參考線。
- View end／Window 可定位到較早的保留資料。按 Analyze 更新快照；Clear cursors 只清除游標／參考線。

過去已被 retention 淘汰的資料無法分析；完整舊資料須從離線檔案重播到對應位置。

## 明確啟用的傳送自動化

必須先連線，再填寫並按 Start，確認目標與硬體風險。預設不啟動、不保存命令、不在重連後恢復。

| Mode | 行為 |
|---|---|
| Periodic | 單一命令，每次完成後等待 Interval，最多 Cycles 次 |
| Batch | 每行一個命令，依序傳送，每步間隔 Interval，整組重複 Cycles 次 |
| Auto reply | 單一回覆命令；收到 Trigger 後才送，受 cooldown 與最大回覆次數限制 |

Text 使用 literal UTF-8；HEX 與 Console 相同，無 `0x`。一行一個命令，空白行略過；最多 64 行。
Reply／Trigger 使用同一 Text／HEX 格式，為區分大小寫的原始 bytes 比對，可跨 read；不是 Regex／script，也不是語意上的協定 ACK。
回音或其他相同 bytes 可能命中。原始 RX 進入 worker 後比對，不受 Include／Exclude 限制。

- Checksum：None／Sum8／XOR8／CRC16-Modbus（初值 FFFF、反向多項式 A001、低位元組先送）。
  對**原始 payload** 計算後，以 binary bytes 附加，再加 None／CR／LF／CRLF；總長 1..4096 bytes。
  Sum8 是無號總和 modulo 256，XOR8 是逐 byte XOR；兩者不是 Modbus ASCII LRC。
- Interval／Timeout 100..60000 ms，Cycles／回覆上限 1..10000，Timeout retries 0..3，單次執行最多一小時。
  Reply 留空不等待回覆；啟用時，只有逾時可按明確設定的重試次數重送該命令。
- 部分傳送、driver error、queue 失敗直接停止，不自動重試。Stop／關閉進階視窗／Disconnect／裝置消失均停止。
  主選單 UART > Stop automation 隨時可用。已進入 driver 的命令可能已生效，Stop 不代表可以撤銷。
- 執行時禁止 Console 的手動 TX／流控變更／Break，避免混入命令；Reset statistics 不影響執行。

實體 UART 的 pin／Break／target 副作用、吞吐量與精準時序仍須硬體驗收；模擬傳送測試不代表這些已通過。
未加入韌體特定異常觸發錄製、網路登入或自動 target reset。
