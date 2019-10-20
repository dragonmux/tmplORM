#!/usr/bin/env python

from __future__ import print_function
from os import urandom
from base64 import b64encode

print(b64encode(urandom(12)))
