# reimagine
This tool can be used for decrypting IMG3 files, listing and dumping their tags, stripping the IMG3 container, and compressing/decompressing lzss kernelcaches.

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
	-x, --decompress	decompress lzss compressed kernelcache
	-c, --compress		lzss compress an uncompressed kernelcache
```

## Example usage
* Decrypt a kernelcache and extract it into a raw MachO file
`reimagine kernelcache.release.n90 kernel.macho -iv ... -k ... -x -r`
* Re-compress a raw kernelcache into a lzss `complzss` container
`reimagine kernel.macho kernelcache.lzss -c -r`

## Building
`make`
