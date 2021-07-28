---
layout: post
title: Disable Google's FLoC on Sinatra
author: Ulysse
---

Google's new technology, FLoC, is quite intrusive [according to the EFF](https://amifloced.org/).
Hence I strongly suggest against, as already it is disabled by Wordpress (2/5 of the web), and
in [discussion](https://discuss.rubyonrails.org/t/proposal-add-default-header-to-disable-floc/78025) to be disabled by default on Rails.

To do disable FLoC for your users on Sinatra, it is quite easy:

```ruby
before do
  # Disable google's intrusive FLoC.
  # See:
  # - [TL;DR](https://andycroll.com/ruby/opt-out-of-google-floc-tracking-in-rails/)
  # - [No link to analytics](https://usefathom.com/blog/google-floc)
  # - [main reference](https://amifloced.org/)
  headers "Permissions-Policy" => "interest-cohort=()"
end
```
