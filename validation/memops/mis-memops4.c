void foo(int a, int *p)
{
	*p;
	for (;; (*p)++)
		for (; a;)
			*p;
}

/*
 * check-name: mis-memops4
 * check-command: test-linearize -Wno-decl $file
 *
 * check-output-ignore
 * check-output-pattern(1): load\\.
 */
