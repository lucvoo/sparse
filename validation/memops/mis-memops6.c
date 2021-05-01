void fn1(void);
void fn2(int *a)
{
	int r = *a;
	if (r)
		if (r)
			*a = 0;
	if (*a)
		fn1();
}

/*
 * check-name: mis-memops6
 * check-command: test-linearize -Wno-decl $file
 * check-known-to-fail
 *
 * check-output-ignore
 * check-output-excludes: phi
 * check-output-excludes: call
 */
