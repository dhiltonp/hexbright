/* Check that either .data is empty, or it gets properly copied */
ASSERT(SIZEOF(.data) == 0 || DEFINED(__do_copy_data), "Data section not empty. Please compile with -DHAVE_DATA")

/* Check that either .bss is empty, or gets properly cleared */
ASSERT(SIZEOF(.bss) == 0 || DEFINED(__do_clear_bss), "BSS section not empty. Please compile with -DHAVE_BSS")
