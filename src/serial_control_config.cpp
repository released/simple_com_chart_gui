#include "serial_control_config.h"

bool configure_serial_dcb(DCB& dcb, const SerialControls& options) {
    if (!valid_serial_controls(options)) return false;
    dcb.fBinary = TRUE;
    dcb.fOutxCtsFlow = options.flow == SerialFlow::RtsCts;
    dcb.fOutxDsrFlow = options.flow == SerialFlow::DsrDtr;
    dcb.fDtrControl = options.flow == SerialFlow::DsrDtr ? DTR_CONTROL_HANDSHAKE : options.dtr ? DTR_CONTROL_ENABLE : DTR_CONTROL_DISABLE;
    dcb.fRtsControl = options.flow == SerialFlow::RtsCts ? RTS_CONTROL_HANDSHAKE : options.rts ? RTS_CONTROL_ENABLE : RTS_CONTROL_DISABLE;
    dcb.fOutX = dcb.fInX = options.flow == SerialFlow::XonXoff;
    dcb.fTXContinueOnXoff = FALSE; dcb.fDsrSensitivity = FALSE;
    dcb.fErrorChar = FALSE; dcb.fNull = FALSE; dcb.fAbortOnError = FALSE;
    dcb.XonChar = 0x11; dcb.XoffChar = 0x13; dcb.XonLim = 128; dcb.XoffLim = 128;
    return true;
}
