#include "log_parser.h"
#include "line_filter.h"
#include "channel_model.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

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
        ChannelModel model;
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
        std::cout << "PASS: native parser, filter, model and screenshot telemetry\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
