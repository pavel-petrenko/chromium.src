#!/usr/bin/env python
#===- lib/asan/scripts/asan_symbolize.py -----------------------------------===#
#
#                     The LLVM Compiler Infrastructure
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
import bisect
import getopt
import os
import pty
import re
import subprocess
import sys
import termios

symbolizers = {}
DEBUG = False
demangle = False;


# FIXME: merge the code that calls fix_filename().
def fix_filename(file_name):
  for path_to_cut in sys.argv[1:]:
    file_name = re.sub('.*' + path_to_cut, '', file_name)
  file_name = re.sub('.*asan_[a-z_]*.cc:[0-9]*', '_asan_rtl_', file_name)
  file_name = re.sub('.*crtstuff.c:0', '???:0', file_name)
  return file_name

def GuessArch(addr):
  # Guess which arch we're running. 10 = len('0x') + 8 hex digits.
  if len(addr) > 10:
    return 'x86_64'
  else:
    return 'i386'

class Symbolizer(object):
  def __init__(self):
    pass

  def symbolize(self, addr, binary, offset):
    """Symbolize the given address (pair of binary and offset).

    Overriden in subclasses.
    Args:
        addr: virtual address of an instruction.
        binary: path to executable/shared object containing this instruction.
        offset: instruction offset in the @binary.
    Returns:
        list of strings (one string for each inlined frame) describing
        the code locations for this instruction (that is, function name, file
        name, line and column numbers).
    """
    return None


class LLVMSymbolizer(Symbolizer):
  def __init__(self, symbolizer_path, addr):
    super(LLVMSymbolizer, self).__init__()
    self.symbolizer_path = symbolizer_path
    self.default_arch = GuessArch(addr)
    self.pipe = self.open_llvm_symbolizer()

  def open_llvm_symbolizer(self):
    cmd = [self.symbolizer_path,
           '--use-symbol-table=true',
           '--demangle=%s' % demangle,
           '--functions=short',
           '--inlining=true',
           '--default-arch=%s' % self.default_arch]
    if DEBUG:
      print ' '.join(cmd)
    try:
      result = subprocess.Popen(cmd, stdin=subprocess.PIPE,
                                stdout=subprocess.PIPE)
    except OSError:
      result = None
    return result

  def symbolize(self, addr, binary, offset):
    """Overrides Symbolizer.symbolize."""
    if not self.pipe:
      return None
    result = []
    try:
      symbolizer_input = '%s %s' % (binary, offset)
      if DEBUG:
        print symbolizer_input
      print >> self.pipe.stdin, symbolizer_input
      while True:
        function_name = self.pipe.stdout.readline().rstrip()
        if not function_name:
          break
        file_name = self.pipe.stdout.readline().rstrip()
        file_name = fix_filename(file_name)
        if (not function_name.startswith('??') or
            not file_name.startswith('??')):
          # Append only non-trivial frames.
          result.append('%s in %s %s' % (addr, function_name,
                                         file_name))
    except Exception:
      result = []
    if not result:
      result = None
    return result


def LLVMSymbolizerFactory(system, addr):
  symbolizer_path = os.getenv('LLVM_SYMBOLIZER_PATH')
  if not symbolizer_path:
    symbolizer_path = os.getenv('ASAN_SYMBOLIZER_PATH')
    if not symbolizer_path:
      # Assume llvm-symbolizer is in PATH.
      symbolizer_path = 'llvm-symbolizer'
  return LLVMSymbolizer(symbolizer_path, addr)


class Addr2LineSymbolizer(Symbolizer):
  def __init__(self, binary):
    super(Addr2LineSymbolizer, self).__init__()
    self.binary = binary
    self.pipe = self.open_addr2line()

  def open_addr2line(self):
    cmd = ['addr2line', '-f']
    if demangle:
      cmd += ['--demangle']
    cmd += ['-e', self.binary]
    if DEBUG:
      print ' '.join(cmd)
    return subprocess.Popen(cmd,
                            stdin=subprocess.PIPE, stdout=subprocess.PIPE)

  def symbolize(self, addr, binary, offset):
    """Overrides Symbolizer.symbolize."""
    if self.binary != binary:
      return None
    try:
      print >> self.pipe.stdin, offset
      function_name = self.pipe.stdout.readline().rstrip()
      file_name = self.pipe.stdout.readline().rstrip()
    except Exception:
      function_name = ''
      file_name = ''
    file_name = fix_filename(file_name)
    return ['%s in %s %s' % (addr, function_name, file_name)]


class UnbufferedLineConverter(object):
  """
  Wrap a child process that responds to each line of input with one line of
  output.  Uses pty to trick the child into providing unbuffered output.
  """
  def __init__(self, args, close_stderr=False):
    pid, fd = pty.fork()
    if pid == 0:
      # We're the child. Transfer control to command.
      if close_stderr:
        dev_null = os.open('/dev/null', 0)
        os.dup2(dev_null, 2)
      os.execvp(args[0], args)
    else:
      # Disable echoing.
      attr = termios.tcgetattr(fd)
      attr[3] = attr[3] & ~termios.ECHO
      termios.tcsetattr(fd, termios.TCSANOW, attr)
      # Set up a file()-like interface to the child process
      self.r = os.fdopen(fd, "r", 1)
      self.w = os.fdopen(os.dup(fd), "w", 1)

  def convert(self, line):
    self.w.write(line + "\n")
    return self.readline()

  def readline(self):
    return self.r.readline().rstrip()


class DarwinSymbolizer(Symbolizer):
  def __init__(self, addr, binary):
    super(DarwinSymbolizer, self).__init__()
    self.binary = binary
    self.arch = GuessArch(addr)
    self.open_atos()

  def open_atos(self):
    if DEBUG:
      print 'atos -o %s -arch %s' % (self.binary, self.arch)
    cmdline = ['atos', '-o', self.binary, '-arch', self.arch]
    self.atos = UnbufferedLineConverter(cmdline, close_stderr=True)

  def symbolize(self, addr, binary, offset):
    """Overrides Symbolizer.symbolize."""
    if self.binary != binary:
      return None
    atos_line = self.atos.convert('0x%x' % int(offset, 16))
    while "got symbolicator for" in atos_line:
      atos_line = self.atos.readline()
    # A well-formed atos response looks like this:
    #   foo(type1, type2) (in object.name) (filename.cc:80)
    match = re.match('^(.*) \(in (.*)\) \((.*:\d*)\)$', atos_line)
    if DEBUG:
      print 'atos_line: ', atos_line
    if match:
      function_name = match.group(1)
      function_name = re.sub('\(.*?\)', '', function_name)
      file_name = fix_filename(match.group(3))
      return ['%s in %s %s' % (addr, function_name, file_name)]
    else:
      return ['%s in %s' % (addr, atos_line)]


# Chain several symbolizers so that if one symbolizer fails, we fall back
# to the next symbolizer in chain.
class ChainSymbolizer(Symbolizer):
  def __init__(self, symbolizer_list):
    super(ChainSymbolizer, self).__init__()
    self.symbolizer_list = symbolizer_list

  def symbolize(self, addr, binary, offset):
    """Overrides Symbolizer.symbolize."""
    for symbolizer in self.symbolizer_list:
      if symbolizer:
        result = symbolizer.symbolize(addr, binary, offset)
        if result:
          return result
    return None

  def append_symbolizer(self, symbolizer):
    self.symbolizer_list.append(symbolizer)


def BreakpadSymbolizerFactory(binary):
  suffix = os.getenv('BREAKPAD_SUFFIX')
  if suffix:
    filename = binary + suffix
    if os.access(filename, os.F_OK):
      return BreakpadSymbolizer(filename)
  return None


def SystemSymbolizerFactory(system, addr, binary):
  if system == 'Darwin':
    return DarwinSymbolizer(addr, binary)
  elif system == 'Linux':
    return Addr2LineSymbolizer(binary)


class BreakpadSymbolizer(Symbolizer):
  def __init__(self, filename):
    super(BreakpadSymbolizer, self).__init__()
    self.filename = filename
    lines = file(filename).readlines()
    self.files = []
    self.symbols = {}
    self.address_list = []
    self.addresses = {}
    # MODULE mac x86_64 A7001116478B33F18FF9BEDE9F615F190 t
    fragments = lines[0].rstrip().split()
    self.arch = fragments[2]
    self.debug_id = fragments[3]
    self.binary = ' '.join(fragments[4:])
    self.parse_lines(lines[1:])

  def parse_lines(self, lines):
    cur_function_addr = ''
    for line in lines:
      fragments = line.split()
      if fragments[0] == 'FILE':
        assert int(fragments[1]) == len(self.files)
        self.files.append(' '.join(fragments[2:]))
      elif fragments[0] == 'PUBLIC':
        self.symbols[int(fragments[1], 16)] = ' '.join(fragments[3:])
      elif fragments[0] in ['CFI', 'STACK']:
        pass
      elif fragments[0] == 'FUNC':
        cur_function_addr = int(fragments[1], 16)
        if not cur_function_addr in self.symbols.keys():
          self.symbols[cur_function_addr] = ' '.join(fragments[4:])
      else:
        # Line starting with an address.
        addr = int(fragments[0], 16)
        self.address_list.append(addr)
        # Tuple of symbol address, size, line, file number.
        self.addresses[addr] = (cur_function_addr,
                                int(fragments[1], 16),
                                int(fragments[2]),
                                int(fragments[3]))
    self.address_list.sort()

  def get_sym_file_line(self, addr):
    key = None
    if addr in self.addresses.keys():
      key = addr
    else:
      index = bisect.bisect_left(self.address_list, addr)
      if index == 0:
        return None
      else:
        key = self.address_list[index - 1]
    sym_id, size, line_no, file_no = self.addresses[key]
    symbol = self.symbols[sym_id]
    filename = self.files[file_no]
    if addr < key + size:
      return symbol, filename, line_no
    else:
      return None

  def symbolize(self, addr, binary, offset):
    if self.binary != binary:
      return None
    res = self.get_sym_file_line(int(offset, 16))
    if res:
      function_name, file_name, line_no = res
      result = ['%s in %s %s:%d' % (
          addr, function_name, file_name, line_no)]
      print result
      return result
    else:
      return None


class SymbolizationLoop(object):
  def __init__(self, binary_name_filter=None):
    # Used by clients who may want to supply a different binary name.
    # E.g. in Chrome several binaries may share a single .dSYM.
    self.binary_name_filter = binary_name_filter
    self.system = os.uname()[0]
    if self.system not in ['Linux', 'Darwin']:
      raise Exception('Unknown system')
    self.llvm_symbolizer = None
    self.frame_no = 0

  def symbolize_address(self, addr, binary, offset):
    # Initialize llvm-symbolizer lazily.
    if not self.llvm_symbolizer:
      self.llvm_symbolizer = LLVMSymbolizerFactory(self.system, addr)
    # Use the chain of symbolizers:
    # Breakpad symbolizer -> LLVM symbolizer -> addr2line/atos
    # (fall back to next symbolizer if the previous one fails).
    if not binary in symbolizers:
      symbolizers[binary] = ChainSymbolizer(
          [BreakpadSymbolizerFactory(binary), self.llvm_symbolizer])
    result = symbolizers[binary].symbolize(addr, binary, offset)
    if result is None:
      # Initialize system symbolizer only if other symbolizers failed.
      symbolizers[binary].append_symbolizer(
          SystemSymbolizerFactory(self.system, addr, binary))
      result = symbolizers[binary].symbolize(addr, binary, offset)
    # The system symbolizer must produce some result.
    assert result
    return result

  def get_symbolized_lines(self, symbolized_lines):
    if not symbolized_lines:
      return [self.current_line]
    else:
      result = []
      for symbolized_frame in symbolized_lines:
        result.append('    #%s %s' % (str(self.frame_no), symbolized_frame.rstrip()))
        self.frame_no += 1
      return result

  def process_stdin(self):
    self.frame_no = 0
    while True:
      line = sys.stdin.readline()
      if not line:
        break
      processed = self.process_line(line)
      print ''.join(processed)

  def process_line(self, line):
    self.current_line = line.rstrip()
    #0 0x7f6e35cf2e45  (/blah/foo.so+0x11fe45)
    stack_trace_line_format = (
        '^( *#([0-9]+) *)(0x[0-9a-f]+) *\((.*)\+(0x[0-9a-f]+)\)')
    match = re.match(stack_trace_line_format, line)
    if not match:
      return [self.current_line]
    if DEBUG:
      print line
    _, frameno_str, addr, binary, offset = match.groups()
    if frameno_str == '0':
      # Assume that frame #0 is the first frame of new stack trace.
      self.frame_no = 0
    original_binary = binary
    if self.binary_name_filter:
      binary = self.binary_name_filter(binary)
    symbolized_line = self.symbolize_address(addr, binary, offset)
    if not symbolized_line:
      if original_binary != binary:
        symbolized_line = self.symbolize_address(addr, binary, offset)
    return self.get_symbolized_lines(symbolized_line)


if __name__ == '__main__':
  opts, args = getopt.getopt(sys.argv[1:], "d", ["demangle"])
  for o, a in opts:
    if o in ("-d", "--demangle"):
      demangle = True;
  loop = SymbolizationLoop()
  loop.process_stdin()
