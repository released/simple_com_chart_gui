#pragma once
#include "uart_lab_window.h"
inline bool lab_file_cancel(int page,int action){
    return (page==UartLabWindow::Recording&&action==5)||
        (page==UartLabWindow::Sessions&&action==7)||
        (page==UartLabWindow::Views&&action==5)||
        (page==UartLabWindow::Images&&action==1)||
        (page==UartLabWindow::Notes&&action==6)||
        (page==UartLabWindow::Reports&&action==2);
}
