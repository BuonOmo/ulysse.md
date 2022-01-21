---
layout: post
title: Unexpected long running command
author: Ulysse
meta:
  description: |
    TL;DR: time matters in your terminal, so just print it, you’ll gain
    productivity!
  image: https://raw.githubusercontent.com/BuonOmo/zsh-command-time/main/demo.gif
---

When we're coding, we are often forced to take a break due to some `npm install`, `make test` or
else, but that's fine, we can estimate the time those commands take since we are doing those a lot.

However, sometimes you stumble upon a new command, that will take a lot of time, and you won't
expect that. The issue here is that you don't know how long it will take to run that one again.
Hence you will be forced to either wait in front of your terminal, or come back way later to be
sure it is not running.

I've had that issue tons of times, for instance trying to copy files with the `aws` CLI, or working
with `docker-compose`. And I found some work-arounds to make sure I can better manage my time.

# Use notifications

The great [terminal-notifier](https://github.com/julienXX/terminal-notifier) is a good way to
address that issue: just add `; terminal-notifier -message done` to the end of a command
you expect will take a long time, and you'll be alerted as soon as the job is done.

If you like that solution, here's a way to type less characters for this technique:

```zsh
# Append NOTIF to your command to have a notification once it is done, clicking
# on the notification will focus a terminal.
alias -g NOTIF=';terminal-notifier -group endCommandNotif -activate com.apple.Terminal -ignoreDnD -sound default -message done'
```

This is unfortunately not enough, since you may not expect at all a command to last.

# Choose a timed ZSH theme

This is a very effective and simple solution, as there are great themes for that. I'll
suggest for instance the [crunch](https://github.com/ohmyzsh/ohmyzsh/wiki/Themes#crunch)
theme. I myself use a mix of crunch and [robyrussell](https://github.com/ohmyzsh/ohmyzsh/wiki/Themes#robbyrussell),
which is time and git focused. I called it crunchrobby, you can give it a try if
you want!

```zsh
CRUNCH_BRACKET_COLOR="%{$fg[white]%}"
CRUNCH_TIME_COLOR="%(?:%{$fg[yellow]%}:%{$fg[red]%})"
CRUNCH_TIME_="$CRUNCH_TIME_COLOR%T%{$reset_color%}"

PROMPT="$CRUNCH_TIME_ "
PROMPT+=' %{$fg[cyan]%}%c%{$reset_color%} $(git_prompt_info)'

ZSH_THEME_GIT_PROMPT_PREFIX="%{$fg_bold[blue]%}git:(%{$fg[red]%}"
ZSH_THEME_GIT_PROMPT_SUFFIX="%{$reset_color%} "
ZSH_THEME_GIT_PROMPT_DIRTY="%{$fg[blue]%}) %{$fg[yellow]%}✗"
ZSH_THEME_GIT_PROMPT_CLEAN="%{$fg[blue]%})"
```

There is still a tiny issue with that solution: it is only ok for command lasting
minutes. Yet sometimes you want to know a bit more precise how long it took. For
instance, when I'm trying to find an instable test, I'd like to know how long it
took to run it 100 times, so I instantly know if I can run it 1000 times without
waiting too long.

# Time commands

So, couldn't the ideal solution just be to always prepend your commands with `time`? I don't
think so, as it would clutter way too much the terminal, for information that is useless more
often than not...

That is why I created a tiny zsh plugin called [zsh-command-time](https://github.com/BuonOmo/zsh-command-time),
which only prints time if longer than 5 seconds. Hence you'll have timing for those long running commands, while
not being bothered most of the time.

# Wrap it up

I use all of those three solutions in my day to day work, and they help me manage my time spent on the
terminal, and finding time for those [tiny](https://xkcd.com/) [breaks](https://thecodinglove.com/) we all love!

# BONUS: Ruby IRB timings

If you don't have an irbrc file yet, now may be a good time to create yours! Here's
[mine](https://github.com/BuonOmo/dotfiles/blob/main/.irbrc) as an example.

To have a similar way of working as in zsh with `zsh-command-time`, you may add
this to your irbrc:

```ruby
if IRB.respond_to?(:set_measure_callback)
  IRB.set_measure_callback do |context, code, line_no, &block|
    time = Time.now
    result = block.()
    now = Time.now
    diff = (now - time)
    puts 'processing time: %fs' % diff if IRB.conf[:MEASURE] && diff > 0.5
    result
  end
end
```

And now you'll have timings as long as it is more than 0.5 seconds.
