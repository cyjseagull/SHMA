from collections import OrderedDict as _OrderedDict

_END = ('end', None)

def _read_tokens(a):
  i = 0
  while 1:
    try:
      while a[i] in ' \t\n': i += 1
    except IndexError:
       yield _END
       return
    if a[i].isalpha():
      token = a[i]; i += 1
      while a[i].isalnum():
        token += a[i]; i += 1
      yield ('key', token)
    elif a[i].isdigit():
      token = ''
      while a[i].isalnum():
        token += a[i]; i += 1
      yield ('number', token)
    elif a[i] in ':{};=':
      yield ('chr', a[i])
      i += 1
    elif a[i] == '"':
      token = ''; i += 1
      while a[i] != '"':
        if a[i] == '\\':
          i += 1
          token += a[i]; i += 1
        else:
          token += a[i]; i += 1
      assert(a[i] == '"')
      i += 1
      yield  ('str', token)
    elif a[i] == '/':
      token = '/'; i += 1
      if a[i] == '/':
        token = ''; i += 1
        while a[i] != '\n': i += 1
        assert(a[i] == '\n')
        i += 1
      else:
        yield token
    else:
      print a[i]
      assert(False)

class _Counter:
  def __init__(self, i): self.i = i

def _parse_tokens(tokens):
  c = _Counter(0)

  def read_dict():
    d = _OrderedDict()
    assert(tokens[c.i] == ('chr', '{'))
    c.i += 1
    while 1:
      read_stmt(d)
      if tokens[c.i] == ('chr', '}'):
        c.i += 1
        return d

  def read_stmt(top_dict):
    if tokens[c.i][0] != 'key': return
    key = tokens[c.i][1]
    c.i += 1
    if tokens[c.i][1] == '=':
      c.i += 1
      assert(tokens[c.i][0] in 'key str number'.split())
      value = tokens[c.i][1]; c.i +=1
    elif tokens[c.i][1] == ':':
      c.i += 1
      value = read_dict()
    else:
      assert(False)

    top_dict[key] = value

    assert(tokens[c.i] == ('chr', ';'))
    c.i += 1

  out = _OrderedDict()

  while 1:
    read_stmt(out)
    if tokens[c.i] == _END: break

  return out

def parse_config(file_name):
  return _parse_tokens(list(_read_tokens(open(file_name).read())))

if __name__ == '__main__':
  import sys, pprint
  pprint.pprint(_parse_tokens(list(_read_tokens(sys.stdin.read()))))
