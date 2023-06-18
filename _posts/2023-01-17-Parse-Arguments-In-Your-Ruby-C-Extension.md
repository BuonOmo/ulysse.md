---
layout: post
title: Parse Arguments In Your Ruby C Extension
author: Ulysse
canonical: https://blog.appsignal.com/2023/01/18/how-to-parse-arguments-in-your-ruby-c-extension.html
meta:
  description: >
    Let's explore two ways to set up a complex Ruby API written in C.
  image: https://blog.appsignal.com/_next/image?url=%2Fimages%2Fblog%2F2023-01%2Fparse-rubyc.png&w=2048&q=50
---

Ruby is a wonderful language, made for humans first and machines
second. It is easy to read and write. There are plenty of ways to write anything,
and you can often guess its standard library by typing the name of the
method you would have chosen yourself.

Because of this, Ruby's arguments are very flexible, which lets us
express our APIs very clearly. But this comes with a drawback:  Ruby is quite hard to parse for C
extension developers!

In this article, we'll go through two ways to set up a complex Ruby API
that is written in C:

- with `rb_define_method` and parsing it with `rb_scan_args`
- using a Ruby interface

Let's get started!

# C and Ruby: An Introduction
As mentioned, Ruby is hard to parse for C extension developers.

For example:

```ruby
def this(is, a = "quite", *convoluted, yet: 1, possible:, &example)
  # And we could have omitted the block, yet still passed it as an argument!
end
```

The beauty of C, the language Ruby is written in, stems from its simplicity, including in its function parameters:

- `<data-type> <variable-identifier>`
- `...` for variadic arguments.

These will help you maintain a
codebase that is not too hard to understand.

Here's the most complex way to define a C function:

```c
int printf(const char*, ...);
```

When you code a C extension for your Ruby codebase, you'll start
to understand where the complexity begins. But don't worry - Ruby MRI developers have us covered.

# Simple Method Definition in a Ruby C Extension

We'll start with a method you'll have to use at some point, `rb_define_method`.

You can also [follow along with the code examples in this repo](https://github.com/BuonOmo/c-arg-parsing-comparison).

Here is `rb_define_method`'s signature:

```c
void rb_define_method(VALUE klass, const char *name,
                      VALUE (*func)(ANYARGS), int argc);
```

And according to [Ruby's extension.rdoc](https://ruby-doc.org/core-3.1.2/ruby-3_1_2/doc/extension_rdoc.html):

> argc is the number of arguments.  if argc is -1, the function will receive
> 3 arguments: argc, argv, and self.  if argc is -2, the function will
> receive 2 arguments, self and args, where args is a Ruby array of
> the method arguments.

In a nutshell:

```c
// argc is -1.
VALUE func(int argc, VALUE* argv, VALUE self);
// argc is -2.
VALUE func(VALUE self, VALUE args);
// argc is N (here N=2).
VALUE func(VALUE self, VALUE arg1, VALUE arg2);
```

So if your API only consists of methods with fixed parameter lengths, or only
one variadic parameter (`def foo(*bar)`), read no further - you are done! If
you want a richer way to call your API, please, be my guest.

_By the way, if you want more involved examples on `rb_define_method`, I
suggest you read [Peter Zhu's article on Defining Methods](https://blog.peterzhu.ca/ruby-c-ext-part-2/)._

# Using Ruby C API Internals
So, let's go back to our use case: parsing complex arguments. Fortunately, some tools can help us.

But first,
let's see the limitations of solely using `rb_define_method`.

# Drawbacks of `rb_define_method`

## No Mention of Block Arguments
One limitation is that `rb_define_method` never mentions block arguments. Those
are not considered, as it doesn't really matter to Ruby anyway if you pass
a block. You can still ensure that a block is passed by using
`rb_block_given_p` or `rb_need_block`. There's more on that topic in
[Peter Zhu's article](https://blog.peterzhu.ca/ruby-c-ext-part-2/#blocks).

## Args Can Vary
Another important limitation is that args can vary, but the method
call itself is not so constrained. Therefore, if you want your API to be
like `def foo(bar, *baz)`, you'll have to parse your arguments. There are
a few methods to help you down that path. `rb_check_arity` is one you
can use, along with the -1 version of `rb_define_method`.

Here's the function signature:

```c
rb_check_arity(int argc, int min, int max)
```

## Keyword Arguments
One last limitation we'll go through is the use of keyword arguments.
And I kept the best for last, as that will be the core of this article.

We have to retrieve
keyword arguments before we can parse them correctly. Fortunately, the Ruby C API comes with a method for this,
`rb_scan_args`.

Here's the `rb_scan_args` signature:

```c
rb_scan_args(int argc, VALUE *argv, const char *fmt, ...)
```

You pass `rb_scan_args` the `argc` and `argv`
given by `rb_define_method` - a string that says how the arguments should be
parsed (`fmt`) and the receiver for those arguments. There you go, all of
Ruby's args complexity parsed in one line! Well, almost.

You can refer to [extension.rdoc](https://ruby-doc.org/core-3.1.2/ruby-3_1_2/doc/extension_rdoc.html) for a formal representation of
how `fmt` should be written, although we'll partially cover it in
the examples below.

# Write and Parse a Function: An Example
For the rest of this article, let's consider that we want to write this
function:

```ruby
def voronoi_diagram(envelope, *polygons, tolerance:, only_edges: false)
end
```

Parsing this using `rb_scan_args` will look like this:

```c
VALUE voronoi_diagram(int argc, VALUE *argv, VALUE self) {
  VALUE envelope;
  VALUE polygons;
  VALUE kwargs;
  rb_scan_args(argc, argv, "1*:", &envelope, &polygons, &kwargs);

  // Actual method
}
```

The `"1*:"` gibberish means:

- `1`: one required positional argument
- `*`: any amount of positional arguments not required
- `:`: keyword arguments at the end

# Parse Keyword Arguments
Now we've constrained the method, unfortunately, we are not done yet. Our
current API is `def voronoi_diagram(envelope, *polygons, **kwargs)`.

Finally, we
need to parse those keyword arguments using [`rb_get_kwargs`](https://ruby-doc.org/core-3.1.2/ruby-3_1_2/doc/extension_rdoc.html#label-Method+Definition).

```c
int rb_get_kwargs(VALUE keyword_hash, const ID *table,
                  int required, int optional, VALUE *values);
```

You have
to choose some required and optional arguments. Once that's done, you use `table` to tell Ruby the name of
those arguments, and you store the result in an array (`values`).

```c
VALUE voronoi_diagram(int argc, VALUE *argv, VALUE self) {
  VALUE envelope;
  VALUE polygons;
  VALUE tolerance;
  VALUE only_edges;

  VALUE kwargs;
  rb_scan_args(argc, argv, "1*:", &envelope, &polygons, &kwargs);

  ID table[2];
	table[0] = rb_intern("tolerance");
	table[1] = rb_intern("only_edges");
  VALUE *values;
  rb_get_kwargs(kwargs, table, 1, 1, values);

  tolerance = values[0];
  only_edges = values[1] == Qundef ? Qfalse : values[1];

  // Actual method
}
```

There you have it! A complex Ruby method, parsed using only C. However,
if this is too convoluted for you, there is another option.

# Using a Ruby Interface

Another way to handle the problem is actually to use Ruby's syntax directly
and do the parsing at the Ruby stage.

```ruby
def voronoi_diagram(envelope, *polygons, tolerance:, only_edges: false)
  c_voronoi_diagram(envelope, polygons, tolerance, only_edges)
end
```

With this, you can directly use the third form of `rb_define_method`, for a C method that looks like this:

```c
VALUE c_voronoi_diagram(VALUE self, VALUE envelope,
                        VALUE polygons, VALUE tolerance,
                        VALUE only_edges) {
  // Actual method
}
```

And there you go - you completely avoid the problem with an elegant
solution that is actually used for some methods in a Ruby
implementation (with the `Primitive` class).

Although the class used by the MRI is quite complex and generates C itself, we can get inspired by it.

Let's create an object and plug our methods
to avoid having a visible `c_voronoi_diagram` for users of our API:

```c
void Init_ext() {
  VALUE primary_mod = rb_define_module("Hidden")
  rb_define_method(primary_mod, "voronoi_diagram", func, 4)
}
```

```ruby
def voronoi_diagram(envelope, *polygons, tolerance:, only_edges: false)
  Hidden.voronoi_diagram(envelope, polygons, tolerance, only_edges)
end
```

[Check out a real use case of this class in RGeo's codebase](https://github.com/rgeo/rgeo).

# Parsing Arguments: Which Method Should I Use?
[My repo](https://github.com/BuonOmo/c-arg-parsing-comparison) showcases the two ways to set up a complex Ruby API
that is written in C - to recap:

- with `rb_define_method` and parsing it with `rb_scan_args`
- using a Ruby interface

When we compare
both solutions in terms of performance, they are roughly equal (Ruby parsing is
1.02 times faster on average on my M1). That doesn't make much of a difference.

In the [RGeo](https://github.com/rgeo/rgeo) lib, our first design choice was to have an API that
only uses variadic length arguments. No keywords, no blocks. This can be
very limiting, and we are now using the `Primitive` way to allow more
convoluted arguments.

## Benefits of Using a Ruby Interface
My advice is to use a Ruby interface for multiple
reasons, including that:

- the code size is smaller
- changes are easier to make

Overall,
this makes the experience of reading your codebase simpler.

Engaging Ruby users in reading the source code of the gems they use is
important to me. As *Ruby is easy to read*, gems should be as well.

## Benefits of Using `rb_define_method`
The C version, however, gives us a taste of some very useful internal methods.
For instance, `rb_check_arity` is still really useful. Methods for handling
blocks are great as well, and you might not need to use the Ruby facade for
blocks.

It's all about finding the best choice for your use case.

# Wrapping Up
In this post, we explored two methods to parse arguments in your Ruby C extension - using `rb_define_method` (also looking briefly at its limitations) and parsing it with `rb_scan_args` and using a Ruby interface.

If you want to read more about C extensions, I recommend
[A Rubyist's Walk Along the C-side](https://blog.peterzhu.ca/ruby-c-ext/) or my article [Working with ruby C extensions on Mac](https://medium.com/klaxit-techblog/working-with-ruby-c-extensions-on-a-macbook-639c068a9815).

Happy coding!
