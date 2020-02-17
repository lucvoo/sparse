void foo(int h)
{
	unsigned x[1];
loop:
	if (h) {
		x[0] = x[0] >> 1;
		goto loop;
	}
}

/*
 * check-name: bad-memops1
 * check-command: test-linearize -Wno-decl $file
 * check-known-to-fail
 *
 * check-output-ignore
 * check-output-excludes: %r3 <- %r3
 * check-output-excludes: %r4 <- %r4
 * check-output-contains: load\\.
 * check-output-contains: store\\.
 */
