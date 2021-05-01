void foo(int p)
{
	unsigned x[1];

	while (1) {
		if (p)
			x[0] += 1;
		else
			break;
	}
}

/*
 * check-name: bad-memops0
 * check-command: test-linearize -Wno-decl $file
 *
 * check-output-ignore
 * check-output-excludes: %r3 <- %r3
 * check-output-excludes: %r4 <- %r4
 */
