from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'src' / 'python'))

from core.log_parser import parse_kv_log
from core.line_filter import LineFilter
from core.channel_model import ChannelModel


class TelemetryTests(unittest.TestCase):
    def setUp(self):
        self.fan, self.ac = Path(__file__).with_name('telemetry.txt').read_text().splitlines()

    def test_screenshot_and_extended_forms(self):
        values = parse_kv_log(self.fan)
        self.assertEqual((values['fg_hz'], values['err'], values['cap[2]'], values['tick[1]']),
                         (69.1, 11, 29, 609))
        self.assertEqual(len(values), 19)
        values = parse_kv_log(self.ac)
        self.assertEqual((values['cmd'], values['offset']), (12802, -2.5))
        self.assertEqual(parse_kv_log('x=-.25,A:2e-3,Q2/Q3:21mv,very.long-key:2.5 Hz,neghex:-0x10'),
                         {'x': -.25, 'A': .002, 'Q2/Q3': 21, 'very.long-key': 2.5, 'neghex': -16})

    def test_invalid_fields(self):
        self.assertEqual(parse_kv_log('x:1/?,z:NaN,a:1e999,b:0xGG,c:hello2,d:12.3.4,e:1/'), {})
        self.assertEqual(parse_kv_log('overflow:0x10000000000000000'), {})
        self.assertEqual(parse_kv_log('underflow:1e-999'), {'underflow': 0})

    def test_filters_and_error_recovery(self):
        rule = LineFilter()
        self.assertTrue(rule.matches(self.fan))
        rule.configure('fan,', 'fault:1')
        self.assertTrue(rule.matches(self.fan))
        self.assertFalse(rule.matches(self.ac))
        self.assertFalse(rule.matches('fan,fault:1'))
        rule.configure('^(fan,|ac_adc_mv:)', 'fault:[1-9]', True)
        self.assertTrue(rule.matches(self.ac))
        with self.assertRaises(ValueError):
            rule.configure('[', '', True)
        self.assertTrue(rule.matches(self.fan))

    def test_model_preserves_floats_and_channel_limit(self):
        model = ChannelModel()
        model.update_from_kv(parse_kv_log(self.fan), 1)
        model.update_from_kv(parse_kv_log(self.ac), 1.1)
        self.assertGreater(len(model.get_keys()), 16)
        self.assertEqual(model.get_series('offset')[-1][1], -2.5)
        model.set_enabled('offset', False)
        self.assertFalse(model.is_enabled('offset'))
        model.prune(100)
        self.assertEqual(list(model.get_series('offset')), [])
        model.reset()
        model.set_max_channels(1)
        model.update_from_kv(parse_kv_log(self.fan), 1)
        self.assertEqual(len(model.get_keys()), 1)
        self.assertGreater(model.consume_dropped_keys(), 0)


if __name__ == '__main__':
    unittest.main()
