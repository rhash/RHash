# Contribution guidelines for the RHash project

There are many ways of contributing to the project.
* Contributing by writing code
* Hunting bugs
* Translating to other languages
* Packaging RHash to a new distribution
* Contributing money

## Contributing by writing code
* Fixing bugs and implementing features, check current [issues].
* Updating documentation

## Translating to other languages
For online translation you need to register at the [Launchpad] platform.
Then visit [RHash translations] and translate untranslated strings.

Alternatively, you can translate one of [po files](../po/) and send a patch.

## Hunting bugs
* Test RHash with miscellaneous options. Try different OS and different environments.
* Test compilation by different compilers.
* Try static/dynamic analysis or phasing.

If you have a found bug, try to reproduce it with the latest version, compiled from
the [repository]. Collect information about you environment, particularly use command:
```sh
make -C librhash print-info
```
File new bugs at the [issues] page.

## Packaging RHash to a new distribution
Check if your OS distribution has the latest RHash. If not, then make a package and publish it into the OS repository.

## Contributing money
<a href="http://sourceforge.net/donate/index.php?group_id=205103"><img align="left" src="http://images.sourceforge.net/images/project-support.jpg" width="88" height="32" border="0" alt="Support RHash" title="Please donate to support RHash development!"/></a>
If you like the project, please consider [donating] a few dollars.

[issues]: https://github.com/rhash/RHash/issues
[Launchpad]: https://launchpad.net/
[RHash translations]: https://translations.launchpad.net/rhash/
[repository]: https://github.com/rhash/RHash/
[donating]: http://sourceforge.net/donate/index.php?group_id=205103
