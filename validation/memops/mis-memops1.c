int foo(int *p, int r, int *q)
{
	if (*p)
		if (*q)
			r = *p;
	return r;
}

/*
 * check-name: mis-memops1
 * check-command: test-linearize -Wno-decl $file
 * check-known-to-fail
 *
 * check-output-ignore
 * check-output-pattern(2): load\\.
 * check-output-contains: select\\.
 * check-output-contains: phi\\.
 */
