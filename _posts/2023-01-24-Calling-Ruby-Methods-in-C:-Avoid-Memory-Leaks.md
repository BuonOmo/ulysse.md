---
layout: post
title: 'Calling Ruby Methods in C: Avoid Memory Leaks'
author: Ulysse
canonical: https://blog.appsignal.com/2023/01/25/calling-ruby-methods-in-c-avoid-memory-leaks.html
meta:
  description: >
    Discover how you can avoid memory leaks in your C extension by using functions like `rb_protect`.
  image: https://blog.appsignal.com/images/blog/2023-01/ruby-methods-c.png
---

Memory leaks are a pain for gem users. They are hard to track and can lead to expensive infrastructure costs.

Memory leaks within a C extension are even worse. You'll see a lot of tools and
articles about finding leaks in Ruby. However, you don't have the same access to internals in C.

A naive usage of `rb_funcall` can cause memory leaks: it's much better to use `rb_protect` instead. So, if you are a C extension
writer, please read on for the sake of developers who will use your gem.

Let's get started!

# The Issue with `rb_funcall` and C

`rb_funcall` can be a great tool when you need to interact between Ruby and the C parts
of your library but only need to write a little C.

However, when you run `rb_funcall`, you are no longer in C where everything is
straightforward. You can be left in muddy waters if the called function:
1. Completely changes its definition
during runtime
2. Raises a call

Number 1 is the easiest one to catch. You'll likely end up with a segfault, and
if your test suite is complete enough, you should catch that before publishing.

However, the latter can cause memory leaks and make your codebase way harder
to read. Let's take a look at that now.

# Raise in Ruby Causing C Memory Leaks

Ruby's raising mechanism jumps between parts of the code from one scope to the
first parent that catches an error. This is implemented in the MRI using `longjmp`
and `setjmp`.

If you are interested in how this is built, read the
[Evaluator chapter in the Ruby Hacking Guide][rhg-raise]. In a nutshell, when
you use a `begin..ensure` block, you `setjmp()`, and when you raise within
this block, you `longjmp()` to the saved position.

So if a function is raised with `rb_funcall`, the C code called after it
never executes.

The example below illustrates a potential leak. If `json_parse` raises, it will
leak.

```c
VALUE rb_create_geometry_hash(VALUE self, VALUE wkt) {
	// Alloc
	GEOSWKTReader* reader = GEOSWKTReader_create();
	GEOSGeoJSONWriter* writer = GEOSGeoJSONWriter_create();

	// C processing
    GEOSGeometry* geom = GEOSWKTReader_read(reader, StringValuePtr(wkt));
	char* geojson = GEOSGeoJSONWriter_writeGeometry(writer, geom, -1);

	// Ruby processing
	VALUE rb_geojson = rb_str_new_cstr(geojson);
	VALUE result = rb_funcall(self, rb_intern("json_parse"), 1, rb_geojson);

	// Free
	GEOSWKTReader_destroy(reader);
	GEOSGeom_destroy(geom);
	GEOSGeoJSONWriter_destroy(writer);
	GEOSFree(geojson);

	return result;
}
```

Of course, the example above is a bit silly - you could invert the freeing
and Ruby processing parts. However, this is not always possible, and
longer function bodies can become more intertwined.

# Using `begin..ensure` in Ruby

If you're using Ruby, you could instead write the above example using
`begin..ensure`:

```ruby
def create_geometry_hash(wkt)
	reader = GEOSWKTReader.new
	writer = GEOSGeoJSONWriter.new

	begin
		json_parse(writer.write(reader.read(wkt)))
	ensure
		reader.close
		writer.close
	end
end
```

This API is also available in C with `rb_rescue` and `rb_ensure`:

<!-- TODO: test that -->
```c
static VALUE try_ruby_processing(VALUE args) {
	char* geojson = (char*)args;
	// Ruby processing
	VALUE rb_geojson = rb_str_new_cstr(geojson);
	VALUE result = rb_funcall(self, rb_intern("json_parse"), 1, rb_geojson);
}

struct to_free {
	GEOSWKTReader* reader;
	GEOSGeoJSONWriter* writer;
	GEOSGeometry* geom;
	char* geojson;
};

static VALUE ensure_free(VALUE args) {
	struct to_free data = (struct to_free)args
	GEOSWKTReader_destroy(data.reader);
	GEOSGeom_destroy(data.geom);
	GEOSGeoJSONWriter_destroy(data.writer);
	GEOSFree(data.geojson);

}

VALUE rb_create_geometry_hash(VALUE self, VALUE wkt) {
	// Alloc
	GEOSWKTReader* reader = GEOSWKTReader_create();
	GEOSGeoJSONWriter* writer = GEOSGeoJSONWriter_create();

	// C processing
    GEOSGeometry* geom = GEOSWKTReader_read(reader, StringValuePtr(wkt));
	char* geojson = GEOSGeoJSONWriter_writeGeometry(writer, geom, -1);

	return rb_ensure(
		try_ruby_processing, (VALUE)geojson
		ensure_free, (struct to_free){ reader, writer, geom, geojson }
	);

	return result;
}
```

However, this is a bit cumbersome, and if you want to add a `rescue` block to
the party, it gets way less readable. I suggest reading [Peter Zhu's 'A Rubyist's Walk Along the C-side (Part 8): Exceptions & Error Handling'][zhu-errors]
if you want to use the `begin..rescue..ensure..end` API in C.

# Using `rb_protect` for C

There is another option. First, let's see how it could look in Ruby:

```ruby
def create_geometry_hash(wkt)
	reader = GEOSWKTReader.new
	writer = GEOSGeoJSONWriter.new

	err = nil
	result = nil
	begin
		result = json_parse(writer.write(reader.read(wkt)))
	rescue => e
		err = e
	end

	reader.close
	writer.close

	raise err if err

	result
end
```

This looks strange in Ruby, but is a workflow very well
suited to C. The MRI has an API for that, `rb_protect`, and the C function looks like this:

```c
VALUE ruby_call(VALUE rb_geojson) {
	return rb_funcall(self, rb_intern("json_parse"), 1, rb_geojson);
}

VALUE rb_create_geometry_hash(VALUE self, VALUE wkt) {
	int state;

	// Alloc
	GEOSWKTReader* reader = GEOSWKTReader_create();
	GEOSGeoJSONWriter* writer = GEOSGeoJSONWriter_create();

	// C processing
    GEOSGeometry* geom = GEOSWKTReader_read(reader, StringValuePtr(wkt));
	char* geojson = GEOSGeoJSONWriter_writeGeometry(writer, geom, -1);

	// Ruby processing
	VALUE rb_geojson = rb_str_new_cstr(geojson);
	rb_protect(ruby_call, rb_geojson, &state);

	// Free
	GEOSWKTReader_destroy(reader);
	GEOSGeom_destroy(geom);
	GEOSGeoJSONWriter_destroy(writer);
	GEOSFree(geojson);

	if (state) rb_jump_tag(state);

	return result;
}
```

The above method will re-raise a Ruby error after having freed
everything.

Note that we could also choose to ignore the error by using an
empty `rescue` block in Ruby:

```c
	...

	if (state) rb_set_errinfo(Qnil);

	return result; // => nil
}
```

:warning: If you do not raise the error, the `rb_set_errinfo(Qnil)` step is
important so you don't keep information available about an error that users should not know
about.

Or, you can conditionally choose to raise an error, like `rescue My::Error`:

```c
	...

	if (state) {
		if (rb_obj_is_kind_of(rb_errinfo(), rb_define_class_under(rb_mMy, "Error", rb_eStandardError))) {
			rb_jump_tag(state);
		} else {
			rb_set_errinfo(Qnil);
		}
	}

	return result;
}
```

You can actually consider `rb_errinfo()` as the same as the `$!` global
variable.

This is all great, but when it boils down to one `rb_funcall` only, we can simplify that API.

The overall idea behind using the `rb_protect` API when there is a function
to raise is to enhance readability. You don't need to check if the
function can raise or not, you assume it can, and use the state to work with
that.

# The `rb_protect_funcall` Proposal

Let's isolate `rb_funcall`, as it's the only *dangerous* method to use. Here's an API that will do that:

```c
VALUE rb_protect_funcall(VALUE recv, ID mid, int* state, int n, ...);
```

This API is the same as `rb_funcall`, with a `state` from `rb_protect`. Hence the
usage is pretty straightforward:

```c
VALUE rb_create_geometry_hash(VALUE self, VALUE wkt) {
	int state;

	// Alloc
	GEOSWKTReader* reader = GEOSWKTReader_create();
	GEOSGeoJSONWriter* writer = GEOSGeoJSONWriter_create();

	// C processing
    GEOSGeometry* geom = GEOSWKTReader_read(reader, StringValuePtr(wkt));
	char* geojson = GEOSGeoJSONWriter_writeGeometry(writer, geom, -1);

	// Ruby processing
	VALUE rb_geojson = rb_str_new_cstr(geojson);
	rb_protect_funcall(self, rb_intern("json_parse"), &state, 1,  rb_geojson);

	// Free
	GEOSWKTReader_destroy(reader);
	GEOSGeom_destroy(geom);
	GEOSGeoJSONWriter_destroy(writer);
	GEOSFree(geojson);

	if (state) rb_jump_tag(state);

	return result;
}
```

This API is not yet available in Ruby, and may never be. You can take it from
[RGeo][rgeo-funcall] (MIT LICENSE).

# A Real-World Example
If you want to see a real-world example, I encourage you to read the [RGeo](https://github.com/rgeo/rgeo)
codebase as we recently switched to going full `rb_protect`. We even
have some functions, such as `rgeo_convert_to_geos_geometry`, that propagate
this state for simpler usage. This function is a good place to start digging
around.

Feel free to [open an issue on RGeo](https://github.com/rgeo/rgeo) to
discuss the choices we made further.

# Wrapping Up
In this post, we warned against using `rb_funcall` with C as it can cause memory leaks. We explored using `begin..ensure` or `rb_protect` instead.

Happy coding!

[rhg-raise]: https://rubyhackingguide.ulysse.md/evaluator#raise
[zhu-errors]: https://blog.peterzhu.ca/ruby-c-ext-part-8/
[rgeo-funcall]: https://github.com/rgeo/rgeo/blob/893371229adcaf0f84c59e433db494304823990a/ext/geos_c_impl/ruby_more.c#L31-L56
[rgeo]: https://github.com/rgeo/rgeo#readme
[me]: https://ulysse.md/humans.txt
