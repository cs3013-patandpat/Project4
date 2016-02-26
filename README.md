Authors: Patrick Lebold & Patrick Polley

To compile:

	$ make all
	
To run:

	$ ./virtualmemory
	
	or (to run in DEBUG mode)
	
	$ ./virtualmemory 1
	
---------

Code currently pages correctly in RAM and the SSD. Once it evicts one cell from SSD -> HD, it stops paging correctly.
We are currently investigating this issue.

We recommend running in DEBUG mode to see a more-full version of what is going on behind the scenes.
