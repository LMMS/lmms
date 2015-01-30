/* truncate:

   Truncates a float down to an int without worrying about
   the stack and crap like that.
*/

//static const float _truncate_half = 0.5f;

int truncate(float flt) {
	int i;

	i = flt;
/*
	asm (
		"flds 8(%ebp)\n"
		"\tfsubs _truncate_half\n"
		"\tfistpl -4(%ebp)\n"
	);
*/

	return i;
}
