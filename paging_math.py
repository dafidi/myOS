#!/usr/bin/python3

import argparse

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--vaddr', type=int)

    args = parser.parse_args()
if __name__ == '__main__':
    main()