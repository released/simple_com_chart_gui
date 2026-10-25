# UART 改善功能（1.0.1.022）

本次只修改原生 MFC 版。Python、實體 UART 驅動程式與 GitHub 發佈不在本次範圍。
實際驗證結果與限制見 `../SELF_CHECK.md`；實作追蹤見 `UART_OPTIMIZATION_PLAN.md`。

## 啟動、版面與狀態

- 同一 Windows session 只允許一個一般實例，與 EXE 路徑、檔名、版本、Debug/Release 無關。
  重開會通知原視窗，不會再開串口、覆寫設定或結束既有操作；保留 modal／忙碌狀態。
  此協作式單例適用採用同一身分的版本；無法追溯改造尚未加入單例協定的舊 EXE。
- Windows 10 1703+／11 使用 Per-Monitor V2。DPI 改變時重建字型並重新排版。
  主工具列會換行；進階視窗可以縮小與垂直捲動，Tab 導覽會帶到可見範圍。
  4 Charts 在較窄視窗改為 2×2，較寬時維持一列四圖。
- 主畫面持續顯示 LIVE／OFFLINE、REC、AUTO、來源時鐘、過期與遺失計數；檔案 TX 另有標記。
  Expired 是保留歷史淘汰，不等於實體 UART 遺失；Lost 是軟體待處理資料遺失。
  Skipped 包括 Regex 工作量上限、無效解析時間與超長數值行。REC ERROR 不會被普通狀態蓋掉。

## 通用欄位擷取

開啟 **UART > Field extraction...**。Include／Exclude 仍先篩整行，不是選擇個別欄位。

| Parser | 設定方式 |
|---|---|
| Key/value | 相容原本的逗號分隔 `key:value`／`key=value`、單位、百分比、hex、`a/b` 陣列 |
| CSV／Tab | Column names 使用逗號分隔的唯一欄位名，例如 `time,voltage,current`；資料分隔符由 Parser 決定。支援 CSV 引號與雙引號跳脫；欄數不合不補零 |
| JSON | 每行一個 JSON object，只取最上層有限數值；字串／布林／陣列／巢狀物件不當作量測值 |
| Regex groups | 設定 Regex 與 Name／Value group；最多 8 個擷取群組，每行取第一個符合項目 |

Time field 可指定一個既有數值欄位；Regex 也可用 Time group。
時間單位為秒（0..1e12），空白／group 0 使用原本收訊時間。欄位時間以前後差值映射到本次 capture；倒退或無效時間會略過，原始 RX／PC receipt 不變。

**Preview** 只預覽一行，完全不套用；**Apply parser** 驗證、儲存 INI，開始新的數值 capture。
不清除 Messages，不改變正在錄製的原始位元組。設定也包含於新的 `.sccprofile`；舊 profile 缺省為 Key/value。

Regex 採有工作量限制的 NFA，沒有遞迴 backtracking。支援一般 literal、字元類別、`.`、`^/$`、
`|`、群組、`* + ? {m,n}`、`\d/\w/\s` 與 `\xNN`；不支援 backreference、lookaround、
lazy／possessive quantifier、Unicode property 或 inline flag。採 byte-oriented UTF-8、leftmost-longest 比對；不是完整 Python/ECMAScript 相容引擎。
每個模式最多 1024 UTF-8 bytes／1024 NFA states；重複上限 64、巢狀上限 16。
每次線上比對最多 65536 個工作單位，超額整行略過並累計 Skipped，不悄悄截斷後再比對。
無效套用保留原設定；首次啟動若沒有有效 filter，數值 capture 等待有效套用，raw RX 不受影響。

## Channel 與顯示偏好

Chart 與資料欄位面板的搜尋同時比對原 key 與顯示別名；All／None 只作用於目前搜尋結果，隱藏項目不會被改選。
排序、勾選與數值仍綁定原 key。Analysis 與 Channel display 的輸入式選單也可搜尋。

在 **UART > Channel display...** 選原 key，再 Load selected／Apply display／Reset selected。
可設定 Alias、Unit、Decimals（-1 自動，0..12 固定）、Scale 與 Offset。
非常大的固定精度數值以科學記號顯示，避免產生數百位字串。
顯示值 = 原值 × Scale + Offset；溢位顯示 n/a。最多儲存 256 筆偏好、30 KiB。
原 key、raw log、capture 與 CSV/XLS 等匯出數值不變；圖表、面板與表格只在呈現時轉換。
偏好立即存於 EXE 旁 INI，並可隨 profile 攜帶。套用顯示偏好會離開舊 Snapshot，避免混用新舊顯示設定。

## 圖表操作與資料對應

- 滾輪：以游標為中心縮放時間範圍（1..86400 秒），暫停自動跟到最新。
- 中鍵拖曳，或 Ctrl＋左鍵／Space＋左鍵拖曳：平移時間。
- 左／右鍵分別設定 A／B，可持續拖曳；Analysis 同步更新原快照的統計，不重設拖曳狀態。
- 雙擊：Return to latest。Ctrl＋右鍵選單可 Return to latest、Automatic Y 或 Fixed Y range。
  固定範圍使用顯示單位；繪圖裁切於 plot 區域，不把超界線畫到軸標籤。
- 最新標記對應目前可見範圍內的最新樣本；平移到舊資料時不拿未來點的值冒充。

雙擊 4 Charts 表格的一列可定位原始 Messages bytes，同步表格與圖表時間。
Messages > Options > Locate RX row at caret 可回到保留的資料列；圖表游標也會同步表格／Messages。
對應使用原始 RX offset，不用字串搜尋猜測重複資料。已過期、清除、未通過 filter 的資料會明確提示；
暫停／light terminal 顯示不做不可靠的字元位置映射。HEX 以該 16-byte 列定位。
rolling chart 若已淘汰該時間可顯示空白；需較長歷史時使用 Analysis 的 retained capture 快照。

## 大型 log 與分段重播

原本整份載入的 64 MiB／200000-record 限制已取代：現在背景逐塊掃描，raw 檔保持唯讀，
最多 4096 筆索引留在 RAM；更大的索引存於自動刪除的私人暫存檔，使用最多 256 KiB 的映射頁。
每次背景讀取最多四個 4096-byte 排程區塊；Seek／卸載會取消並排除過期結果。

磁碟輸入保護上限為合計 64 GiB、5000 萬 records、4096 segments；不是把整份檔案塞入 RAM。
暫存磁碟空間不足、讀取失敗或取消會顯示錯誤，保留原檔。載入期間來源檔禁止寫入／刪除；Unload 後釋放。

選取任一 `prefix_N.bin` 會依數字順序載入同資料夾的完整 `prefix_1.bin` 到最大編號分段，不是字典排序。
缺段／重複編號／混用有索引與無索引的分段會拒絕。每份 `.idx` 都必須完整涵蓋對應 raw 檔。
PC receipt 時間跨分段連續，倒退夾為零；無索引仍明確標示 synthetic。

**Find next** 以 Text UTF-8 或 HEX 搜尋原始 bytes，從目前 record 往後跨檔案搜尋；
**Seek time** 依檔案時間定位。兩者在背景執行、停在暫停狀態，Play 才將資料送入檢視。
搜尋到結尾不自動繞回；回到 record 0 可重新搜尋。這些操作從不向 UART 傳送。

## Light terminal 與鍵盤傳送

Messages > Options 可選 **Light terminal display**：解讀 CR 覆寫當行、BS 退一格、CSI `0K/1K/2K` 清行。
這不是完整 VT 終端機；每行最多 4096 字元、最多 8192 行，不做全形字元格寬模擬。
此模式不加時間標籤／SGR 色彩；raw RX 與錄製不變，HEX 隨時可檢查原始 bytes。
Messages Save As 仍只存目前 RX 顯示結果，不包含 local TX echo。

**Enable keyboard TX...** 必須已連線且再次確認；只有在 Messages 內容區輸入才送出。
預設未啟用、不保存 arming，斷線或傳送工作接管即解除。一般字元使用 UTF-8，Enter=CR、Tab=09、BS=08、Esc=1B。
Ctrl+C 送出 03，**不是 Break**。貼上最多 4096 UTF-8 bytes，總是先顯示預覽／確認，提醒多行可能執行多個命令。
確認返回後再檢查同一連線世代，避免重連後誤送。Local echo 只顯示在 TX/status，不偽造 RX。

## File TX

開啟 **UART > Send file...**，選 Binary 或 UTF-8 text、換行轉換、Chunk bytes 與 Delay。
Start 明確確認檔案／目標／風險；文字檔先完整背景驗證 UTF-8，無效不送第一個區塊。
Binary 逐 byte 原樣傳送；Text 除使用者選定的 LF／CRLF／CR 轉換外保留所有 bytes，包含 BOM 與控制字元。

Chunk 1..4096 bytes；Delay 0..60000 ms，自上一塊 driver completion 起算。
進度分開顯示 source read 與 driver accepted，不將預讀當作已送達。
一次只有一個檔案區塊等待完成；Pause 停止排入後續區塊，Resume 繼續，Cancel／斷線／關閉會停止。
部分傳送、失敗或連線世代改變立即停止，**沒有 retry，也不在重連後恢復**。
已進入 driver 的 bytes 無法撤銷；driver accepted 不代表 target ACK。
傳送期間禁止混入 Console／鍵盤／Automation；RX、錄製與停止操作維持獨立。
此功能不是韌體燒錄協定，沒有假設 target、ACK 或自動 reset。

## 驗證邊界

軟體驗證使用隔離 INI、合成資料、私人 helper process 與 fake transport；不會開真實 COM。
125%／150% 放大字型只能證明測試尺寸的幾何／畫面，不能冒充實體螢幕 DPI 切換或真實滑鼠操作。
持續 RX 測試的 bytes／CPU／記憶體／延遲是該機器的模擬結果，不是 UART／USB／流控的硬體吞吐量保證。
