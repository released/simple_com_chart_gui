

update @ 2026/01/29

1. a simple vibe coding COM port GUI ( python , MFC )

2. GUI tool location : 

MFC

\native_mfc\build\cmake\Release\simple_com_chart_gui_mfc.exe

\native_mfc\build\vs\Release\simple_com_chart_gui_mfc.exe

PYTHON

\main.pyw

3. function 

	- COM port baud rate adjust
	
	- button Overlay : 
	
	Toggles whether channels are drawn as an overlay; 
	
	updates the plot immediately (or just redraws the overlay when in Snapshot mode).
	
	- button Snapshot : 
	
	Freezes/unfreezes live plotting; 
	
	when ON the plot stops updating, when OFF it resumes live data and refreshes the view.
	
	- button Fit : 
	
	Auto-fits the view to the currently enabled channels.
	
	- button Refresh : 
	
	Clears current samples and resets the plot display; 
	
	updates the status counts (does not disconnect).

4. MFC GUI support data format : 

![image](https://github.com/released/simple_com_chart_gui/blob/main/mfc_help.jpg)

MFC GUI display , 

![image](https://github.com/released/simple_com_chart_gui/blob/main/mfc_ui.jpg)


5. Python GUI support data format :

![image](https://github.com/released/simple_com_chart_gui/blob/main/python_help.jpg)

Python GUI display , 

![image](https://github.com/released/simple_com_chart_gui/blob/main/python_ui.jpg)

![image](https://github.com/released/simple_com_chart_gui/blob/main/python_ui2.jpg)

![image](https://github.com/released/simple_com_chart_gui/blob/main/python_ui3.jpg)

![image](https://github.com/released/simple_com_chart_gui/blob/main/python_ui4.jpg)

![image](https://github.com/released/simple_com_chart_gui/blob/main/python_ui5.jpg)

![image](https://github.com/released/simple_com_chart_gui/blob/main/python_ui6.jpg)


# Simple COM Chart GUI

## Purpose
This tool visualizes real-time MCU log data via UART / COM port.
The MCU firmware must output logs in a specific text format
for this tool to correctly decode and plot channel values.

---

## Supported Log Format

Each log line must be a **single line**, ending with newline (`\r\n`).

### Basic format

key:value,key:value,...

Example:

state:5,CHG:4179mv,T1:2296mv,T2:1589mv,Q6:2111mv,Q2/Q3:21mv

### Rules

- Fields are separated by comma `,`
- Key and value are separated by colon `:`
- Units are optional but recommended (e.g. `mv`)
- Spaces are ignored
- Order of fields does not matter
- Unknown keys are ignored

---

## Reserved Keys (Example)

| Key | Description |
|---|---|
| state | System state (integer) |
| CHG | Charger voltage (mV) |
| T1 | Battery voltage T1 (mV) |
| T2 | Battery voltage T2 (mV) |
| Q6 | MOSFET Q6 voltage (mV) |
| Q2/Q3 | Sense voltage (mV) |

---

## MCU Firmware Example (C)

```c
printf(
    "state:%d,CHG:%dmv,T1:%dmv,T2:%dmv,Q6:%dmv,Q2/Q3:%dmv\r\n",
    state, chg_mv, t1_mv, t2_mv, q6_mv, q23_mv
);
```

