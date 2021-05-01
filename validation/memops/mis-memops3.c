void foo(int *p)
{
	for (; *p;)
		for (; *p;) ;
}

/*
 * check-name: mis-memops3
 * check-command: test-linearize -Wno-decl $file
 *
 * check-output-ignore
 * check-output-pattern(1): load\\.
 */
