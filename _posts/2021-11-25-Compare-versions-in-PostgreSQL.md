---
layout: post
title: Compare versions in PostgreSQL
author: Ulysse
meta:
  description: |
    TL;DR: Implementing a semver_match(version, '~> 2.3.1') function in SQL.
---

Yesterday, one of my colleagues asked me for a version comparison trick in SQL.
I gave him a very basic one, and looked a bit the internet for a simple pure
SQL solution to do easily a bit more than that. Nothing convinced me really,
hence this blog post and implementation.

When using semantic versioning (`MAJOR.MINOR.PATCH`), there are some nice tools
to compare a given version with a requirement, for instance, ruby's `Gemfile`
syntax goes like:

```ruby
# Minor and patch version can only be greater or
# equal to MAJOR.3.0.
# Major must be 2.
gem "fast-polylines", "~> 2.3"
# Only the patch version can be greater
# than the expected one.
gem "hiredis", "~> 0.6.3"
# Only patch version may vary.
gem "oj", "= 3.13"
# Any version greater or equal to 3.
gem "rgeo", ">= 3"
```

This can also be used in code directly:

```ruby
Gem::Version("10.0.1") > Gem::Version("10.0.0") # true
```

See [the ruby documentation](https://ruby-doc.org/stdlib-2.4.0/libdoc/rubygems/rdoc/Gem/Version.html) for a more in-depth explanation of those operators.

But what if you had a table with a _version_ (text) column in your SQL? Well,
fortunately in PostgreSQL, there is a nice `string_to_array` function which lets
us split our version easily:

```sql
SELECT string_to_array('4.2.0', '.'); -- {4,2,0}
```

And we can easily convert our resulting array to integers to simplify later
computation:

```sql
SELECT ARRAY['4', '2']::int[];
```

Since in PostgreSQL array comparison is done by comparing each pair consecutively, we are already very close to a complete solution:

```sql
SELECT ARRAY[2, 1] > ARRAY[2, 0]; -- ture
SELECT ARRAY[2, 1] > ARRAY[2]; -- true
```

combining what we have already, it is quite easy to do basic comparison between
two versions now:

```sql
SELECT string_to_array('6.6.6', '.')::int[] > string_to_array('6.5', '.')::int[]
```

However, what if we wanted to compare using ruby like `~>` comparison? We'll have
to dig a tiny bit deeper.

So `~> X.Y.Z` basically means that the expected version should be:
- `>= X.Y.Z`
- `< X.Y.(Z+1)`

And this apply to a comparison without patch as well, `~> X.Y` implies:
- `>= X.Y`
- `< X.(Y+1)`

Hence we could just write:

```sql
SELECT string_to_array('6.6.6', '.')::int[] >= string_to_array('6.6.6', '.')::int[]
AND string_to_array('6.6.6', '.')::int[] < string_to_array('6.6.7', '.')::int[]
```

However, I'd like to be able to express this comparison in a single line:

```sql
SELECT semver_match(ver, req)
FROM (VALUES ('6.6.6', '~> 6.6.6')) AS _ (ver, req);
```

So how can we can we compar our `ver` with both `>= 6.6.6` and `< 6.6.7`? Feel
free to pause there and find your own solution!

The first comparison is trivial, we just have to parse our `6.6.6` text. The
second one is a bit harder since we have to somehow increment our array. There is no postgres operator that lets us mutate an array in place, except when
setting a value. Hence I went for a more functional way: consider the n - 1
first elements of my array, and using [`array_append`](https://www.postgresql.org/docs/14/functions-array.html)
to concatenate the last value, incremented:

```sql
SELECT array_append(
	(ARRAY[4,2,1])[1:(array_length(ARRAY[4,2,1], 1) - 1)], -- X.Y
	(ARRAY[4,2,1])[array_length(ARRAY[4,2,1], 1)] + 1 -- Z + 1
)
```

Once we have that, we can use a `CASE` statement to see which operator was
used, and the rest is just a bit of syntax:

```sql
-- https://ulysse.md
-- example usage: semver_match('4.2.1', '~> 4.2.0').
CREATE OR REPLACE FUNCTION semver_match(version text, req text) RETURNS boolean
    LANGUAGE SQL
    IMMUTABLE
    RETURNS NULL ON NULL INPUT
    AS $$
    SELECT CASE
    WHEN req LIKE '~>%' THEN
		string_to_array(version, '.')::int[] >= string_to_array(substring(req from 4), '.')::int[]
		AND
		string_to_array(version, '.')::int[] <
		-- increment last item by one. (X.Y.Z => X.Y.(Z+1))
		array_append(
			(string_to_array(substring(req from 4), '.')::int[])[1:(array_length(string_to_array(req, '.'), 1) - 1)], -- X.Y
			(string_to_array(substring(req from 4), '.')::int[])[array_length(string_to_array(req, '.'), 1)] + 1 -- Z + 1
		)
    WHEN req LIKE '>%' THEN string_to_array(version, '.')::int[] > string_to_array(substring(req from 3), '.')::int[]
    WHEN req LIKE '<%' THEN string_to_array(version, '.')::int[] < string_to_array(substring(req from 3), '.')::int[]
    WHEN req LIKE '>=%' THEN string_to_array(version, '.')::int[] >= string_to_array(substring(req from 4), '.')::int[]
    WHEN req LIKE '<=%' THEN string_to_array(version, '.')::int[] <= string_to_array(substring(req from 4), '.')::int[]
    WHEN req LIKE '=%' THEN
		(string_to_array(version, '.')::int[])[1:array_length(string_to_array(substring(req from 3), '.'), 1)] =
		string_to_array(substring(req from 3), '.')::int[]
    ELSE NULL
    END $$;
```

Now we only have left to test our function:

```sql
-- tests.
SELECT
	ver,
	req,
	CASE WHEN semver_match(ver, req) = expected
	THEN '✅' ELSE '❌' END AS test_passed
FROM (VALUES
	('2.3.1', '> 2.3', TRUE),
	('2.3.1', '< 2.3.2', TRUE),
	('2.3.1', '~> 2.3.2', FALSE),
	('2.4.3', '~> 2.3.2', FALSE),
	('2.3.2', '~> 2.3.2', TRUE),
	('2.3.2', '= 2.3.2', TRUE),
	('2.3.2', '= 2.3', TRUE),
	('2.3.2', '= 2.4', FALSE)
) AS _ (ver, req, expected);
```

The whole code is also available [on StackOverflow](https://stackoverflow.com/a/70107570/6320039), feel free to improve or ping
me in the comments for suggestions!
