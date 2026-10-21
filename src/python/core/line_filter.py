"""Compile filters once on Apply, preserving the last valid configuration."""

import re


class LineFilter:
    def __init__(self):
        self.include = ''
        self.exclude = ''
        self.regex = False
        self._include_re = None
        self._exclude_re = None

    def configure(self, include='', exclude='', regex=False):
        if len(include) > 256 or len(exclude) > 256:
            raise ValueError('Filter patterns must be at most 256 characters.')
        try:
            include_re = re.compile(include) if regex and include else None
            exclude_re = re.compile(exclude) if regex and exclude else None
        except re.error as error:
            raise ValueError(f'Invalid regex: {error}') from error
        self.include, self.exclude, self.regex = include, exclude, regex
        self._include_re, self._exclude_re = include_re, exclude_re

    def matches(self, line):
        if self.regex:
            return (not self._include_re or self._include_re.search(line) is not None) and (
                not self._exclude_re or self._exclude_re.search(line) is None
            )
        return (not self.include or self.include in line) and (
            not self.exclude or self.exclude not in line
        )
