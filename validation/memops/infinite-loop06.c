void foo(void)
{
	switch (0)
		for (;;)
			foo;
}

/*
 * check-name: infinite-loop06
 * check-command: sparse -Wno-decl $file
 * check-timeout:
 *
 * check-error-start
memops/infinite-loop06.c:3:9: warning: switch with no cases
 * check-error-end
 */
