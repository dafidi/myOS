# myOS TODO

I think keeping a TODO list will allow me to have concrete, measurable goals to accomplish.

## Target Features

### Majors

S/N|Feature Name     |Description                                 |Status      |Notes                                                    |
---|-----------------|--------------------------------------------|------------|---------------------------------------------------------|
1  |Memory Management|Paging, Virtual Memory and what not.        |Done        |We should have object_alloc support allocations > 2k.    |
2  |File System      |File System as known to mankind.            |In progress |Basic filesystem supporting create, update, view, delete.|
3  |Processes        |Execute/dispatch simple x86 32-bit binaries.|In progress |E.g. a simple hello_word.asm.                            |

### Minors
<ol>
<li>
<b>Write new file content to disk (create_file). (DONE)</b>

The metadata accounting is done but the actual file is not written to disk.
</li>
<li>
<b>Make query_free_sectors more efficient. (DONE)</b>

This currently takes like 1.5 seconds which is egregiously bad.
</li>
<li>
<b>Fill in error handling paths in create_file. (DONE)</b>

Effort put into error-handling in this project on.
</li>
<li>
<b>Have object_alloc support allocations > 2k. (PENDING)</b>

Currently, allocs > 2k have to be zone_alloc'd.</li>
<li>
<b>Write changes to fnode_table content to disk. (DONE)</b>

</li>
<li>
<b>Simplify shell code - it's too complicated. (DONE)</b>

It's way too unecessarily and irritating and wrong.
</li>
<li>
<b>Support > 8k disk reads. (DONE)</b>

Gracefully handle DRQ data block max of 16 for read requests greater than 8k.
</li>
<li>
<b> Update dir_entry for curr_dir on create_file. (PENDING)</b>

A new file in a directory increases the directory's size, the dir_entry pointing to this directory in the parent directory should be update accordingly
</li>
</ol>
