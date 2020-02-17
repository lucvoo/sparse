struct s {
	int a, b;
};

int foo(int *x, int r)
{
	struct s s;
	if (*x)
		if (s.a)
			r = 0;
	return r;
}

/*
 * check-name: bad-memops2
 * check-command: test-linearize -Wno-decl $file
 * check-known-to-fail
 *
 * check-output-ignore
 * check-output-pattern(2): load\\.
 * check-output-contains: select\\.
 * check-output-contains: phi\\.
 */
