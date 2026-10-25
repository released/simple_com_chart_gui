# UART Workbench

從原生版主視窗選擇 **UART > Workbench / Dual COM...**。Workbench 使用同一個
EXE，不需要額外的 Python 或執行階段套件。

## 兩個 COM 同時顯示

- 上方 **Port A** 與主視窗共用既有連線、Messages 與 Chart。兩邊的連線設定同步。
- 下方 **Port B** 有獨立連線、接收區、傳送佇列與錄製。B 的一般文字是獨立監看；
  B 的二進位解析與 RTU 數值可送到圖表。
- 兩個接收區同時可見，中間分隔線可拖曳。ASCII、HEX、Auto-scroll、Clear、
  Save As 與 Record RX 各自獨立，不會把 A/B 訊息混在一起。
- 同一個實體裝置不能同時由 A/B 開啟。若被其他程式占用，先在該程式中斷線。
- 啟動不會自動開啟 COM。成功連線後會記住設定；已連線的橋接器意外移除時，
  只嘗試找回同一個裝置，不會把舊命令、腳本或檔案傳送重新執行。
- B 使用預設的無流量控制及 DTR/RTS 開啟設定；A 的進階串列控制仍在原本的
  UART Console。連線或改變控制線可能影響設備，請依設備接線使用。

每個接收區的 **Save As** 匯出目前保留、所選模式的快照，預設為電腦日期時間
到秒數及 A/B 後綴，支援 `.log`、`.txt`。ASCII／HEX／Terminal 是 RX 顯示；
RX/TX inspect 也包含驅動程式已接受的 TX。Clear 只清除該接收區，不停止
數值解析或連續錄製。接收歷史有容量上限；連續保存請用 **Record RX**。
首次啟用 Workbench／二進位解析後才收集這裡的歷史，不回填較早的主視窗 Messages。

Record RX 各自產生 `_1.bin`、`.log`、`.idx` 及後續輪替段。`.bin` 只含原始 RX；
可讀 log 分開標示 TX/RX。兩個錄製目的地必須使用不同前綴。磁碟寫入、容量或
關閉錯誤會顯示出來；保留的部分檔案不等於完整錄製。

## 封包檢視

**Tools > Packet inspection A/B...** 設定各自的分組方式：

| 模式 | 分組方式 |
| --- | --- |
| Read chunks | 原始讀取區塊，並非通訊協定封包 |
| LF / CRLF | 等待完整分隔符號，可跨多次讀取 |
| Fixed bytes | 固定總長度 |
| Idle receipt gap | 依電腦接收時間的空檔分組 |
| Header + fixed bytes | 找到 HEX header 後，取固定總長度 |

RX/TX 使用不同的組包緩衝。每列顯示方向、電腦接收時間、時間差、長度及
HEX／ASCII 預覽；長封包的畫面預覽會縮短，Save As 保留已保留封包的完整內容。
不完整／溢位／重新同步捨棄的位元組會計數。變更設定只重開封包檢視，
不改寫原始 RX 或錄製。這些分組設定只在本次執行有效。

USB、驅動與排程會合併讀取，電腦接收時間不是精確的 UART 線路時間，
Idle receipt gap 不能取代硬體邏輯分析儀。

## 二進位數值與圖表

**Tools > Binary parser A/B...** 可設定 header、固定總長度，或長度欄位的位置、
寬度、大小端及額外長度。長度與 offset 都是從零開始的十進位位元組數。

欄位一列一個：

```text
rpm,2,u16le
temperature,4,i16be,0.1,-5
```

格式為 `name,offset,type[,scale[,add]]`。支援 `u8/i8` 及 `u16/i16/u32/i32/f32/f64`
的 `le/be` 形式。輸出為 `原值 × scale + add`，不產生無效或非有限數值。
Checksum 支援 None、Sum8、XOR8、CRC16 Modbus、CRC16 CCITT-FALSE、CRC32；
checksum 位於封包末端，可指定計算起點與儲存大小端。

**Preview HEX** 只解析貼入的資料，不傳送、不套用。**Apply** 儲存設定並開始
新的數值擷取，保留原始 Messages 與錄製。A 啟用二進位解析時，取代 A 的文字
數值解析；文字 Include／Exclude 不用來過濾二進位封包。A/B 圖表欄位分別為
`A.name`、`B.name`。跨讀取組包，斷線世代之間不會拼接半個封包。

共用圖表使用 PC receipt 時間，不能同時使用文字解析器的 Time field／Time group。
離線操作、背景設定或工具傳送期間的衝突會拒絕套用。設定保存於可攜 INI，
重啟可恢復解析選項，但不會開啟硬體。解析及顯示有明確的工作量、緩衝與
輸出上限；CRC／loss 計數包含解析失敗、緩衝間隙或畫面佇列捨棄，並非精確的
實體 UART 錯誤統計。需要完整資料時保留原始錄製。

## 命名命令與逐步腳本

開啟 **Tools > Commands / Step scripts...**，先選擇 **Port A / Port B**。

Commands 支援群組篩選及命名清單。New／Edit 可設定 TEXT（UTF-8）或 HEX、
結尾 CR／LF／CRLF、checksum，以及參數的最小值、最大值、step 與預設值。
面板可用數值欄、`-`／`+` 或 slider 調整本次參數，**Send / Start** 才傳送；
選取、載入、預覽或移動 slider 都不會傳送。Edit 可修改已儲存預設值，
送出後也會記住該次數值。

- TEXT 參數使用 `{value}`。
- HEX 使用 `{value}`（u8），或 `{value:u8/i8/u16le/u16be/i16le/i16be/u32le/u32be/i32le/i32be}`。
- checksum 在結尾字元之前加入。完整傳送資料限制為 1..4096 bytes。
- 最多 128 個命令；同群組／名稱不可重複。

Step scripts 用 New／Edit 定義每一步的命令、傳送前延遲、原始 RX Expect 及
timeout；Up／Down 調整次序。Expect 可用 TEXT 或 HEX，空白表示只等驅動程式
接受傳送。Start 會先確認設備操作，逐步結果可另存 log。

腳本為有限次序，最多 128 步／一小時；每步 timeout 100..60000 ms，
不自行重試。Stop、關閉命令視窗、斷線或應用程式關閉都會停止；已接受的
位元組不能撤回。匹配到原始 RX 只是字串比對，不代表設備協定驗證成功。
同一個 COM 的腳本、Console／keyboard、檔案傳送、RTU 及串列控制互斥；
A/B 可以各自執行獨立工作。

命令／腳本預設保存在 EXE 旁的 `commands.scccommands`、`commands.sccsteps`。
Import／Export 使用有版本及容量驗證的定義檔；錯誤匯入保留原資料。
這些檔案及 INI 是明文設定，可能含私人命令，不應發布。啟動讀取設定不會
自動執行命令或腳本。

## 傳送節奏

**Tools > TX pacing A/B...** 設定 byte gap（0..1000 ms）及 line gap
（0..60000 ms）。A 套用到原本 Console、keyboard/paste、命名命令、腳本及
檔案 TX；B 套用到該埠的傳送。原有檔案 chunk delay 另外計算。
CRLF 在同一個 chunk 內計為一次行結尾。設定需在 TX 閒置時套用，並保存在 INI。
這是 Windows／驅動程式的盡力節奏，不保證實體線路的精確間隔。
單次 payload 的預估延遲超過一小時會拒絕排入佇列。

## XMODEM / YMODEM

**Tools > File transfer A/B...** 選擇 Send／Receive、XMODEM checksum／CRC／1K，
或 YMODEM batch，再選擇檔案／目的地。支援每區塊 1..10 次嘗試及 timeout。
埠的狀態列顯示進度，**Tools > Stop operation A/B** 可取消。

- XMODEM 一次一檔，不傳檔名或長度。接收時可指定已知長度；留空會保留
  所有填補位元組，不擅自刪除檔尾。CRC 使用 XMODEM 的初值零，並非 CCITT-FALSE。
- YMODEM 一次 1..128 檔，接受 128／1024-byte 區塊、檔名、可選長度及批次結尾。
  使用可攜 ASCII 檔名，不接受路徑穿越或裝置名稱，接收批次不覆蓋同名檔案。
- 接收先寫入同目錄的私有暫存檔，完整驗證、flush 後才成為成品；
  XMODEM 的覆蓋需經另存對話框同意。失敗保留原檔；已完成的批次檔案仍保留。
- 檔案讀寫在背景執行，資料緩衝有上限，總傳輸限制 64 GiB／一小時。
  重傳針對協定要求的相同區塊；驅動部分寫入或不確定的寫入錯誤會停止。
- 取消時盡力送 CAN；連線／驅動失效時無法保證對方收到。斷線不會重新傳送。

這是通用 PC 檔案傳輸，不包含設備的擦除、解鎖、燒錄或韌體驗證流程。
對方必須使用相同傳輸協定，傳輸期間避免混入一般 debug 輸出。
工具暫停該埠的普通數值解析，原始 RX 與錄製繼續保存。

## Modbus RTU

**Tools > Modbus RTU A/B...** 提供唯讀 FC03 Holding／FC04 Input 輪詢。
Unit 1..247、起始位址從零開始（例如文件中的 40001 對應 address 0），
每次 1..125 registers；不提供 broadcast 讀取或寫入功能。

Fields CSV：`name,word_offset,type[,ABCD/CDAB/BADC/DCBA[,scale[,add]]]`。
支援 u16、i16、u32、i32、f32、f64；CDAB 反轉 16-bit word 順序，
BADC 交換每 word 的 byte，DCBA 兩者皆做。欄位空白時顯示每個原始 register。
圖表名稱為 `A.RTU.name`／`B.RTU.name`，或 `A.RTU.r0` 等原始位址。

Preview 只顯示 request；Apply 才開始輪詢。一次只有一個未完成 request，
檢查 unit、function、長度、CRC 及 exception。Timeout／exception／斷線停止，
不把延遲回覆當成下一筆交易。Stop operation 可停止。參數只在本次執行有效。
RS-485 的方向控制與實體時序由所用硬體／驅動負責，PC 接收時間不是線路時序驗證。

## Terminal 與 Instruments / FFT

接收區 **Terminal (VT100)** 為獨立的單色 ASCII 畫面，支援游標定位／移動、
清除畫面／行、捲動區、tabs、插入／刪除及 DEC 線條字集。設定 rows／columns
在 **Tools > Terminal screen A/B...**。不是完整 VT100 認證模擬：不處理彩色
SGR、query 自動回覆、keypad 協商或 UTF-8 寬字元。原始 HEX／錄製始終可用，
畫面控制不會自動向設備傳送回覆。

**Tools > Instruments / FFT...** 提供獨立選擇欄位的 Number、Gauge、Bar、LED，
以及 FFT。Ranges / FFT 設定範圍、LED threshold、FFT 點數與取樣率。
欄位及範圍保存在 INI；缺少的欄位顯示 n/a，不選任意替代數值。

FFT 使用 32..4096 的 2 次方點數、Hann window、去平均值的單邊振幅頻譜。
取樣率 0 會檢查原始樣本時間是否單調且在 5% 規則間隔內；重複、倒退或
不規則的接收時間會拒絕計算，不靜默重取樣。確知設備取樣率時可明確指定，
畫面會標示為使用者指定。Analyze FFT 為快照，Auto FFT 最多每秒更新兩次。
此顯示套用 channel scale／offset，但不改寫原始擷取或匯出值。

## 參考協定

- [XMODEM/YMODEM protocol reference，Chuck Forsberg 編輯](https://android.googlesource.com/platform/external/syslinux/+/refs/heads/oreo-m7-release/com32/libupload/ymodem.txt)
- [Modbus application protocol V1.1b3](https://www.modbus.org/file/secure/modbusprotocolspecification.pdf)
- [Modbus 官方規範入口](https://www.modbus.org/modbus-specifications)
- [DEC VT100 technical manual](https://vt100.net/dec/ek-vt100-tm-002.pdf)
