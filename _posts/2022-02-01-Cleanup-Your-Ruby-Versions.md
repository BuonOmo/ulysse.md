---
layout: post
title: Cleanup Your Ruby Versions
author: Ulysse
meta:
  description: A simple way to make some space on your computer
---

One day my computer decided to tell that I had no space available anymore. As
a programmer, my first reaction was `du -sh *` and that is when I saw that my
deprecated ruby versions were still here, and they some place!

```sh
$ cd ~/.rbenv/versions/
$ du -sh *
108M	2.4.4
1.4G	2.5.3
910M	2.6.5
588M	2.6.6
823M	2.7.1
1.1G	2.7.2
2.0G	2.7.4
 87M	3.0.0
```

However, `rm -rf ~/.rbenv/versions` might have been an overreaction, let's first
check for usage of various ruby versions:

```sh
$ cd ~/Dev
$ fd --no-ignore --hidden --glob .ruby-version | ruby -e '
	pp $<.
		map(&:chomp).
		group_by { IO.read(_1).strip }.
		sort
'
[["2.3.1", ["presidentbeef/brakeman/test/apps/rails5.2/.ruby-version"]],
 ["2.7.2", ["battleship/.ruby-version"]],
 ["2.7.4",
  ["klaxit/klaxit-devops/.ruby-version",
   "klaxit/klaxit-api/.ruby-version",
   "klaxit/klaxit-via/.ruby-version"]],
 ["3.0.0",
  ["ruby/ruby/.ruby-version",
   "klaxit/fast-polylines/.ruby-version"]],
 ["3.0.3", ["rubygems/rubygems.org/.ruby-version"]]]
```

Ok well, there are some unused versions there!

```sh
$ rbenv uninstall --force 2.4.4
$ rbenv uninstall --force 2.5.3
$ rbenv uninstall --force 2.6.5
$ rbenv uninstall --force 2.6.6
$ rbenv uninstall --force 2.7.1
$ rbenv uninstall --force 2.7.2
```

The example I gave may seem pointless, however just check `du -sh ~/.rbenv` on
your computer, you may gain up to 10G.
