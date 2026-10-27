#include "log_parser.h"
#include "line_filter.h"
#include "channel_model.h"
#include "chart_time.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <filesystem>

void test_capture(const std::filesystem::path& project);
void test_messages();
void test_serial_connection();

void check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

int main(int argc, char** argv) {
    try {
        check(argc == 2, "Fixture argument required");
        std::ifstream fixture(argv[1]);
        std::string fan, ac;
        check(static_cast<bool>(std::getline(fixture, fan)), "Read fan fixture");
        check(static_cast<bool>(std::getline(fixture, ac)), "Read AC fixture");
        auto first = log_parser::parse_kv_log(fan);
        auto second = log_parser::parse_kv_log(ac);
        check(first.at("fg_hz") == 69.1 && first.at("err") == 11, "Float / percent");
        check(first.at("cap[2]") == 29 && first.at("tick[1]") == 609, "Vector fields");
        check(second.at("cmd") == 12802 && second.at("offset") == -2.5, "Hex / negative");
        check(first.count("fan") == 0 && first.size() == 19, "Prefix and discovered keys");
        auto extra = log_parser::parse_kv_log("x=-.25,A:2e-3,Q2/Q3:21mv,very.long-key:2.5 Hz,neghex:-0x10");
        check(extra.at("x") == -.25 && extra.at("A") == .002 && extra.at("Q2/Q3") == 21, "Extended numeric forms");
        check(extra.at("very.long-key") == 2.5 && extra.at("neghex") == -16, "Keys and signed hex");
        check(log_parser::parse_kv_log("x:1/?,z:NaN,a:1e999,b:0xGG,c:hello2,d:12.3.4,e:1/").empty(), "Reject malformed fields");
        check(log_parser::parse_kv_log("overflow:0x10000000000000000").empty(), "Reject hex overflow");
        check(log_parser::parse_kv_log("underflow:1e-999").at("underflow") == 0, "Decimal underflow matches Python");
        LineFilter filter;
        std::string error;
        check(filter.matches(fan), "Empty filters");
        check(filter.configure("fan,", "fault:1", false, error) && filter.matches(fan) && !filter.matches(ac), "Literal filter");
        check(!filter.matches("fan,fault:1"), "Exclude priority");
        check(filter.configure("^(fan,|ac_adc_mv:)", "fault:[1-9]", true, error) && filter.matches(fan) && filter.matches(ac), "Regex filter");
        check(!filter.configure("[", "", true, error) && !error.empty() && filter.matches(fan), "Invalid regex retains active rules");
        check(filter.configure("fan,ac_adc_mv:", "", false, error) && !filter.matches(fan) && !filter.matches(ac) &&
            filter.matches("fan,ac_adc_mv:123"), "Text comma is literal, not a multi-string separator");
        check(filter.configure("fan;ac_adc_mv:", "", false, error) && !filter.matches(fan) && !filter.matches(ac) &&
            filter.matches("fan;ac_adc_mv:123"), "Text semicolon is literal");
        check(filter.configure("fan,|ac_adc_mv:", "", false, error) && !filter.matches(fan) && !filter.matches(ac),
            "Text pipe is literal too; explicitly select Regex");
        check(filter.configure("fan,|ac_adc_mv:", "error|timeout", true, error) && filter.matches(fan) && filter.matches(ac),
            "Displayed Regex OR include example accepts both line types");
        check(!filter.matches("fan,error,value:1") && !filter.matches("ac_adc_mv:1,timeout") &&
            !filter.matches("other:1") && filter.matches("fan,Error,value:1"), "Regex OR exclusion wins and is case-sensitive");
        check(filter.configure("fan;ac_adc_mv:", "", true, error) && !filter.matches(fan) && !filter.matches(ac),
            "Regex semicolon is literal, not OR");
        check(filter.configure("", "", true, error) && filter.matches(fan) && filter.matches("anything"), "Empty filters accept all");
        ChannelModel model;
        double seconds=7;
        check(chart_time::parse(L"250ms",seconds)&&seconds==.25,"250 ms input converts to seconds");
        check(chart_time::parse(L" 500 MS ",seconds)&&seconds==.5,"500 ms input supports spacing and units");
        for(int sec=1;sec<=300;++sec)check(chart_time::parse(std::to_wstring(sec),seconds)&&seconds==sec,"Every integer second 1..300 is accepted");
        for(const auto* text:{L"",L"0",L"249 ms",L"300.01",L"301",L"nan",L"inf",L"1junk",L"1 s extra",L"1,5"})
            check(!chart_time::parse(text,seconds),"Malformed/out-of-range input is rejected");
        check(chart_time::label(.25)==L"250 ms"&&chart_time::label(.5)==L"500 ms"&&chart_time::label(299)==L"299 s","Canonical display units preserve exact windows");
        check(chart_time::tick_step(.25,5)==.05&&chart_time::tick_step(.5,5)==.1,"Subsecond X ticks retain distinct fractional seconds");
        model.set_time_window(.25);model.update_from_kv({{"short",1}},1);model.update_from_kv({{"short",2}},1.2);model.prune(1.4);
        check(model.get_time_window()==.25&&model.get_series("short").size()==1&&model.get_series("short").back().v==2,"250 ms pruning does not silently use 1 second");
        model.set_time_window(std::numeric_limits<double>::quiet_NaN());
        check(model.get_time_window()==.25,"Nonfinite time does not poison chart state");
        model.reset();check(model.get_time_window()==.25,"Connection sample reset preserves the committed time range");model.set_time_window(30);
        model.update_from_kv(first, 1);
        model.update_from_kv(second, 1.1);
        check(model.get_keys().size() > 16, "More than 16 channels");
        check(model.get_series("offset").back().v == -2.5, "Signed model value");
        model.set_enabled("offset", false);
        check(!model.is_enabled("offset"), "Channel checkbox state");
        model.prune(100);
        check(model.get_series("offset").empty(), "Time pruning");
        model.reset();
        model.set_max_channels(1);
        model.update_from_kv(first, 1);
        check(model.get_keys().size() == 1 && model.consume_dropped_keys() > 0, "Configured channel cap");
        test_capture(std::filesystem::path(argv[1]).parent_path().parent_path());
        test_messages();
        test_serial_connection();
        std::cout << "PASS: native parser, filter, model and screenshot telemetry\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
