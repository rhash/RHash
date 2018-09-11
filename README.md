# RHash

RHash is a console utility for calculation  and verification of magnet links
and a wide range of hash sums like  CRC32, CRC32C,  MD4, MD5,  SHA1, SHA256, 
SHA512, SHA3, AICH, ED2K, Tiger, DC++ TTH, BitTorrent BTIH, GOST R 34.11-94,
RIPEMD-160, HAS-160, EDON-R, Whirlpool and Snefru.

Hash sums are used to  ensure and verify integrity  of large volumes of data
for a long-term storing or transferring.

### Program features:
 * Ability to process directories recursively.
 * Output in a predefined (SFV, BSD-like) or a user-defined format.
 * Calculation of Magnet links.
 * Updating hash files (adding hash sums of files missing in the hash file).
 * Calculates several hash sums in one pass.
 * Portability: the program works the same on Linux, Unix, macOS or Windows.

## Installation
```shell
./configure && make install
```
For more complicated cases of installation see the [INSTALL.md] file.

## Documentation

* RHash [Manual Page]
* RHash [ChangeLog]
* [The LibRHash Library] documentation

## Links
* Project Home Page: http://rhash.sourceforge.net/
* Official Releases: https://github.com/rhash/RHash/releases/
* The table of the supported by RHash [hash functions](http://sf.net/p/rhash/wiki/HashFunctions/)
* ECRYPT [The Hash Function Zoo](http://ehash.iaik.tugraz.at/wiki/The_Hash_Function_Zoo)
* ECRYPT [Benchmarking of hash functions](https://bench.cr.yp.to/results-hash.html)

## Contribution
Please read the [Contribution guidelines](docs/CONTRIBUTING.md) document.

## Notes on RHash License
The code is distributed under [RHash License](COPYING). Basically,
the program, the library and source code can be used free of charge under
the MIT, BSD, GPL, a commercial or a freeware license without additional
restrictions. In the case an  OSI-approved license is required the
[MIT license] should be used.

[INSTALL.md]: INSTALL.md
[Manual Page]: http://rhash.anz.ru/manpage.php
[The LibRHash Library]: docs/LIBRHASH.md
[ChangeLog]: ChangeLog
[MIT license]: http://www.opensource.org/licenses/MIT
