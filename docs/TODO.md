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
S/N|Task                                         |Description                                                                            |Status |
---|---------------------------------------------|---------------------------------------------------------------------------------------|-------|
1  |Write new file content to disk (create_file).|The metadata accounting is done but the actual file is not written to disk.            |Done   |
2  |Make query_free_sectors more efficient.      |This currently takes like 1.5 seconds which is egregiously unacceptable.               |Pending|
3  |Fill in error handling paths in create_file. |Effort put into error-handling in this project on the whole adds value.                |Done   |
4  |Have object_alloc support allocations > 2k.  |Currently, allocs > 2k have to zone_alloc'd, we should raise the max object_alloc size.|Pending|
5  |Write changes to fnode_table content to disk |                                                                                       |Done   |
6  |Simplify shell code - it's too complicated   |It's way too unecessarily and irritatingly complicated.                                |Pending|
7  |
