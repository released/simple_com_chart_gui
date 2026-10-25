#include "binary_options.h"
#include "uart_files.h"
#include "message_format.h"
#include <algorithm>
#include <cmath>
bool parse_binary_options(const std::vector<std::wstring>& f,BinaryFrameConfig& out,std::wstring& error){
    if(f.size()!=12||(f[0]!=L"Enabled"&&f[0]!=L"Disabled")||(f[5]!=L"Little"&&f[5]!=L"Big")||(f[9]!=L"Little"&&f[9]!=L"Big")){error=L"Invalid binary configuration fields/mode/order.";return false;}
    std::string compact;for(auto c:f[1]){if(c==L' '||c==L'\t'||c==L'\r'||c==L'\n')continue;if(c>127){error=L"Header must be HEX.";return false;}compact+=static_cast<char>(c);}if(!uart_unhex(compact,out.header)){error=L"Invalid header HEX.";return false;}
    const int indexes[]={2,3,4,6,8};size_t* outputs[]={&out.fixed_bytes,&out.length_offset,&out.length_width,&out.length_add,&out.checksum_from};
    for(size_t i=0;i<5;++i){double value=0;if(!uart_number(f[indexes[i]],0,65536,value)||value!=std::floor(value)){error=L"Binary byte counts must be integers 0..65536.";return false;}*outputs[i]=static_cast<size_t>(value);}
    out.length_big_endian=f[5]==L"Big";out.checksum_big_endian=f[9]==L"Big";
    const std::vector<std::wstring> names{L"None",L"Sum8",L"XOR8",L"CRC16 Modbus",L"CRC16 CCITT-FALSE",L"CRC32"};const auto found=std::find(names.begin(),names.end(),f[7]);if(found==names.end()){error=L"Unsupported checksum.";return false;}out.checksum=static_cast<FrameChecksum>(found-names.begin());std::string detail;
    if(!parse_binary_fields(message_utf8(f[10]),out.fields,detail)||!validate_binary_config(out,detail)){error=uart_wide(detail);return false;}return true;
}
