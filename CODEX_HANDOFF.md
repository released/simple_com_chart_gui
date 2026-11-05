# CODEX_HANDOFF.md

> 恢復開發追記（2026-11-05）：使用者已明確要求「有問題，就要修正」。
> 下方暫停快照與等待授權的描述僅供歷史追溯，已由本次恢復取代。
> 已在主對話框資源加入 WS_CLIPCHILDREN，版本改為 1.0.1.044；
> 新增 message_background 針對性測試，13 項檢查通過（8.37 秒）。
> 本次修正已完成，並非暫停：Release GUI／CLI 已更新至 build 目錄；
> 正式版在獨立桌面啟動／正常關閉成功，沒有關閉使用者程序或操作實體 COM。
> 已清除 373 個 run 檔案（793255909 bytes）及 12 個證據檔（160270 bytes）；
> 最後 safe clean 後 63 個正式／使用者檔案雜湊未變，verify 已無檔案。
> 自動審查拒絕含目錄的清理，縮小為逐檔雜湊核對後清理成功；空目錄保留。
> 詳細結果在 HANDOFF、SELF_CHECK、latest-validation 的 .044 區段。
> 未驗證實體 COM／使用者原始 repaint 觸發點；既有完整資料夾容量超標仍保留，
> 不刪 Git 或使用者資料。下方原 .043 未修正／pending 文字均為歷史快照。

> 恢復追記（2026-11-05 21:13 +08:00）：使用者已明確恢復。已核對
> workspace、未提交修改、主對話框 STYLE 與正式 GUI hash；仍為未修正的
> .043，沒有本專案執行程序。第4、5、9節所述清理結果已回填
> HANDOFF／SELF_CHECK／latest-validation；沒有重建或重跑應用程式。
> 下方保留原暫停快照供追溯，其中 pending 清理標記已被此追記取代。
> 原授權為原因定位；是否一併做正式修正的範圍確認尚待使用者答覆。

- 記錄時間：2026-11-05 20:03:14 +08:00（Asia/Taipei，暫停時唯讀快照）。
- 專案：`D:\SourceCode\python\simple_com_chart_gui`。
- 正式版本：`1.0.1.043`。
- 暫停依據：使用者明確要求在安全點暫停、保存交接，不提交、不還原、不丟棄修改。
- 最後安全點：暫停前已啟動的驗證證據檔案清理已完成；沒有再開始修正、建置、測試或新一輪清理。本交接檔是暫停指令後唯一新增／修改的專案文件。

## 1. 原始任務與目標

- 前一批已獲准的 N1–N6：原生 MFC 關鍵字標色、智慧捲動／新增位元組計數／回到最新、不可變原始 bytes 檢視、訊息／圖表分割、最近使用項目、工作階段事件。這些已實作於 .043；詳細歷史在 HANDOFF、SELF_CHECK、docs/MONITOR_WORKFLOW_PLAN.md。
- 目前任務：使用者提供「訊息頁上半部大片灰色、只有底部黑底綠字」截圖，先問是否記憶體不足，再要求「找出原因」。本輪屬原因定位，尚未要求修改正式程式。
- 重要持續限制：使用者明確說「不要用 computer-use 干擾我的作業」。取消前只曾嘗試唯讀取得視窗資訊；視窗已最小化，未執行輸入。取消後沒有再呼叫 computer-use，後續不得重試或操作使用者桌面／連線。
- 公用測試規則已在前一批同步：首次完整開發做一輪全功能驗收；之後只測改動與受影響路徑，沿用仍適用的舊證據。再次完整測試須使用者明確要求。不可預設跑 All、Python、所有字型／Debug 變體。

## 2. 目前開發狀態

- 已確認並在隔離環境重現父視窗背景覆蓋訊息子視窗的缺陷。
- **正式 .043 仍未修正**；本輪沒有修改 production C++、資源、VERSION 或正式 EXE，也未重建正式程式。
- `src/SimpleComChart.rc:31–32` 的 `IDD_MAIN_DIALOG` STYLE 缺少 `WS_CLIPCHILDREN`；`CMainDialog::OnInitDialog` 沒有補上該樣式。MessagePage 自己的 `WS_CLIPCHILDREN` 不能阻止其祖先視窗畫進來。
- `docs/verification/latest-validation.json` 根節點已將 `application_acceptance_passed` 設為 false，status 說明目前有未修正背景問題；先前 N1–N6 正向測試紀錄仍保留，並非否定其歷史結果。
- 新的診斷資料在 JSON 的 `message_background_diagnosis`：包含完整原生 probe 原始碼、CMake recipe、pixel 結果、來源／測試 EXE hashes 與限制。

## 3. 已完成工作

- 唯讀檢查程式與資源。先前量到私有記憶體約 7 MiB、工作集約 15.9 MiB、系統可用實體記憶體約 1.67 GiB；截圖原始接收約 4 KiB／256 KiB、遺失 0。未以這些數字宣稱已檢查所有系統資源。
- 最初一般 offscreen／WM_PRINTCLIENT 快照都顯示完整黑底，無法證實省略 erase 就是原因，因此沒有依猜測修改程式。
- 改用完全獨立的 non-input desktop 執行原生 MessagePage 假資料測試。沒有呼叫 SwitchDesktop，沒有切換輸入桌面、傳送鍵盤滑鼠或接觸實體 COM；測試結束原 thread desktop restored=1、private desktop closed=1。
- 使用現有 production MessagePage 原始碼、同樣未設 child clipping 的測試父視窗、200 行假 RX，重現與使用者截圖相同的上灰／下黑綠文字區塊。
- 實驗僅讓父視窗部分區域重畫：上方三個取樣 pixel 從 `000000` 變成 `F0F0F0`，底部仍 `000000`，子視窗沒有 pending repaint。原始 3892 bytes 與文字未變、輸入遺失 0。
- 同一個視窗只加入 `WS_CLIPCHILDREN`，重複相同父視窗重畫後四個取樣 pixel 全保持 `000000`，資料仍相同。確認 child clipping 的保護效果。
- 另以完整 `RDW_NOERASE` 重畫可恢復黑底，故先前「RDW_NOERASE 本身是主因」的猜測未被支持。
- 實際受損畫面採隔離視窗現有 surface 的 BitBlt，沒有先要求它完整重畫；已視覺檢查 baseline-native.png 與 clipped-native.png。WM_PRINT／PrintWindow 會重畫內容，可能掩蓋這類殘留畫面。
- 已保存診斷、可重現原始碼／recipe 到既有文件，再退役所有 owned test EXE、obj、影像與證據副本。

## 4. 進行中工作與安全停止點

- 沒有建置、測試、檔案寫入或 probe worker 尚待完成。`functions.exec` cell 338 已在暫停後僅等待既有清理結束，回傳成功。
- 已開始的最後檔案退役結果：16 檔／260929 bytes 已刪除；`build/verify` 檔案數為 0；正式 GUI hash 不變；零檔案 verify 目錄樹共有 55 個目錄。
- **文件尚未收尾**：清理在暫停前已啟動，結果在暫停後才回來；依暫停規範沒有再改其他文件。因此下列 pending 標記是文件落後，不代表還有那些暫存檔：
  - HANDOFF.md、SELF_CHECK.md：`PAINT_DIAG_CLEANUP_PENDING`。
  - latest-validation.json → message_background_diagnosis.artifact_cleanup.evidence_retirement：`PENDING_PAINT_EVIDENCE_RETIREMENT`。
- 本交接檔保留清理實際結果；恢復後再核對及同步，不能為了補證據重建剛刪除的 EXE。

## 5. 剩餘任務

1. 只有在使用者明確恢復後，才重新讀取本檔並核對 workspace。
2. 將上述已完成清理結果同步到 HANDOFF／SELF_CHECK／latest-validation，移除已過時的 pending 標記；做文件檢查即可，不跑應用程式測試。
3. latest-validation 的 `capacity_current`／部分文件檢查資訊仍是前一批的快照；本輪診斷增加文件內容後未重新計量／收尾。暫停期間未啟動容量掃描或新清理。
4. 目前授權是「找原因」，不是正式修正。如果後續使用者要求修正，最小方向是在主對話框 STYLE 加 `WS_CLIPCHILDREN`，保留既有訊息增量更新，不用每筆 RX 強制全視窗重畫掩蓋問題。
5. 若需要找出使用者當次究竟由哪個事件觸發父視窗重畫，這仍未直接追蹤；不可為此干擾其即時工作。使用隔離重現，或待使用者另行指定可操作環境。

## 6. 修改與新增檔案

### 本輪原因定位

- HANDOFF.md：新增背景診斷、A/B 證據與未修正狀態；清理標記尚待文件收尾。
- SELF_CHECK.md：新增只涵蓋背景問題的驗證結果與限制；清理標記尚待文件收尾。
- docs/verification/latest-validation.json：新增 `message_background_diagnosis`，保存完整可重現 probe source／CMake recipe；標記正式程式目前有已知未修正問題。
- CODEX_HANDOFF.md：本次暫停交接。
- 暫時生成的 probe.cpp、CMakeLists.txt、obj、EXE、PNG、報告副本均在 owned run／evidence 下；現已退役。沒有把測試 target 加進正式 CMakeLists。

### 原先已存在的大量未提交工作，必須全部保留

- 根目錄 AGENTS.md、CMakeLists.txt、README.md、HANDOFF.md、SELF_CHECK.md、VERSION，以及四個正式 build EXE／catalog 輸出。
- docs/USER_GUIDE.md、docs/USER_GUIDE_EN.md、docs/MONITOR_WORKFLOW_PLAN.md、docs/verification/latest-validation.json。
- scripts/test_artifacts.ps1、scripts/verify_runtime.ps1。
- src/SimpleComChart.rc、resource.h、i18n catalogs、light_terminal、message_format、message_page 系列、mfc_main_dialog 系列、uart_tool_form、uart_workbench、runtime_* 與 tests/ui_language_tests.cpp 等。
- 未追蹤的新模組／測試包含：message_page_monitor、mfc_main_dialog_monitor、monitor_center、monitor_features、runtime_monitor、runtime_message_readability／repaint、tests/message_view_tests、monitor_feature_tests、light_terminal_tests。
- 這些 Git M／?? 並非全由目前診斷產生；不要回復、重做、格式化或丟棄。暫停時 git status 已唯讀核對，沒有 commit／reset／checkout／stash。
- 前一批在 `D:\SourceCode\python\#READ_ME_FIRST#` 更新的 AGENTS／WORKSPACE_RULES／SELF_CHECK／HANDOFF 測試範圍規範亦需保留。本輪未修改它們。

## 7. 重要實作與決策

- 主對話框現在仍為：`STYLE DS_SETFONT | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU | WS_THICKFRAME`。
- 受控重現使用 `RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_NOCHILDREN` 只重畫父視窗上方至訊息區 3/5 的區域，以隔離父視窗覆蓋機制；不是宣稱已在 production 找到同樣 flags 的呼叫。
- Win32 官方樣式語意：https://learn.microsoft.com/en-us/windows/win32/winmsg/window-styles 。WS_CLIPCHILDREN 會排除子視窗佔用區域，避免父視窗畫入。
- 可重現程式碼取自 JSON 的 `message_background_diagnosis.reproduction_source`／`reproduction_cmake`。後續若重跑，須產生全新、專案內、具 ownership 的 run directory；不能直接使用已刪除的舊路徑。
- 原 probe run 身分：`Message-paint-diagnosis`／token `2007918f4c904ffaa83f7063210dae60`。run 路徑已不存在。
- 不需要接實體 COM，不要重跑全部功能，不要因分析結果擅自安裝依賴或修改 app 設定。

## 8. 已知問題、限制與程序狀態

- 灰底覆蓋問題仍存在於正式 .043；沒有套用 clipping 修正，也沒有 bump 版本。
- 實際 live app 當次 repaint caller／事件時序未追蹤。已確認的是可重現的繪圖缺陷與對應來源缺漏，不應宣稱完成了使用者實體工作階段的逐事件追蹤。
- 暫停時唯讀程序查詢沒有觀察到目前專案路徑下的執行程序。先前使用者的 SimpleComChart PID17512 曾運作，但本代理沒有關閉它；不能推斷其硬體／錄製是否由使用者停止。
- 沒有已知 agent-owned test／build worker。其他專案、整台電腦及硬體操作未檢查，不能據此宣稱整台電腦可安全關機。
- 先前自動審查拒絕刪除 verify 空目錄；未重試或繞過。現為 0 檔案、55 目錄的樹，無待清除的 probe binary／影像。
- 既有完整專案 45 MB 容量上限仍未達成。前一批約 55.1 MB、Git40340528 bytes；本輪新增文件後沒有重新計量。不得為達容量刪 Git／使用者資料，也不得宣稱獲得新容量例外。

## 9. 建置、測試與驗證證據

### 本輪新執行

- 只建置獨立 `paint_probe`，MSVC x64 Release／靜態 CRT／C++17，引用現有 MessagePage 及其相依 source，link comctl32／gdiplus；不建置正式 GUI／CLI，不執行完整功能 suite。
- Recipe：`cmake -S <fresh-owned-run> -B <run>/obj -G "Visual Studio 17 2022" -A x64`；`cmake --build <run>/obj --config Release --target paint_probe --parallel 2`；hidden process `paint_probe.exe <fresh-owned-run>`。
- 最終結果：private_desktop=1、input_desktop_switched=0；baseline pixels F0F0F0/F0F0F0/F0F0F0/000000；child pending repaint=0；clipped pixels 全000000；raw3892 bytes/lost0；desktop restored=1、private desktop closed=1。
- Probe source SHA256：`C51809DFBBE830EB931066907F2CD1EAE67089499F8B5FF53315AC7234726F9D`。
- 測試 EXE472576 bytes，SHA256 `B1654AF73C194B9551AD6F649F8262A60ECAE64CC510E54DF6A8298BF6DA8F0E`，以及 compiler-ID EXE15360 bytes，均已刪除。
- Complete-VerificationRun 已處理90個登錄檔案、4524164 generated bytes；曾暫留15個證據檔225762 bytes，彙整後連同 registry 共16檔260929 bytes 也已退役。
- 已執行既有安全 `scripts/clean_mfc.bat`：0 files／0 directories；沒有停止 live EXE，沒有重跑未變更的 clean 稽核。
- 暫停後只完成已在執行的最後退役操作；唯讀核對 `build/verify` 檔案數0。
- 正式 GUI SHA256 不變：`7C27CD29E92D5102932DDB1DAFAE9D8B9DDB6AD29EC0654BBE429E779260E148`。

### 前一批證據，非本輪重跑

- N1–N6 native EN／TW Font125／Debug TW Font125 各60項、最終 message_view27項，以及 60秒假 RX／錄製 Soak：5275648 bytes、0 pending loss、max UI gap312 ms。完整細節在既有 records。
- 本輪沒有重跑 All、Python、協定、Serial、Soak、所有字型／Debug 變體；既有印出式 screenshot 方法對本次 stale-background 問題有遮蔽風險，不能拿舊 PASS 當成此缺陷不存在。

## 10. 恢復建議

1. 等使用者明確恢復；先讀 workspace entry、本檔、HANDOFF、SELF_CHECK 及最新 JSON，核對實際 source／程序，保留全部現存修改。
2. 先做尚未完成的文件收尾，把已完成清理的16檔／260929 bytes／verify0檔／55目錄記入原 records，檢查 UTF-8／JSON／狀態一致；不要重建測試 EXE 來補已存在的證據。
3. 若使用者只是恢復診斷，完成原因報告即可；若明確要求修正，再做主對話框 child-clipping 的最小修改及相關背景／捲動／resize 驗證，不擴大為整套測試。
4. 持續遵守不使用 computer-use、不干擾使用者桌面／目前工作之限制。不得為建置逕自中斷後續可能恢復的硬體工作階段。
5. 不提交、不推送、不還原；沒有使用者新指令前，交接後停止。
