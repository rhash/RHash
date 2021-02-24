# Contribution guidelines for the RHash project

There are many ways of contributing to the project.
* Donations
* Translating to other languages
* Hunting bugs
* Packaging RHash to a new distribution
* Contributing by writing code

## Donations
Click the image to the right to support the project by money.
<a href="http://sourceforge.net/donate/index.php?group_id=205103"><img src="http://images.sourceforge.net/images/project-support.jpg" width="88" height="32" border="0" valign="middle" alt="Support RHash" title="Please donate to support RHash development!"/></a>

## Translating to other languages
For online translation you need to register at the [Launchpad] platform.
Then visit [RHash translations] and translate untranslated strings.

Alternatively, you can translate one of the [po files](../po/) and create a [Pull Request] or send a patch.

## Hunting bugs
* Test RHash with miscellaneous options. Try different OS and different environments.
* Test compilation by different compilers.
* Try static/dynamic analysis or phasing.

If you have a found bug, try to reproduce it with the latest version, compiled from
the [repository]. Collect information about you environment, particularly use command:
```sh
make print-info
```
File new bugs at the [issues] page.

## Bug fixing
Investigate one of bugs listed at [issues] page, fix it and make a pull request.

## Packaging RHash to a new distribution
Check if your OS distribution has the latest RHash. If not, then make a package and publish it into the OS repository.

[donating]: http://sourceforge.net/donate/index.php?group_id=205103
[Launchpad]: https://launchpad.net/
[RHash translations]: https://translations.launchpad.net/rhash/
[Pull Request]: https://github.com/rhash/RHash/pulls
[repository]: https://github.com/rhash/RHash/
[issues]: https://github.com/rhash/RHash/issues
