# reimagine
This tool can be used for decrypting IMG files, listing, dumping their tags and patching their contents (using ibootsup from opensn0w-X).

**Licensed under GNU GPL.** 

```
Usage: reimagine <infile> [<outfile>] <options>
	-iv <IV>		set IV for decryption
	-k <key>		set key for decryption
	-ivk <ivkey>		set ivkey for decryption
	-d, --dump		print tag names and hexdump their content
				(Note: this option must be used on the decrypted file)
	-l, --list		list tags present in file
	-r, --raw		dump the DATA tag to <outfile>
	-p, --patch		patch the file using ibootsup
	-x, --decompress	decompress lzss compressed kernelcache
```

## Example usage
* Pwned iBEC for kloader
`reimagine iBEC.n90ap.RELEASE.dfu -k ... -iv ... -p -r`
* Pwned iBoot for iBEC
`reimagine iBoot.n90ap.RELEASE.img3 -k ... -iv ... -p`
* Decrypt kernel and extract into MachO file
`reimagine kernelcache.release.n90 kernel.macho -iv ... -k ... -x -r`

## Building
`make`
