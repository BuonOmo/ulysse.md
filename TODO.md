# Prio

- [x] Meta tags
- [x] Fix RSS

# Most Wanted

- [x] email address
- [x] 404 page
- [ ] mobile version

# Ideas

- [ ] Re-inject disqus per posts?
- [ ] integrate description to main page
- [ ] add description only posts

# Globe

- [x] web design
- [x] Database design
- [x] Database setup (see globe ruby project)
- [x] Generation of the website with github action
- [ ] application to write to the website + to store gps locations
	- [ ] get gps positions in background
	- [ ] send gps to the db when internet available
	- [ ] write a breve

## Optional

- [ ] add pictures

## Database design

```ruby
create_table "breves", force: :cascade do |t|
  t.integer "lat", null: false
  t.integer "lng", null: false
  t.date "date", null: false
  t.text "title"
  t.text "content"
  t.datetime "created_at", null: false
  t.datetime "updated_at", null: false
end
```

db uploaded to elephantsql
