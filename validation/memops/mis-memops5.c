void fn1(int b, int *p)
{
	*p = 1;
	for (; *p;) ;
}

/*
 * check-name: mis-memops5
 * check-command: test-linearize -Wno-decl $file
 *
 * check-output-ignore
 * check-output-excludes: load\\.
 */
