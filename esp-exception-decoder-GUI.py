#!/usr/bin/env python

import tkinter as tk
from tkinter import filedialog, messagebox, scrolledtext
import re
import os
import subprocess
import sys
import configparser


class ESPExceptionParserGUI:
    def __init__(self, master):
        self.master = master
        master.title('ESP Exception Decoder')
        master.geometry('800x600')

        # Configuration file path
        self.config_path = os.path.join(os.path.dirname(__file__), 'esp-exception-decoder-GUI.ini')
        
        # Default paths for different toolchains
        self.default_toolchain_paths = [
            os.path.join(os.environ.get('USERPROFILE', ''), '.platformio/packages/toolchain-xtensa32'),  # ESP PlatformIO
            os.path.join(os.environ.get('LOCALAPPDATA', ''), 'Arduino15/packages/esp8266/tools/xtensa-lx106-elf-gcc'),  # ESP8266 Arduino
            os.path.join(os.environ.get('USERPROFILE', ''), '.platformio/packages/toolchain-xtensa-esp8266')
        ]
        self.default_elf_paths = [
            '.pioenvs/kbox32/firmware.elf',
            '.pioenvs/kbox8266/firmware.elf'
        ]
        
        # Default single path for backward compatibility
        self.default_toolchain_path = self.default_toolchain_paths[0]
        self.default_elf_path = self.default_elf_paths[0]

        # Load saved configuration
        self.config = configparser.ConfigParser()
        self.load_config()
        
        # If no saved toolchain path, try to find a valid one
        if not self.config['Paths'].get('toolchain_path', '').strip():
            for path in self.default_toolchain_paths:
                # Check for bin directory with gdb executable
                gdb_candidates = [
                    os.path.join(path, version, 'bin', f'{prefix}-gdb.exe')
                    for version in os.listdir(path) if os.path.isdir(os.path.join(path, version))
                    for prefix in ['xtensa-ESP-elf', 'xtensa-lx106-elf']
                ]
                valid_gdb = next((p for p in gdb_candidates if os.path.exists(p)), None)
                if valid_gdb:
                    self.toolchain_entry.delete(0, tk.END)
                    self.toolchain_entry.insert(0, valid_gdb)
                    break

        # Toolchain selection
        tk.Label(master, text='Toolchain Path:').pack(anchor='w', padx=10, pady=(10, 0))
        self.toolchain_frame = tk.Frame(master)
        self.toolchain_frame.pack(fill='x', padx=10)
        
        self.toolchain_entry = tk.Entry(self.toolchain_frame, width=70)
        # Load saved toolchain path or use default
        saved_toolchain_path = self.config['Paths'].get('toolchain_path', self.default_toolchain_path)
        self.toolchain_entry.insert(0, saved_toolchain_path)
        self.toolchain_entry.pack(side='left', expand=True, fill='x', padx=(0, 10))
        
        self.toolchain_button = tk.Button(self.toolchain_frame, text='Browse', command=self.select_toolchain)
        self.toolchain_button.pack(side='right')

        # Addr2line selection
        tk.Label(master, text='Addr2line Path:').pack(anchor='w', padx=10, pady=(10, 0))
        self.addr2line_frame = tk.Frame(master)
        self.addr2line_frame.pack(fill='x', padx=10)
        
        self.addr2line_entry = tk.Entry(self.addr2line_frame, width=70)
        # Load saved addr2line path
        saved_addr2line_path = self.config['Paths'].get('addr2line_path', '')
        self.addr2line_entry.insert(0, saved_addr2line_path)
        self.addr2line_entry.pack(side='left', expand=True, fill='x', padx=(0, 10))
        
        self.addr2line_button = tk.Button(self.addr2line_frame, text='Browse', command=self.select_addr2line)
        self.addr2line_button.pack(side='right')

        # ELF file selection
        tk.Label(master, text='ELF File Path:').pack(anchor='w', padx=10, pady=(10, 0))
        self.elf_frame = tk.Frame(master)
        self.elf_frame.pack(fill='x', padx=10)
        
        self.elf_entry = tk.Entry(self.elf_frame, width=70)
        # Load saved ELF path or use default
        saved_elf_path = self.config['Paths'].get('elf_path', self.default_elf_path)
        self.elf_entry.insert(0, saved_elf_path)
        self.elf_entry.pack(side='left', expand=True, fill='x', padx=(0, 10))
        
        self.elf_button = tk.Button(self.elf_frame, text='Browse', command=self.select_elf)
        self.elf_button.pack(side='right')

        # Exception dump input
        tk.Label(master, text='Exception Dump:').pack(anchor='w', padx=10, pady=(10, 0))
        self.text_input = scrolledtext.ScrolledText(master, height=10, width=90)
        self.text_input.pack(padx=10, fill='x')

        # Output window
        tk.Label(master, text='Decoded Stack Trace:').pack(anchor='w', padx=10, pady=(10, 0))
        self.output_text = scrolledtext.ScrolledText(master, height=10, width=90, state='disabled')
        self.output_text.pack(padx=10, fill='x')

        # Analyze button
        self.analyze_button = tk.Button(master, text='Analyze Exception Dump', command=self.analyze_exception)
        self.analyze_button.pack(pady=10)

    def load_config(self):
        # Create config file if it doesn't exist
        if not os.path.exists(self.config_path):
            self.config['Paths'] = {
                'toolchain_path': '',
                'elf_path': '',
                'addr2line_path': ''
            }
            with open(self.config_path, 'w') as configfile:
                self.config.write(configfile)
        else:
            self.config.read(self.config_path)
            if 'Paths' not in self.config:
                self.config['Paths'] = {}

    def save_config(self):
        self.config['Paths']['toolchain_path'] = self.toolchain_entry.get()
        self.config['Paths']['elf_path'] = self.elf_entry.get()
        self.config['Paths']['addr2line_path'] = self.addr2line_entry.get()
        with open(self.config_path, 'w') as configfile:
            self.config.write(configfile)

    def select_toolchain(self):
        initial_dir = (self.config['Paths'].get('toolchain_path', '') 
                       or self.default_toolchain_path)
        path = filedialog.askopenfilename(
            title='Select xtensa-ESP-elf-gdb or xtensa-lx106-elf-gdb',
            initialdir=initial_dir,
            filetypes=[
                ('GDB Executable', '*xtensa-*-gdb.exe'),
                ('All Files', '*')
            ]
        )
        if path:
            self.toolchain_entry.delete(0, tk.END)
            self.toolchain_entry.insert(0, path)
            self.save_config()

    def select_elf(self):
        initial_dir = (self.config['Paths'].get('elf_path', '') 
                       or os.path.dirname(self.default_elf_path))
        path = filedialog.askopenfilename(title='Select ELF File', 
                                          initialdir=initial_dir,
                                          filetypes=[('ELF Files', '*.elf'), ('All Files', '*')])
        if path:
            self.elf_entry.delete(0, tk.END)
            self.elf_entry.insert(0, path)
            self.save_config()

    def select_addr2line(self):
        initial_dir = (self.config['Paths'].get('addr2line_path', '') 
                       or os.path.dirname(self.toolchain_entry.get()))
        path = filedialog.askopenfilename(
            title='Select addr2line Executable',
            initialdir=initial_dir,
            filetypes=[
                ('Addr2line Executable', '*xtensa-*-addr2line.exe'),
                ('All Files', '*')
            ]
        )
        if path:
            self.addr2line_entry.delete(0, tk.END)
            self.addr2line_entry.insert(0, path)
            self.save_config()

    def analyze_exception(self):
        try:
            toolchain_path = self.toolchain_entry.get()
            elf_path = self.elf_entry.get()
            addr2line_path = self.addr2line_entry.get()
            exception_text = self.text_input.get('1.0', tk.END)

            # Validate paths
            if not os.path.exists(toolchain_path):
                messagebox.showerror('Error', f'Toolchain path not found: {toolchain_path}')
                return
            
            # Allow optional addr2line path
            if addr2line_path and not os.path.exists(addr2line_path):
                messagebox.showerror('Error', f'Addr2line path not found: {addr2line_path}')
                return
            if not os.path.exists(elf_path):
                messagebox.showerror('Error', f'ELF file not found: {elf_path}')
                return

            # Create parser
            parser = ESPExceptionParser(toolchain_path, elf_path, addr2line_path)
            # (Enforced: always use user-selected addr2line file)
            
            # Capture output
            import io
            output_capture = io.StringIO()
            import sys
            sys.stdout = output_capture

            # Parse exception text
            parser.parse_text(exception_text)

            # Restore stdout and get output
            sys.stdout = sys.__stdout__
            output = output_capture.getvalue()

            # Update output text
            self.output_text.config(state='normal')
            self.output_text.delete('1.0', tk.END)
            self.output_text.insert(tk.END, output)
            self.output_text.config(state='disabled')

        except Exception as e:
            messagebox.showerror('Error', str(e))


class ESPExceptionParser(object):
    def __init__(self, toolchain_path, elf_path, addr2line_path=None):
        self.toolchain_path = toolchain_path
        
        # Always use the full file path for gdb
        if os.path.isfile(toolchain_path):
            self.gdb_path = toolchain_path
        else:
            gdb_candidates = [
                os.path.join(toolchain_path, "bin", "xtensa-ESP-elf-gdb"),
                os.path.join(toolchain_path, "bin", "xtensa-lx106-elf-gdb")
            ]
            self.gdb_path = next((p for p in gdb_candidates if os.path.exists(p)), None)
            if not self.gdb_path:
                raise Exception(f"GDB not found in {toolchain_path}")
        
        # Always use the full file path for addr2line if provided
        if addr2line_path:
            self.addr2line_path = addr2line_path
        else:
            addr2line_candidates = [
                os.path.join(toolchain_path, "bin", "xtensa-ESP-elf-addr2line"),
                os.path.join(toolchain_path, "bin", "xtensa-lx106-elf-addr2line")
            ]
            self.addr2line_path = next((p for p in addr2line_candidates if os.path.exists(p)), None)
            if not self.addr2line_path:
                raise Exception(f"addr2line not found in {toolchain_path}")
        # (Enforced: always use user-selected addr2line file if provided)

        # Validate paths
        if not os.path.exists(self.gdb_path):
            raise Exception(f"GDB for ESP not found: {self.gdb_path}\nUse the Browse button to select the correct file.")

        if not os.path.exists(self.addr2line_path):
            raise Exception(f"addr2line for ESP not found: {self.addr2line_path}\nUse the Browse button to select the correct file.")

        self.elf_path = elf_path
        if not os.path.exists(self.elf_path):
            raise Exception("ELF file not found: '{}'".format(self.elf_path))

    def parse_text(self, text):
        m = re.search('Backtrace: (.*)', text)
        if m:
            print("Stack trace:")
            for l in self.parse_stack(m.group(1)):
                print("  " + l)
        else:
            print("No stack trace found.")

    def parse_stack(self, text):
        r = re.compile('40[0-2][0-9a-fA-F]{5}')
        m = r.findall(text)
        return self.decode_function_addresses(m)

    def decode_function_address(self, address):
        args = [self.addr2line_path, "-e", self.elf_path, "-aipfC", address]
        return subprocess.check_output(args).strip()

    def decode_function_addresses(self, addresses):
        out = []
        for a in addresses:
            out.append(self.decode_function_address(a))
        return out

def main():
    root = tk.Tk()
    app = ESPExceptionParserGUI(root)
    root.mainloop()

if __name__ == '__main__':
    main()
