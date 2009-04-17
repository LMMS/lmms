#define fl_file_chooser fixed_file_chooser
#include <string>

extern std::string __presets_dir;

static inline char * fixed_file_chooser(const char * message, const char *pattern, const char *fname, int relative = 0)
{
	Fl_File_Chooser* fc = new Fl_File_Chooser( __presets_dir.c_str(),
							pattern,
							Fl_File_Chooser::SINGLE,
								message );
	fc->show();
	while( fc->shown() )
	{
		Fl::wait();
	}

	char * r = NULL;
	if( fc->count() > 0 )
	{
		r = strdup( fc->value() );
	}
	delete fc;

	return r;
}

