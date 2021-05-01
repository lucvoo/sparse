int foo(int a, int b)
{
	int r = a;

	if (a || 1)
		r = b;

	return r;
}

/*
 * check-name: pack-phi
 * check-command: test-linearize -Wno-decl $file
 *
 * check-output-ignore
 * check-output-excludes: phi
 */
