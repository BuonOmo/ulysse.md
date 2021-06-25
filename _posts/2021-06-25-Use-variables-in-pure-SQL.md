---
layout: post
title: Use variables in pure SQL
author: Ulysse
---

{{ page.title }}
================

<p class="date">{{ page.date | date: "%B %e, %Y" }} — {{ page.author }}</p>

Here's the hack:

```sql
WITH vars as (
  SELECT 'my-very-long-long-name' as foo,
         112098237449803127134987 as num,
         '{"hello":"there"}'::json as js
)
SELECT vars.foo, vars.js->'hello', num / 2
FROM vars;
```
