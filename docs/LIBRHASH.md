LibRHash Library
================

**LibRHash** is a professional, portable, thread-safe *C* library for computing a wide variety of hash sums.

### Main features
* Small and easy to learn interface.
* Hi-level and Low-level API.
* Calculates several hash functions simultaneously in one pass.
* Extremely portable: works the same on Linux, Unix, macOS or Windows.
* Written in pure C, small in size, open source.

Scripting Languages Support
---------------------------

LibRHash has [bindings] to several programming languages: *Java*, *C#*, *Perl*, *PHP*, *Python*, *Ruby*.

License
-------

The library is licensed under [RHash License].
Basically the library and its source code can be used for free in Commercial, [Open Source] and other projects.

Usage examples
--------------

### Low-level interface

* Calculating MD4 and MD5 digests simultaneously of a million characters of 'a'

```c
 #include "rhash.h" /* LibRHash interface */
 
 int main(int argc, char *argv[]) {
   rhash context;
   char digest[64];
   char output[130];
   int i;
 
   rhash_library_init(); /* initialize static data */
 
   context = rhash_init(RHASH_MD4 | RHASH_MD5);
   if(!context) {
     fprintf(stderr, "error: couldn't initialize rhash context\n");
     return 1;
   }
 
   for(i = 0; i < 1000000; i++) {
     rhash_update(context, "a", 1);
   }
   rhash_final(context, NULL); /* finalize hash calculation */
 
   /* output digest as a hexadecimal hash string */
   rhash_print(output, context, RHASH_MD4, RHPR_UPPERCASE); 
   printf("%s ('a'x1000000) = %s\n", rhash_get_name(RHASH_MD4), output);
 
   rhash_print(output, context, RHASH_MD5, RHPR_UPPERCASE); 
   printf("%s ('a'x1000000) = %s\n", rhash_get_name(RHASH_MD5), output);
 
   rhash_free(context);
   return 0;
 }
```

### Hi-level interface

* Calculating SHA1 hash of a string

```c
 #include <string.h>
 #include "rhash.h" /* LibRHash interface */
 
 int main(int argc, char *argv[])
 {
   const char* msg = "message digest";
   char digest[64];
   char output[130];
 
   rhash_library_init(); /* initialize static data */
 
   int res = rhash_msg(RHASH_SHA1, msg, strlen(msg), digest);
   if(res < 0) {
     fprintf(stderr, "hash calculation error\n");
     return 1;
   }
 
   /* convert binary digest to hexadecimal string */
   rhash_print_bytes(output, digest, rhash_get_digest_size(RHASH_SHA1),
       (RHPR_HEX | RHPR_UPPERCASE));
 
   printf("%s (\"%s\") = %s\n", rhash_get_name(RHASH_SHA1), msg, output);
   return 0;
 }
```

* Calculating TTH hash of a file

```c
 #include <errno.h>
 #include "rhash.h" /* LibRHash interface */
 
 int main(int argc, char *argv[])
 {
   const char* filepath = "test_file.txt";
   char digest[64];
   char output[130];
 
   rhash_library_init(); /* initialize static data */
 
   int res = rhash_file(RHASH_TTH, filepath, digest);
   if(res < 0) {
     fprintf(stderr, "LibRHash error: %s: %s\n", filepath, strerror(errno));
     return 1;
   }
 
   /* convert binary digest to hexadecimal string */
   rhash_print_bytes(output, digest, rhash_get_digest_size(RHASH_TTH),
       (RHPR_BASE32 | RHPR_UPPERCASE));
 
   printf("%s (%s) = %s\n", rhash_get_name(RHASH_TTH), filepath, output);
   return 0;
 }
```

[bindings]: ../bindings/
[RHash License]: ../COPYING
[Open Source]: http://en.wikipedia.org/wiki/Open_Source
