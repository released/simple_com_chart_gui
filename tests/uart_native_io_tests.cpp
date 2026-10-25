#include "serial_native_io.h"
#include <string>
#include <thread>
#include <stdexcept>
#include <iostream>

namespace {
struct PipeFixture {
    HANDLE server = INVALID_HANDLE_VALUE, client = INVALID_HANDLE_VALUE;
    std::atomic<bool> cancelled{false}, writer_done{false};
    std::thread writer;
    ~PipeFixture() {
        cancelled = true;
        if (client != INVALID_HANDLE_VALUE) CancelIoEx(client,nullptr);
        if (server != INVALID_HANDLE_VALUE) CancelIoEx(server,nullptr);
        if (writer.joinable()) writer.join();
        if (client != INVALID_HANDLE_VALUE) CloseHandle(client);
        if (server != INVALID_HANDLE_VALUE) CloseHandle(server);
    }
};
void require(bool value, const char* label) { if (!value) throw std::runtime_error(label); std::cout << "PASS: " << label << '\n'; }
}
void test_uart_native_io() {
    PipeFixture pipe;
    const auto name = L"\\\\.\\pipe\\SimpleComChart.UartIoTest." + std::to_wstring(GetCurrentProcessId());
    pipe.server = CreateNamedPipeW(name.c_str(),PIPE_ACCESS_DUPLEX|FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_BYTE|PIPE_READMODE_BYTE|PIPE_WAIT,1,4096,4096,0,nullptr);
    require(pipe.server != INVALID_HANDLE_VALUE,"private overlapped pipe fixture created; no COM access");
    OVERLAPPED connect{}; connect.hEvent = CreateEventW(nullptr,TRUE,FALSE,nullptr);
    require(connect.hEvent != nullptr,"fixture connection event created");
    const bool connected = ConnectNamedPipe(pipe.server,&connect) != FALSE;
    const DWORD initial_error = connected ? ERROR_SUCCESS : GetLastError();
    pipe.client = CreateFileW(name.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,nullptr);
    DWORD transferred = 0;
    const bool ready = pipe.client != INVALID_HANDLE_VALUE && (connected || (initial_error == ERROR_IO_PENDING &&
        WaitForSingleObject(connect.hEvent,1000) == WAIT_OBJECT_0 && GetOverlappedResult(pipe.server,&connect,&transferred,FALSE)));
    if (!ready && initial_error == ERROR_IO_PENDING) { CancelIoEx(pipe.server,&connect); GetOverlappedResult(pipe.server,&connect,&transferred,TRUE); }
    CloseHandle(connect.hEvent); require(ready,"private pipe connects with overlapped completion");
    std::string payload("A\0\xFF\r\n",5); DWORD error = 0;
    require(serial_transfer(pipe.client,true,payload.data(),static_cast<DWORD>(payload.size()),transferred,pipe.cancelled,error) && transferred == payload.size(),"native helper exact write completion");
    char buffer[16]{};
    require(serial_transfer(pipe.server,false,buffer,5,transferred,pipe.cancelled,error) && std::string(buffer,transferred) == payload,"native helper exact binary read completion");
    DWORD written = 0, write_error = 0; bool write_result = false;
    std::string blocked(65536,'x');
    pipe.writer = std::thread([&] {
        write_result = serial_transfer(pipe.client,true,blocked.data(),static_cast<DWORD>(blocked.size()),written,pipe.cancelled,write_error);
        pipe.writer_done = true;
    });
    Sleep(20); const bool pending = !pipe.writer_done;
    std::atomic<bool> read_cancelled{false}; std::string inbound = "RX";
    const bool sent = serial_transfer(pipe.server,true,inbound.data(),2,transferred,read_cancelled,error);
    const bool read = serial_transfer(pipe.client,false,buffer,2,transferred,read_cancelled,error);
    const bool received = read && std::string(buffer,transferred) == "RX";
    pipe.cancelled = true; CancelIoEx(pipe.client,nullptr); pipe.writer.join();
    require(pending && sent && received,"native overlapped RX proceeds on the same handle while TX is pending");
    require(!write_result && pipe.writer_done && (write_error == ERROR_OPERATION_ABORTED || write_error == ERROR_TIMEOUT),"cancel completes pending write before OVERLAPPED storage/event release");
    require(!serial_transfer(pipe.client,true,inbound.data(),2,transferred,pipe.cancelled,error) && error == ERROR_OPERATION_ABORTED && !transferred,"pre-cancelled native operation never issues data");
}
