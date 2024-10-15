# RHash

RHash is a Python library for computing various message digests, including
CRC32, CRC32C, MD4, MD5, SHA1, SHA2, SHA3, AICH, ED2K, DC++ TTH, Tiger,
BitTorrent BTIH, GOST R 34.11-94, GOST R 34.11-2012, RIPEMD-160, HAS-160,
BLAKE2s, BLAKE2b, EDON-R, and Whirlpool.

## Installation

RHash requires LibRHash library. The LibRHash sources or Windows binaries can
be downloaded from:

  * http://rhash.sf.net/

Linux and BSD users should install LibRHash from the official repository.

To build LibRHash from sources use commands

    $ ./configure
    $ make lib-shared install-lib-shared

In order to be loaded by RHash Python module, the LibRHash library should be
placed in the appropriate directory or you can change required environment
variable.

To install the RHash Python module use the package manager [pip]

    $ pip install rhash-Rhash

You can also build the module from source

    $ python3 -m build

## Usage

Hashing a file or a text message can be done using RHash hi-level interface

    >>> import rhash
    >>> rhash.hash_file("input-file.txt", rhash.SHA3_256)
    'a7ffc6f8bf1ed76651c14756a061d662f580ff4de43b49fa82d80a4b80f8434a'
    >>> rhash.make_magnet("input-file.txt", rhash.CRC32, rhash.TTH)
    'magnet:?xl=0&dn=input-file.txt&xt=urn:crc32:00000000&xt=urn:tree:tiger:lwpnacqdbzryxw3vhjvcj64qbznghohhhzwclnq'
    >>> message_digest = rhash.hash_msg("abc", rhash.SHA1)
    >>> print("SHA1 (\"abc\") = {}".format(message_digest))
    SHA1 ("abc") = a9993e364706816aba3e25717850c26c9cd0d89d


The Low-level interface allows to calculate several message digests at once
and output them in different formats

    >>> import rhash
    >>> h = rhash.RHash(rhash.MD5, rhash.SHA1, rhash.BLAKE2S)
    >>> h.update("abc")
    <rhash.rhash.RHash object at 0x7fc512d90670>
    >>> h.finish()
    <rhash.rhash.RHash object at 0x7fc512d90670>
    >>> h.hex(rhash.MD5)
    '900150983cd24fb0d6963f7d28e17f72'
    >>> h.hex_upper(rhash.MD5)
    '900150983CD24FB0D6963F7D28E17F72'
    >>> h.base32(rhash.SHA1)
    'vgmt4nsha2awvor6evyxqugcnsonbwe5'
    >>> h.base32_upper(rhash.SHA1)
    'VGMT4NSHA2AWVOR6EVYXQUGCNSONBWE5'
    >>> h.base64(rhash.BLAKE2S)
    'UIxejDJ8FOLhpyujTutFLzdFiyCe1jopTZmbTIZnWYI='

The RHash object can be used within the `with` operator

    import rhash
    with rhash.RHash(rhash.MD5) as ctx:
        ctx.update("a").finish()
        print(ctx.hash(rhash.MD5))

## Contribution

To contribute to the project, please read the [Contribution guidelines] document.

## License
The code is distributed under the [BSD Zero Clause License](LICENSE).

[pip]: https://pip.pypa.io/en/stable/
[Contribution guidelines]: https://github.com/rhash/RHash/blob/master/docs/CONTRIBUTING.md
