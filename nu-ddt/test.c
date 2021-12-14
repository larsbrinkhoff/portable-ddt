main ()
{
	register int	a, b, i;

	a = 5;
	b = 10;

	for (i = 1; i < 4; i++)
		b = a * b + 10;

	printf ("Result = %d\n\n", b);
	}
