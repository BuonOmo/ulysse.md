---
layout: post
---

```ruby
require "bundler/inline"

gemfile do
	source "https://rubygems.org"
	gem "rails"
	gem "sqlite3"
end

require "active_record"
ActiveRecord::Base.establish_connection(adapter: "sqlite3", database: ":memory:")
ActiveRecord::Schema.define do
	create_table :posts, force: true do |t|
		t.string :title
		t.text :body
	end
end

class Post < ActiveRecord::Base
	has_many :comments
end

prefix = "posts-#{rand(0x100...0x1000).to_s(16)}"
trap(:EXIT) { system("rm -f #{prefix}.gv #{prefix}.svg") }
IO.write("#{prefix}.gv", Post.where(title: "Plot SQL query with Rails").arel.to_dot)
system("dot -T svg", in: "#{prefix}.gv", out: "#{prefix}.svg")
system("qlmanage -p #{prefix}.svg", %i[out err] => File::NULL)

```
