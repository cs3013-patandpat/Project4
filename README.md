Authors: Patrick Lebold & Patrick Polley

To compile:

	$ make all
	
To run:

	$ ./virtualmemory
	
	or (to run in DEBUG mode)
	
	$ ./virtualmemory 1

	or (to run in DEBUG mode with a spesified eviction algorithm)
	
	$ ./virtualmemory 1 [0 OR 1 OR 2]

	or (to run in non-DEBUG mode with a spesified eviction algorithm)
	
	$ ./virtualmemory 0 [0 OR 1 OR 2]
	
---------

Code currently pages correctly in RAM and the SSD. Once it evicts one cell from SSD -> HD, it stops paging correctly.
We are currently investigating this issue.

We recommend running in DEBUG mode to see a more-full version of what is going on behind the scenes.

Eviction mode 0 evicts the first page it finds that is eligible.
Eviction mode 1 evicts a random page from the eligible pages it finds.
Eviction mode 2 is a second chance eviction scheme, based on eviction mode 1, where it evicts the first page it finds UNLESS that page has been used recently.
