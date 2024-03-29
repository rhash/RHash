# Contribution guidelines for the RHash project

There are many ways of contributing to the project.
* Sponsor the project
* Translating to other languages
* Hunting bugs
* Packaging RHash to a new distribution
* Contributing by writing code

## Sponsor the project
<a href="https://opencollective.com/rhash"><img src="http://rhash.sourceforge.net/img/sponsor-button-36.png" width="136" height="36" border="0" valign="middle" alt="Sponsor RHash"/></a>
&nbsp;Support the project by money.

## Translating to other languages
For online translation you need to register at the [Launchpad] platform,
then visit [RHash translations] and translate untranslated strings.

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
Investigate one of the bugs listed at [issues] page, fix it and make a pull request.

## Packaging RHash to a new distribution
Check if your OS distribution has the latest RHash. If not, then make a package and publish it into the OS repository.

[donating]: http://sourceforge.net/donate/index.php?group_id=205103
[Launchpad]: https://launchpad.net/
[RHash translations]: https://translations.launchpad.net/rhash/
[Pull Request]: https://github.com/rhash/RHash/pulls
[repository]: https://github.com/rhash/RHash/
[issues]: https://github.com/rhash/RHash/issues
