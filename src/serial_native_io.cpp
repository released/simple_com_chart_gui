#include "serial_native_io.h"

bool serial_transfer(HANDLE handle, bool write, void* data, DWORD size, DWORD& transferred,
    const std::atomic<bool>& cancelled, DWORD& error) {
    transferred = 0; error = 0;
    if (cancelled) { error = ERROR_OPERATION_ABORTED; return false; }
    OVERLAPPED operation{}; operation.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!operation.hEvent) { error = GetLastError(); return false; }
    const bool issued = (write ? WriteFile(handle, data, size, &transferred, &operation) :
        ReadFile(handle, data, size, &transferred, &operation)) != FALSE;
    const DWORD issue_error = issued ? ERROR_SUCCESS : GetLastError();
    bool result = false;
    if (issued || issue_error == ERROR_IO_PENDING) {
        // Recheck after issue to cover a cancellation arriving just before Read/WriteFile.
        if (cancelled) CancelIoEx(handle, &operation);
        const auto wait = WaitForSingleObject(operation.hEvent, write ? 1500 : 250);
        const bool timed_out = wait != WAIT_OBJECT_0;
        if (timed_out) CancelIoEx(handle, &operation);
        // OVERLAPPED storage/event must outlive driver completion, even after cancellation.
        // A malfunctioning driver may delay this worker; the UI never waits here.
        result = GetOverlappedResult(handle, &operation, &transferred, TRUE) != FALSE;
        error = result ? (timed_out ? ERROR_TIMEOUT : ERROR_SUCCESS) : GetLastError();
        if (timed_out) result = false;
    } else error = issue_error;
    CloseHandle(operation.hEvent); return result;
}
