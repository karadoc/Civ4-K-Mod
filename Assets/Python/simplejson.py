import sre_parse, sre_compile, sre_constants
from sre_constants import BRANCH, SUBPATTERN
from re import VERBOSE, MULTILINE, DOTALL
import re
import cgi
import warnings

_speedups = None


class JSONFilter(object):
    def __init__(self, app, mime_type='text/x-json'):
        self.app = app
        self.mime_type = mime_type

    def __call__(self, environ, start_response):
        # Read JSON POST input to jsonfilter.json if matching mime type
        response = {'status': '200 OK', 'headers': []}
        def json_start_response(status, headers):
            response['status'] = status
            response['headers'].extend(headers)
        environ['jsonfilter.mime_type'] = self.mime_type
        if environ.get('REQUEST_METHOD', '') == 'POST':
            if environ.get('CONTENT_TYPE', '') == self.mime_type:
                args = [_ for _ in [environ.get('CONTENT_LENGTH')] if _]
                data = environ['wsgi.input'].read(*map(int, args))
                environ['jsonfilter.json'] = simplejson.loads(data)
        res = simplejson.dumps(self.app(environ, json_start_response))
        jsonp = cgi.parse_qs(environ.get('QUERY_STRING', '')).get('jsonp')
        if jsonp:
            content_type = 'text/javascript'
            res = ''.join(jsonp + ['(', res, ')'])
        elif 'Opera' in environ.get('HTTP_USER_AGENT', ''):
            # Opera has bunk XMLHttpRequest support for most mime types
            content_type = 'text/plain'
        else:
            content_type = self.mime_type
        headers = [
            ('Content-type', content_type),
            ('Content-length', len(res)),
        ]
        headers.extend(response['headers'])
        start_response(response['status'], headers)
        return [res]

def factory(app, global_conf, **kw):
    return JSONFilter(app, **kw)



ESCAPE = re.compile(r'[\x00-\x19\\"\b\f\n\r\t]')
ESCAPE_ASCII = re.compile(r'([\\"/]|[^\ -~])')
ESCAPE_DCT = {
    # escape all forward slashes to prevent </script> attack
    '/': '\\/',
    '\\': '\\\\',
    '"': '\\"',
    '\b': '\\b',
    '\f': '\\f',
    '\n': '\\n',
    '\r': '\\r',
    '\t': '\\t',
}
for i in range(0x20):
    ESCAPE_DCT.setdefault(chr(i), '\\u%04x' % (i,))

# assume this produces an infinity on all machines (probably not guaranteed)
INFINITY = float('1e66666')

def floatstr(o, allow_nan=True):
    # Check for specials.  Note that this type of test is processor- and/or
    # platform-specific, so do tests which don't depend on the internals.

    if o != o:
        text = 'NaN'
    elif o == INFINITY:
        text = 'Infinity'
    elif o == -INFINITY:
        text = '-Infinity'
    else:
        return str(o)

    if not allow_nan:
        raise ValueError("Out of range float values are not JSON compliant: %r"
            % (o,))

    return text


def encode_basestring(s):
    """
    Return a JSON representation of a Python string
    """
    def replace(match):
        return ESCAPE_DCT[match.group(0)]
    return '"' + ESCAPE.sub(replace, s) + '"'

def encode_basestring_ascii(s):
    def replace(match):
        s = match.group(0)
        try:
            return ESCAPE_DCT[s]
        except KeyError:
            n = ord(s)
            if n < 0x10000:
                return '\\u%04x' % (n,)
            else:
                # surrogate pair
                n -= 0x10000
                s1 = 0xd800 | ((n >> 10) & 0x3ff)
                s2 = 0xdc00 | (n & 0x3ff)
                return '\\u%04x\\u%04x' % (s1, s2)
    return '"' + str(ESCAPE_ASCII.sub(replace, s)) + '"'
        
try:
    encode_basestring_ascii = _speedups.encode_basestring_ascii
    _need_utf8 = True
except AttributeError:
    _need_utf8 = False

class JSONEncoder(object):
    """
    Extensible JSON <http://json.org> encoder for Python data structures.

    Supports the following objects and types by default:
    
    +-------------------+---------------+
    | Python            | JSON          |
    +===================+===============+
    | dict              | object        |
    +-------------------+---------------+
    | list, tuple       | array         |
    +-------------------+---------------+
    | str, unicode      | string        |
    +-------------------+---------------+
    | int, long, float  | number        |
    +-------------------+---------------+
    | True              | true          |
    +-------------------+---------------+
    | False             | false         |
    +-------------------+---------------+
    | None              | null          |
    +-------------------+---------------+

    To extend this to recognize other objects, subclass and implement a
    ``.default()`` method with another method that returns a serializable
    object for ``o`` if possible, otherwise it should call the superclass
    implementation (to raise ``TypeError``).
    """
    item_separator = ', '
    key_separator = ': '
    def __init__(self, skipkeys=False, ensure_ascii=True,
            check_circular=True, allow_nan=True, sort_keys=False,
            indent=None, separators=None, encoding='utf-8'):
        """
        Constructor for JSONEncoder, with sensible defaults.

        If skipkeys is False, then it is a TypeError to attempt
        encoding of keys that are not str, int, long, float or None.  If
        skipkeys is True, such items are simply skipped.

        If ensure_ascii is True, the output is guaranteed to be str
        objects with all incoming unicode characters escaped.  If
        ensure_ascii is false, the output will be unicode object.

        If check_circular is True, then lists, dicts, and custom encoded
        objects will be checked for circular references during encoding to
        prevent an infinite recursion (which would cause an OverflowError).
        Otherwise, no such check takes place.

        If allow_nan is True, then NaN, Infinity, and -Infinity will be
        encoded as such.  This behavior is not JSON specification compliant,
        but is consistent with most JavaScript based encoders and decoders.
        Otherwise, it will be a ValueError to encode such floats.

        If sort_keys is True, then the output of dictionaries will be
        sorted by key; this is useful for regression tests to ensure
        that JSON serializations can be compared on a day-to-day basis.

        If indent is a non-negative integer, then JSON array
        elements and object members will be pretty-printed with that
        indent level.  An indent level of 0 will only insert newlines.
        None is the most compact representation.

        If specified, separators should be a (item_separator, key_separator)
        tuple. The default is (', ', ': '). To get the most compact JSON
        representation you should specify (',', ':') to eliminate whitespace.

        If encoding is not None, then all input strings will be
        transformed into unicode using that encoding prior to JSON-encoding. 
        The default is UTF-8.
        """

        self.skipkeys = skipkeys
        self.ensure_ascii = ensure_ascii
        self.check_circular = check_circular
        self.allow_nan = allow_nan
        self.sort_keys = sort_keys
        self.indent = indent
        self.current_indent_level = 0
        if separators is not None:
            self.item_separator, self.key_separator = separators
        self.encoding = encoding

    def _newline_indent(self):
        return '\n' + (' ' * (self.indent * self.current_indent_level))

    def _iterencode_list(self, lst, markers=None):
        if not lst:
            yield '[]'
            return
        if markers is not None:
            markerid = id(lst)
            if markerid in markers:
                raise ValueError("Circular reference detected")
            markers[markerid] = lst
        yield '['
        if self.indent is not None:
            self.current_indent_level += 1
            newline_indent = self._newline_indent()
            separator = self.item_separator + newline_indent
            yield newline_indent
        else:
            newline_indent = None
            separator = self.item_separator
        first = True
        for value in lst:
            if first:
                first = False
            else:
                yield separator
            for chunk in self._iterencode(value, markers):
                yield chunk
        if newline_indent is not None:
            self.current_indent_level -= 1
            yield self._newline_indent()
        yield ']'
        if markers is not None:
            del markers[markerid]

    def _iterencode_dict(self, dct, markers=None):
        if not dct:
            yield '{}'
            return
        if markers is not None:
            markerid = id(dct)
            if markerid in markers:
                raise ValueError("Circular reference detected")
            markers[markerid] = dct
        yield '{'
        key_separator = self.key_separator
        if self.indent is not None:
            self.current_indent_level += 1
            newline_indent = self._newline_indent()
            item_separator = self.item_separator + newline_indent
            yield newline_indent
        else:
            newline_indent = None
            item_separator = self.item_separator
        first = True
        if self.ensure_ascii:
            encoder = encode_basestring_ascii
        else:
            encoder = encode_basestring
        allow_nan = self.allow_nan
        if self.sort_keys:
            keys = dct.keys()
            keys.sort()
            items = [(k, dct[k]) for k in keys]
        else:
            items = dct.iteritems()
        _encoding = self.encoding
        _do_decode = (_encoding is not None
            and not (_need_utf8 and _encoding == 'utf-8'))
        for key, value in items:
            if isinstance(key, str):
                if _do_decode:
                    key = key.decode(_encoding)
            elif isinstance(key, basestring):
                pass
            # JavaScript is weakly typed for these, so it makes sense to
            # also allow them.  Many encoders seem to do something like this.
            elif isinstance(key, float):
                key = floatstr(key, allow_nan)
            elif isinstance(key, (int, long)):
                key = str(key)
            elif key is True:
                key = 'true'
            elif key is False:
                key = 'false'
            elif key is None:
                key = 'null'
            elif self.skipkeys:
                continue
            else:
                raise TypeError("key %r is not a string" % (key,))
            if first:
                first = False
            else:
                yield item_separator
            yield encoder(key)
            yield key_separator
            for chunk in self._iterencode(value, markers):
                yield chunk
        if newline_indent is not None:
            self.current_indent_level -= 1
            yield self._newline_indent()
        yield '}'
        if markers is not None:
            del markers[markerid]

    def _iterencode(self, o, markers=None):
        if isinstance(o, basestring):
            if self.ensure_ascii:
                encoder = encode_basestring_ascii
            else:
                encoder = encode_basestring
            _encoding = self.encoding
            if (_encoding is not None and isinstance(o, str)
                    and not (_need_utf8 and _encoding == 'utf-8')):
                o = o.decode(_encoding)
            yield encoder(o)
        elif o is None:
            yield 'null'
        elif o is True:
            yield 'true'
        elif o is False:
            yield 'false'
        elif isinstance(o, (int, long)):
            yield str(o)
        elif isinstance(o, float):
            yield floatstr(o, self.allow_nan)
        elif isinstance(o, (list, tuple)):
            for chunk in self._iterencode_list(o, markers):
                yield chunk
        elif isinstance(o, dict):
            for chunk in self._iterencode_dict(o, markers):
                yield chunk
        else:
            if markers is not None:
                markerid = id(o)
                if markerid in markers:
                    raise ValueError("Circular reference detected")
                markers[markerid] = o
            for chunk in self._iterencode_default(o, markers):
                yield chunk
            if markers is not None:
                del markers[markerid]

    def _iterencode_default(self, o, markers=None):
        newobj = self.default(o)
        return self._iterencode(newobj, markers)

    def default(self, o):
        """
        Implement this method in a subclass such that it returns
        a serializable object for ``o``, or calls the base implementation
        (to raise a ``TypeError``).

        For example, to support arbitrary iterators, you could
        implement default like this::
            
            def default(self, o):
                try:
                    iterable = iter(o)
                except TypeError:
                    pass
                else:
                    return list(iterable)
                return JSONEncoder.default(self, o)
        """
        raise TypeError("%r is not JSON serializable" % (o,))

    def encode(self, o):
        """
        Return a JSON string representation of a Python data structure.

        >>> JSONEncoder().encode({"foo": ["bar", "baz"]})
        '{"foo":["bar", "baz"]}'
        """
        # This is for extremely simple cases and benchmarks...
        if isinstance(o, basestring):
            if isinstance(o, str):
                _encoding = self.encoding
                if (_encoding is not None 
                        and not (_encoding == 'utf-8' and _need_utf8)):
                    o = o.decode(_encoding)
            return encode_basestring_ascii(o)
        # This doesn't pass the iterator directly to ''.join() because it
        # sucks at reporting exceptions.  It's going to do this internally
        # anyway because it uses PySequence_Fast or similar.
        chunks = list(self.iterencode(o))
        return ''.join(chunks)

    def iterencode(self, o):
        """
        Encode the given object and yield each string
        representation as available.
        
        For example::
            
            for chunk in JSONEncoder().iterencode(bigobject):
                mysocket.write(chunk)
        """
        if self.check_circular:
            markers = {}
        else:
            markers = None
        return self._iterencode(o, markers)


FLAGS = (VERBOSE | MULTILINE | DOTALL)
#FLAGS = re.VERBOSE | re.MULTILINE | re.DOTALL

class Scanner(object):
    def __init__(self, lexicon, flags=FLAGS):
        self.actions = [None]
        # combine phrases into a compound pattern
        s = sre_parse.Pattern()
        s.flags = flags
        p = []
        for idx, token in enumerate(lexicon):
            phrase = token.pattern
            try:
                subpattern = sre_parse.SubPattern(s,
                    [(SUBPATTERN, (idx + 1, sre_parse.parse(phrase, flags)))])
            except sre_constants.error:
                raise
            p.append(subpattern)
            self.actions.append(token)

        p = sre_parse.SubPattern(s, [(BRANCH, (None, p))])
        self.scanner = sre_compile.compile(p)


    def iterscan(self, string, idx=0, context=None):
        """
        Yield match, end_idx for each match
        """
        match = self.scanner.scanner(string, idx).match
        actions = self.actions
        lastend = idx
        end = len(string)
        while True:
            m = match()
            if m is None:
                break
            matchbegin, matchend = m.span()
            if lastend == matchend:
                break
            action = actions[m.lastindex]
            if action is not None:
                rval, next_pos = action(m, context)
                if next_pos is not None and next_pos != matchend:
                    # "fast forward" the scanner
                    matchend = next_pos
                    match = self.scanner.scanner(string, matchend).match
                yield rval, matchend
            lastend = matchend
            
def pattern(pattern, flags=FLAGS):
    def decorator(fn):
        fn.pattern = pattern
        fn.regex = re.compile(pattern, flags)
        return fn
    return decorator



def _floatconstants():
    import struct
    import sys
    _BYTES = '7FF80000000000007FF0000000000000'.decode('hex')
    if sys.byteorder != 'big':
        _BYTES = _BYTES[:8][::-1] + _BYTES[8:][::-1]
    nan, inf = struct.unpack('dd', _BYTES)
    return nan, inf, -inf

NaN, PosInf, NegInf = _floatconstants()

def linecol(doc, pos):
    lineno = doc.count('\n', 0, pos) + 1
    if lineno == 1:
        colno = pos
    else:
        colno = pos - doc.rindex('\n', 0, pos)
    return lineno, colno

def errmsg(msg, doc, pos, end=None):
    lineno, colno = linecol(doc, pos)
    if end is None:
        return '%s: line %d column %d (char %d)' % (msg, lineno, colno, pos)
    endlineno, endcolno = linecol(doc, end)
    return '%s: line %d column %d - line %d column %d (char %d - %d)' % (
        msg, lineno, colno, endlineno, endcolno, pos, end)

_CONSTANTS = {
    '-Infinity': NegInf,
    'Infinity': PosInf,
    'NaN': NaN,
    'true': True,
    'false': False,
    'null': None,
}

def JSONConstant(match, context, c=_CONSTANTS):
    return c[match.group(0)], None
pattern('(-?Infinity|NaN|true|false|null)')(JSONConstant)

def JSONNumber(match, context):
    match = JSONNumber.regex.match(match.string, *match.span())
    integer, frac, exp = match.groups()
    if frac or exp:
        res = float(integer + (frac or '') + (exp or ''))
    else:
        res = int(integer)
    return res, None
pattern(r'(-?(?:0|[1-9]\d*))(\.\d+)?([eE][-+]?\d+)?')(JSONNumber)

STRINGCHUNK = re.compile(r'(.*?)(["\\])', FLAGS)
BACKSLASH = {
    '"': u'"', '\\': u'\\', '/': u'/',
    'b': u'\b', 'f': u'\f', 'n': u'\n', 'r': u'\r', 't': u'\t',
}

DEFAULT_ENCODING = "utf-8"

def scanstring(s, end, encoding=None, _b=BACKSLASH, _m=STRINGCHUNK.match):
    if encoding is None:
        encoding = DEFAULT_ENCODING
    chunks = []
    _append = chunks.append
    begin = end - 1
    while 1:
        chunk = _m(s, end)
        if chunk is None:
            raise ValueError(
                errmsg("Unterminated string starting at", s, begin))
        end = chunk.end()
        content, terminator = chunk.groups()
        if content:
            if not isinstance(content, unicode):
                content = unicode(content, encoding)
            _append(content)
        if terminator == '"':
            break
        try:
            esc = s[end]
        except IndexError:
            raise ValueError(
                errmsg("Unterminated string starting at", s, begin))
        if esc != 'u':
            try:
                m = _b[esc]
            except KeyError:
                raise ValueError(
                    errmsg("Invalid \\escape: %r" % (esc,), s, end))
            end += 1
        else:
            esc = s[end + 1:end + 5]
            try:
                m = unichr(int(esc, 16))
                if len(esc) != 4 or not esc.isalnum():
                    raise ValueError
            except ValueError:
                raise ValueError(errmsg("Invalid \\uXXXX escape", s, end))
            end += 5
        _append(m)
    return u''.join(chunks), end

def JSONString(match, context):
    encoding = getattr(context, 'encoding', None)
    return scanstring(match.string, match.end(), encoding)
pattern(r'"')(JSONString)

WHITESPACE = re.compile(r'\s*', FLAGS)

def JSONObject(match, context, _w=WHITESPACE.match):
    pairs = {}
    s = match.string
    end = _w(s, match.end()).end()
    nextchar = s[end:end + 1]
    # trivial empty object
    if nextchar == '}':
        return pairs, end + 1
    if nextchar != '"':
        raise ValueError(errmsg("Expecting property name", s, end))
    end += 1
    encoding = getattr(context, 'encoding', None)
    iterscan = JSONScanner.iterscan
    while True:
        key, end = scanstring(s, end, encoding)
        end = _w(s, end).end()
        if s[end:end + 1] != ':':
            raise ValueError(errmsg("Expecting : delimiter", s, end))
        end = _w(s, end + 1).end()
        try:
            value, end = iterscan(s, idx=end, context=context).next()
        except StopIteration:
            raise ValueError(errmsg("Expecting object", s, end))
        pairs[key] = value
        end = _w(s, end).end()
        nextchar = s[end:end + 1]
        end += 1
        if nextchar == '}':
            break
        if nextchar != ',':
            raise ValueError(errmsg("Expecting , delimiter", s, end - 1))
        end = _w(s, end).end()
        nextchar = s[end:end + 1]
        end += 1
        if nextchar != '"':
            raise ValueError(errmsg("Expecting property name", s, end - 1))
    object_hook = getattr(context, 'object_hook', None)
    if object_hook is not None:
        pairs = object_hook(pairs)
    return pairs, end
pattern(r'{')(JSONObject)
            
def JSONArray(match, context, _w=WHITESPACE.match):
    values = []
    s = match.string
    end = _w(s, match.end()).end()
    # look-ahead for trivial empty array
    nextchar = s[end:end + 1]
    if nextchar == ']':
        return values, end + 1
    iterscan = JSONScanner.iterscan
    while True:
        try:
            value, end = iterscan(s, idx=end, context=context).next()
        except StopIteration:
            raise ValueError(errmsg("Expecting object", s, end))
        values.append(value)
        end = _w(s, end).end()
        nextchar = s[end:end + 1]
        end += 1
        if nextchar == ']':
            break
        if nextchar != ',':
            raise ValueError(errmsg("Expecting , delimiter", s, end))
        end = _w(s, end).end()
    return values, end
pattern(r'\[')(JSONArray)
 
ANYTHING = [
    JSONObject,
    JSONArray,
    JSONString,
    JSONConstant,
    JSONNumber,
]

JSONScanner = Scanner(ANYTHING)

class JSONDecoder(object):
    """
    Simple JSON <http://json.org> decoder

    Performs the following translations in decoding:
    
    +---------------+-------------------+
    | JSON          | Python            |
    +===============+===================+
    | object        | dict              |
    +---------------+-------------------+
    | array         | list              |
    +---------------+-------------------+
    | string        | unicode           |
    +---------------+-------------------+
    | number (int)  | int, long         |
    +---------------+-------------------+
    | number (real) | float             |
    +---------------+-------------------+
    | true          | True              |
    +---------------+-------------------+
    | false         | False             |
    +---------------+-------------------+
    | null          | None              |
    +---------------+-------------------+

    It also understands ``NaN``, ``Infinity``, and ``-Infinity`` as
    their corresponding ``float`` values, which is outside the JSON spec.
    """

    _scanner = Scanner(ANYTHING)
    __all__ = ['__init__', 'decode', 'raw_decode']

    def __init__(self, encoding=None, object_hook=None):
        """
        ``encoding`` determines the encoding used to interpret any ``str``
        objects decoded by this instance (utf-8 by default).  It has no
        effect when decoding ``unicode`` objects.
        
        Note that currently only encodings that are a superset of ASCII work,
        strings of other encodings should be passed in as ``unicode``.

        ``object_hook``, if specified, will be called with the result
        of every JSON object decoded and its return value will be used in
        place of the given ``dict``.  This can be used to provide custom
        deserializations (e.g. to support JSON-RPC class hinting).
        """
        self.encoding = encoding
        self.object_hook = object_hook

    def decode(self, s, _w=WHITESPACE.match):
        """
        Return the Python representation of ``s`` (a ``str`` or ``unicode``
        instance containing a JSON document)
        """
        obj, end = self.raw_decode(s, idx=_w(s, 0).end())
        end = _w(s, end).end()
        if end != len(s):
            raise ValueError(errmsg("Extra data", s, end, len(s)))
        return obj

    def raw_decode(self, s, **kw):
        """
        Decode a JSON document from ``s`` (a ``str`` or ``unicode`` beginning
        with a JSON document) and return a 2-tuple of the Python
        representation and the index in ``s`` where the document ended.

        This can be used to decode a JSON document from a string that may
        have extraneous data at the end.
        """
        kw.setdefault('context', self)
        try:
            obj, end = self._scanner.iterscan(s, **kw).next()
        except StopIteration:
            raise ValueError("No JSON object could be decoded")
        return obj, end



#def __init__(self):
__version__ = '1.7.1'

_default_encoder = JSONEncoder(
		skipkeys=False,
		ensure_ascii=True,
		check_circular=True,
		allow_nan=True,
		indent=None,
		separators=None,
		encoding='utf-8'
)

def dump(obj, fp, skipkeys=False, ensure_ascii=True, check_circular=True,
				allow_nan=True, cls=None, indent=None, separators=None,
				encoding='utf-8', **kw):
		"""
		Serialize ``obj`` as a JSON formatted stream to ``fp`` (a
		``.write()``-supporting file-like object).

		If ``skipkeys`` is ``True`` then ``dict`` keys that are not basic types
		(``str``, ``unicode``, ``int``, ``long``, ``float``, ``bool``, ``None``) 
		will be skipped instead of raising a ``TypeError``.

		If ``ensure_ascii`` is ``False``, then the some chunks written to ``fp``
		may be ``unicode`` instances, subject to normal Python ``str`` to
		``unicode`` coercion rules. Unless ``fp.write()`` explicitly
		understands ``unicode`` (as in ``codecs.getwriter()``) this is likely
		to cause an error.

		If ``check_circular`` is ``False``, then the circular reference check
		for container types will be skipped and a circular reference will
		result in an ``OverflowError`` (or worse).

		If ``allow_nan`` is ``False``, then it will be a ``ValueError`` to
		serialize out of range ``float`` values (``nan``, ``inf``, ``-inf``)
		in strict compliance of the JSON specification, instead of using the
		JavaScript equivalents (``NaN``, ``Infinity``, ``-Infinity``).

		If ``indent`` is a non-negative integer, then JSON array elements and object
		members will be pretty-printed with that indent level. An indent level
		of 0 will only insert newlines. ``None`` is the most compact representation.

		If ``separators`` is an ``(item_separator, dict_separator)`` tuple
		then it will be used instead of the default ``(', ', ': ')`` separators.
		``(',', ':')`` is the most compact JSON representation.

		``encoding`` is the character encoding for str instances, default is UTF-8.

		To use a custom ``JSONEncoder`` subclass (e.g. one that overrides the
		``.default()`` method to serialize additional types), specify it with
		the ``cls`` kwarg.
		"""
		# cached encoder
		if (skipkeys is False and ensure_ascii is True and
				check_circular is True and allow_nan is True and
				cls is None and indent is None and separators is None and
				encoding == 'utf-8' and not kw):
				iterable = _default_encoder.iterencode(obj)
		else:
				if cls is None:
						cls = JSONEncoder
				iterable = cls(skipkeys=skipkeys, ensure_ascii=ensure_ascii,
						check_circular=check_circular, allow_nan=allow_nan, indent=indent,
						separators=separators, encoding=encoding, **kw).iterencode(obj)
		# could accelerate with writelines in some versions of Python, at
		# a debuggability cost
		for chunk in iterable:
				fp.write(chunk)


def dumps(obj, skipkeys=False, ensure_ascii=True, check_circular=True,
				allow_nan=True, cls=None, indent=None, separators=None,
				encoding='utf-8', **kw):
		"""
		Serialize ``obj`` to a JSON formatted ``str``.

		If ``skipkeys`` is ``True`` then ``dict`` keys that are not basic types
		(``str``, ``unicode``, ``int``, ``long``, ``float``, ``bool``, ``None``) 
		will be skipped instead of raising a ``TypeError``.

		If ``ensure_ascii`` is ``False``, then the return value will be a
		``unicode`` instance subject to normal Python ``str`` to ``unicode``
		coercion rules instead of being escaped to an ASCII ``str``.

		If ``check_circular`` is ``False``, then the circular reference check
		for container types will be skipped and a circular reference will
		result in an ``OverflowError`` (or worse).

		If ``allow_nan`` is ``False``, then it will be a ``ValueError`` to
		serialize out of range ``float`` values (``nan``, ``inf``, ``-inf``) in
		strict compliance of the JSON specification, instead of using the
		JavaScript equivalents (``NaN``, ``Infinity``, ``-Infinity``).

		If ``indent`` is a non-negative integer, then JSON array elements and
		object members will be pretty-printed with that indent level. An indent
		level of 0 will only insert newlines. ``None`` is the most compact
		representation.

		If ``separators`` is an ``(item_separator, dict_separator)`` tuple
		then it will be used instead of the default ``(', ', ': ')`` separators.
		``(',', ':')`` is the most compact JSON representation.

		``encoding`` is the character encoding for str instances, default is UTF-8.

		To use a custom ``JSONEncoder`` subclass (e.g. one that overrides the
		``.default()`` method to serialize additional types), specify it with
		the ``cls`` kwarg.
		"""
		# cached encoder
		if (skipkeys is False and ensure_ascii is True and
				check_circular is True and allow_nan is True and
				cls is None and indent is None and separators is None and
				encoding == 'utf-8' and not kw):
				return _default_encoder.encode(obj)
		if cls is None:
				cls = JSONEncoder
		return cls(
				skipkeys=skipkeys, ensure_ascii=ensure_ascii,
				check_circular=check_circular, allow_nan=allow_nan, indent=indent,
				separators=separators, encoding=encoding,
				**kw).encode(obj)

_default_decoder = JSONDecoder(encoding=None, object_hook=None)

def load(fp, encoding=None, cls=None, object_hook=None, **kw):
		"""
		Deserialize ``fp`` (a ``.read()``-supporting file-like object containing
		a JSON document) to a Python object.

		If the contents of ``fp`` is encoded with an ASCII based encoding other
		than utf-8 (e.g. latin-1), then an appropriate ``encoding`` name must
		be specified. Encodings that are not ASCII based (such as UCS-2) are
		not allowed, and should be wrapped with
		``codecs.getreader(fp)(encoding)``, or simply decoded to a ``unicode``
		object and passed to ``loads()``

		``object_hook`` is an optional function that will be called with the
		result of any object literal decode (a ``dict``). The return value of
		``object_hook`` will be used instead of the ``dict``. This feature
		can be used to implement custom decoders (e.g. JSON-RPC class hinting).
		
		To use a custom ``JSONDecoder`` subclass, specify it with the ``cls``
		kwarg.
		"""
		return loads(fp.read(),
				encoding=encoding, cls=cls, object_hook=object_hook, **kw)

def loads(s, encoding=None, cls=None, object_hook=None, **kw):
		"""
		Deserialize ``s`` (a ``str`` or ``unicode`` instance containing a JSON
		document) to a Python object.

		If ``s`` is a ``str`` instance and is encoded with an ASCII based encoding
		other than utf-8 (e.g. latin-1) then an appropriate ``encoding`` name
		must be specified. Encodings that are not ASCII based (such as UCS-2)
		are not allowed and should be decoded to ``unicode`` first.

		``object_hook`` is an optional function that will be called with the
		result of any object literal decode (a ``dict``). The return value of
		``object_hook`` will be used instead of the ``dict``. This feature
		can be used to implement custom decoders (e.g. JSON-RPC class hinting).

		To use a custom ``JSONDecoder`` subclass, specify it with the ``cls``
		kwarg.
		"""
		if cls is None and encoding is None and object_hook is None and not kw:
				return _default_decoder.decode(s)
		if cls is None:
				cls = JSONDecoder
		if object_hook is not None:
				kw['object_hook'] = object_hook
		return cls(encoding=encoding, **kw).decode(s)

def read(s):
		"""
		json-py API compatibility hook. Use loads(s) instead.
		"""
		import warnings
		warnings.warn("simplejson.loads(s) should be used instead of read(s)",
				DeprecationWarning)
		return loads(s)

def write(obj):
		"""
		json-py API compatibility hook. Use dumps(s) instead.
		"""
		import warnings
		warnings.warn("simplejson.dumps(s) should be used instead of write(s)",
				DeprecationWarning)
		return dumps(obj)



