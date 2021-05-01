static void foo(void)
{
	long c;

	goto l;

	c = (long)&c;

l:
	c = !c;
	goto l;
}

/*
 * check-name: internal infinite loop (5)
 * check-command: sparse $file
 * check-timeout:
 */
