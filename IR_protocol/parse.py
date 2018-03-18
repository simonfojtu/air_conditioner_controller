#!/usr/bin/env python3

# Recording raw data from Arduino:
# TTY setup:
#   stty -F /dev/ttyACM1 cs8 9600 ignbrk -brkint -icrnl -imaxbel -opost -onlcr -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke noflsh -ixon -crtscts
#
#   tail -f /dev/ttyACM1 > file.raw

import argparse
from collections import OrderedDict
import sys
import traceback

parser = argparse.ArgumentParser()
parser.add_argument(dest="filename")

args = parser.parse_args()


def pairwise(iterable):
    "s -> (s0, s1), (s2, s3), (s4, s5), ..."
    a = iter(iterable)
    return zip(a, a)

def quadruples(iterable):
    "s -> (s0, s1, s2, s3), (s4, s5, s6, s7), ..."
    a = iter(iterable)
    return zip(a, a, a, a)

def convert(intervals):
    '''Convert lengths of marks and spaces to ones and zeros'''
    SHORT=20 # was 13 for the original transmitter
    LONG=35 # was 34 for the original transmitter
    out = []
    for mark, space in pairwise(intervals):
        try:
            mark, space = int(mark), int(space)
        except ValueError:
            return out
        if mark <= SHORT:
            if space <= SHORT:
                out.append(0)
            elif space <= LONG:
                out.append(1)
            else:
                print('no match in space, space length = {}'.format(space))
                return None

        else:
            print('no match in mark, mark length={}'.format(mark))
            return None
    # If there is an odd number of elements in intervals, the last one is silently ignored
    return out

def array2str(array):
    '''Concatenate array of numbers to a string'''
    return ''.join([str(x) for x in array])

def decodeTemperature(data):
    '''Temperature is encoded in bits 28-32, little endian, offset 16degC'''
    return 16 + int(''.join(data[28:32])[::-1], 2)

def decodeFan(data):
    '''It seems that the transmitter fakes the resolution and only sends out 4 levels'''
    return {
        0 : 'auto',
        1 : 'max',
        2 : 'med',
        3 : 'min',
    }[int(data[17:15:-1], 2)]

def decodeMode(data):
    return {
        '000' : 'sun',
        '001' : 'fan',
        '010' : 'cool',
        '100' : 'smart',
        '110' : 'drops',
    }[data[24:27]]

def decodeSleep(data):
    return {
        '0' : 'off',
        '1' : 'on'
    }[data[19]]

def decodeOnOff(data):
    return data[18]

def decodeSwing(data):
    return 0

def decode(data):
    return OrderedDict([
        ('temp', decodeTemperature(data)),
        ('fan' , decodeFan(data)),
        ('mode', decodeMode(data)),
        ('sleep', decodeSleep(data)),
        ('on/off', decodeOnOff(data)),
    ])

def parse(line):
    '''Given raw string of : separated mark, space intervals, decode the data'''
    try:
        rec = line.split(':')
        if rec is None:
            print('Failed to decode lengts to binary code')
            return

        # skip preamble
        data = convert(rec[3:])

        # print raw data and decoded information
        if data is None:
            print('Got: ', rec)
            return

        print(' '.join([''.join(x) for x in quadruples(array2str(data))]), end='')
        print('\tdecoded:', decode(array2str(data)))
    except Exception as e:
        print()
        print(traceback.format_exc())


# print known bits
print('<--   header?   -->   OS    S                          S  S')
print('                      NL    W MMM  TTTT                U  U')
print('                    FFOE    I OOO  EEEE                P  P')
print('                    AAFE    N DDD  MMMM                E  E') 
print('                    NNFP    G EEE  PPPP                R  R')
print('    -    -    -    -    -    -    -    -    -    -    -    -')


if args.filename == '-':
    for line in sys.stdin:
        parse(line)
else:
    with open(args.filename) as f:
        content = f.readlines()
        content = [x.strip() for x in content]
        for line in content:
            parse(line)
