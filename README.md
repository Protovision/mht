Mark's hashtable
================

* Separate chaining collision resolution strategy
* Non-empty buckets are linked for fast traversal of entries
* References to entries remain valid even after a table resize/rehash
* Uses only libc

Supported hashtable operations
------------------------------

* Insert key/value pair
* Lookup entry from key
* Delete entry
* Clear all entries
* Resize table
* Get first entry
* Get next entry
* Traverse all entries
