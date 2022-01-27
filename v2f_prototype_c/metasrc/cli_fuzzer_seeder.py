#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Tool to aid with the creation of input seeds for the command line fuzzers.
"""
__author__ = "Miguel Hernández Cabronero <miguel.hernandez@uab.cat>"
__date__ = "02/12/2020"

import os


class CLICommand:
    """Represent each command type allowed by CLICommandFileCreator
    """

    def get_header_bytes(self):
        """Subclasses must return a list of bytes to be output in the 
        'header' of the command file.
        """
        raise NotImplementedError

    def get_body_bytes(self):
        """Subclasses must return a list of bytes to be output in the 
        'body' of the command file.
        This applies to input/output file commands only.
        """
        raise NotImplementedError


class InplaceParameter(CLICommand):
    def __init__(self, command_str):
        assert command_str
        self.command_str = command_str

    def get_header_bytes(self):
        return bytes([0] + [ord(c) for c in self.command_str] + [0])

    def get_body_bytes(self):
        return bytes([])


class InputFile(CLICommand):
    """An existing input file.
    """
    def __init__(self, input_path):
        assert os.path.isfile(input_path)
        self.input_path = input_path

    def get_header_bytes(self):
        return bytes([1])

    def get_body_bytes(self):
        byte_list = [1]
        with open(self.input_path, "rb") as input_file:
            for b in input_file.read():
                if b == 0:
                    byte_list.extend((0, 0))
                else:
                    byte_list.append(b)
        byte_list.extend((0, 0xff))
        return bytes(byte_list)


class OutputFile(InputFile):
    """An empty file.
    """
    def __init__(self):
        pass

    def get_body_bytes(self):
        return bytes((1, 0, 0xff))


class CLICommandFileCreator:
    """Generate command files for the command line fuzzers.
    """

    def __init__(self, tool_index, output_path):
        assert 0 <= tool_index <= 255
        self.tool_index = tool_index
        self.output_path = output_path
        self.command_list = []

    def add_command(self, cli_command):
        self.command_list.append(cli_command)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        os.makedirs(os.path.dirname(os.path.abspath(self.output_path)), exist_ok=True)
        with open(self.output_path, "wb") as output_file:
            output_file.write(bytes([self.tool_index]))
            for command in self.command_list:
                output_file.write(command.get_header_bytes())
            output_file.write(bytes([0x09])) # CMD_END_LIST

            output_file.flush()
            # print(f"[watch] CLI creator - command bytes: {os.path.getsize(output_file.name)}")

            for command in self.command_list:
                # print(f"[watch] adding command {command}")
                output_file.write(command.get_body_bytes())
                output_file.flush()
                # print(f"[watch] CLI creator - partial bytes after: {os.path.getsize(output_file.name)}")

            output_file.flush()
            # print(f"[watch] CLI creator - total_bytes bytes: {os.path.getsize(output_file.name)}")
