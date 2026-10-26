#pragma once
#include "serial_manager.h"
#include "step_script.h"
struct FiniteScriptResult {bool passed=false;std::string status,csv;};
// Transport is already exclusively owned/open. Never connect/reconnect or retry.
FiniteScriptResult execute_finite_script(SerialTransport&,std::vector<ScriptStep>,
    const std::atomic<bool>& cancel,uint64_t maximum_ms=60000);
