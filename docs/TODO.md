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
<b>Have object_alloc support allocations > 2k. (PENDING)</b>

Currently, allocs > 2k have to be zone_alloc'd.
</li>
<li>
<b> Implement delete_folder</b>
</li>
<li>
<b> Make create_file and create_folder work with paths. </b>
</li>
<li>
<b> Implement indirect fnode blocks (single & double indirection) </b>
</ol>

