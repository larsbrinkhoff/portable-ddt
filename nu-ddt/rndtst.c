#


main ()
{
	short	i, j, c, val, seed;
	register short k;

	seed = 0;
	printf ("Seed: ");
	while ((c = getchar ()) != '\r')
		seed = (8 * seed) + (c - '0');
	while (1) {
		printf ("    seed     val    seed     val    seed     val    seed     val\n\n");
		for (i = 0; i < 10; i++) {
			for (j = 0; j < 4; j++) {
				random(&seed, &val);
				printf ("  %6o  %6o", (long)seed & 0xFFFF, (long)val & 0xFFFF);
				for (k = 0; k < 32767; k++);
			}
			printf ("\n");
		}
		getchar (c);
	}
}
