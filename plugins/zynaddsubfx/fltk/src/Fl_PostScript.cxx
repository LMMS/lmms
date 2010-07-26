//
// "$Id: Fl_PostScript.cxx 7653 2010-06-24 08:55:04Z manolo $"
//
// PostScript device support for the Fast Light Tool Kit (FLTK).
//
// Copyright 2010 by Bill Spitzak and others.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems to:
//
//     http://www.fltk.org/str.php
//

#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <FL/fl_draw.H>
#include <stdio.h>
#include <FL/Fl_PostScript.H>
#include <FL/Fl_Native_File_Chooser.H>

const char *Fl_PostScript_Graphics_Driver::device_type = "Fl_PostScript_Graphics_Driver";
const char *Fl_PostScript_File_Device::device_type = "Fl_PostScript_File_Device";
/** \brief Label of the PostScript file chooser window */
const char *Fl_PostScript_File_Device::file_chooser_title = "Select a .ps file";

/**
 @brief The constructor.
 */
Fl_PostScript_Graphics_Driver::Fl_PostScript_Graphics_Driver(void)
{
  close_cmd_ = 0;
  //lang_level_ = 3;
  lang_level_ = 2;
  mask = 0;
  ps_filename_ = NULL;
  type_ = device_type;
  scale_x = scale_y = 1.;
  bg_r = bg_g = bg_b = 255;
}

/** \brief The destructor. */
Fl_PostScript_Graphics_Driver::~Fl_PostScript_Graphics_Driver() {
  if(ps_filename_) free(ps_filename_);
}

/**
 @brief The constructor.
 */
Fl_PostScript_File_Device::Fl_PostScript_File_Device(void)
{
  type_ = device_type;
#ifdef __APPLE__
  gc = fl_gc; // the display context is used by fl_text_extents()
#endif
  Fl_Surface_Device::driver( new Fl_PostScript_Graphics_Driver() );
}

/**
 \brief Returns the PostScript driver of this drawing surface.
 */
Fl_PostScript_Graphics_Driver *Fl_PostScript_File_Device::driver()
{
  return (Fl_PostScript_Graphics_Driver*)Fl_Surface_Device::driver();
}


/**
 @brief Begins the session where all graphics requests will go to a local PostScript file.
 *
 Opens a file dialog entitled with Fl_PostScript_File_Device::file_chooser_title to select an output PostScript file.
 @param pagecount The total number of pages to be created.
 @param format Desired page format.
 @param layout Desired page layout.
 @return 0 iff OK, 1 if user cancelled the file dialog, 2 if fopen failed on user-selected output file.
 */
int Fl_PostScript_File_Device::start_job (int pagecount, enum Fl_PostScript_Graphics_Driver::Page_Format format, enum Fl_PostScript_Graphics_Driver::Page_Layout layout)
{
  Fl_Native_File_Chooser fnfc;
  fnfc.title(Fl_PostScript_File_Device::file_chooser_title);
  fnfc.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
  fnfc.options(Fl_Native_File_Chooser::SAVEAS_CONFIRM);
  fnfc.filter("PostScript\t*.ps\n");
  // Show native chooser
  if ( fnfc.show() ) return 1;
  Fl_PostScript_Graphics_Driver *ps = driver();
  ps->output = fopen(fnfc.filename(), "w");
  if(ps->output == NULL) return 2;
  ps->ps_filename_ = strdup(fnfc.filename());
  ps->start_postscript(pagecount, format, layout);
  this->set_current();
  return 0;
}

/**
 @brief Begins the session where all graphics requests will go to FILE pointer.
 *
 @param ps_output A writable FILE pointer that will receive PostScript output and that will be closed
 when end_job() will be called.
 @param pagecount The total number of pages to be created.
 @param format Desired page format.
 @param layout Desired page layout.
 @return always 0.
 */
int Fl_PostScript_File_Device::start_job (FILE *ps_output, int pagecount, enum Fl_PostScript_Graphics_Driver::Page_Format format, enum Fl_PostScript_Graphics_Driver::Page_Layout layout)
{
  Fl_PostScript_Graphics_Driver *ps = driver();
  ps->output = ps_output;
  ps->ps_filename_ = NULL;
  ps->start_postscript(pagecount, format, layout);
  this->set_current();
  return 0;
}

/**
 @brief The destructor.
 */
Fl_PostScript_File_Device::~Fl_PostScript_File_Device() {
  Fl_PostScript_Graphics_Driver *ps = driver();
  if (ps) delete ps;
}

#ifndef FL_DOXYGEN

#if ! (defined(__APPLE__) || defined(WIN32) )
  #include "print_panel.cxx"
#endif

const Fl_PostScript_Graphics_Driver::page_format Fl_PostScript_Graphics_Driver::page_formats[NO_PAGE_FORMATS] = { // order of enum Page_Format
// comes from appendix B of 5003.PPD_Spec_v4.3.pdf

  // A* // index(Ai) = i
  {2384, 3370, "A0"},
  {1684, 2384, "A1"},
  {1191, 1684, "A2"},
  { 842, 1191, "A3"},
  { 595,  842, "A4"},
  { 420,  595, "A5"},
  { 297,  420, "A6"},
  { 210,  297, "A7"},
  { 148,  210, "A8"},
  { 105,  148, "A9"},

  // B* // index(Bi) = i+10
  {2920, 4127, "B0"},
  {2064, 2920, "B1"},
  {1460, 2064, "B2"},
  {1032, 1460, "B3"},
  { 729, 1032, "B4"},
  { 516,  729, "B5"},
  { 363,  516, "B6"},
  { 258,  363, "B7"},
  { 181,  258, "B8"},
  { 127,  181, "B9"},
  {  91,  127, "B10"},

  // others
  { 459,  649, "EnvC5"}, // envelope
  { 312,  624, "EnvDL"}, // envelope
  { 522,  756, "Executive"},
  { 595,  935, "Folio"},
  {1224,  792, "Ledger"}, // landscape
  { 612, 1008, "Legal"},
  { 612,  792, "Letter"},
  { 792, 1224, "Tabloid"},
  { 297,  684, "Env10"} // envelope
};

//  Prolog string 

static const char * prolog =
"%%BeginProlog\n"
"/L { /y2 exch def\n"
"/x2 exch def\n"
"/y1 exch def\n"
"/x1 exch def\n"
"newpath   x1 y1 moveto x2 y2 lineto\n"
"stroke}\n"
"bind def\n"


"/R { /dy exch def\n"
"/dx exch def\n"
"/y exch def\n"
"/x exch def\n"
"newpath\n"
"x y moveto\n"
"dx 0 rlineto\n"
"0 dy rlineto\n"
"dx neg 0 rlineto\n"
"closepath stroke\n"
"} bind def\n"

"/CL {\n"
"/dy exch def\n"
"/dx exch def\n"
"/y exch def\n"
"/x exch def\n"
"newpath\n"
"x y moveto\n"
"dx 0 rlineto\n"
"0 dy rlineto\n"
"dx neg 0 rlineto\n"
"closepath\n"
"clip\n"
"} bind def\n"

"/FR { /dy exch def\n"
"/dx exch def\n"
"/y exch def\n"
"/x exch def\n"
"currentlinewidth 0 setlinewidth newpath\n"
"x y moveto\n"
"dx 0 rlineto\n"
"0 dy rlineto\n"
"dx neg 0 rlineto\n"
"closepath fill setlinewidth\n"
"} bind def\n"

"/GS { gsave } bind  def\n"
"/GR { grestore } bind def\n"

"/SP { showpage } bind def\n"
"/LW { setlinewidth } bind def\n"
"/CF /Courier def\n"
"/SF { /CF exch def } bind def\n"
"/fsize 12 def\n"
"/FS { /fsize exch def fsize CF findfont exch scalefont setfont }def \n"


"/GL { setgray } bind def\n"
"/SRGB { setrgbcolor } bind def\n"

//  color images 

"/CI { GS /py exch def /px exch def /sy exch def /sx exch def\n"
"translate \n"
"sx sy scale px py 8 \n"
"[ px 0 0 py neg 0 py ]\n"
"currentfile /ASCIIHexDecode filter\n false 3"
" colorimage GR\n"
"} bind def\n"

//  gray images 

"/GI { GS /py exch def /px exch def /sy exch def /sx exch def \n"
"translate \n"
"sx sy scale px py 8 \n"


"[ px 0 0 py neg 0 py ]\n"
"currentfile /ASCIIHexDecode filter\n"
"image GR\n"
"} bind def\n"

// single-color bitmask 

"/MI { GS /py exch def /px exch def /sy exch def /sx exch def \n"
"translate \n"
"sx sy scale px py false \n"
"[ px 0 0 py neg 0 py ]\n"
"currentfile /ASCIIHexDecode filter\n"
"imagemask GR\n"
"} bind def\n"


//  path 

"/BFP { newpath moveto }  def\n"
"/BP { newpath } bind def \n"
"/PL { lineto } bind def \n"
"/PM { moveto } bind def \n"
"/MT { moveto } bind def \n"
"/LT { lineto } bind def \n"
"/EFP { closepath fill } bind def\n"  //was:stroke
"/ELP { stroke } bind def\n"  
"/ECP { closepath stroke } bind def\n"  // Closed (loop)
"/LW { setlinewidth } bind def\n"

// ////////////////////////// misc ////////////////
"/TR { translate } bind def\n"
"/CT { concat } bind def\n"
"/RCT { matrix invertmatrix concat} bind def\n"
"/SC { scale } bind def\n"
//"/GPD { currentpagedevice /PageSize get} def\n"

// show at position with desired width
// usage:
// width (string) x y show_pos_width
"/show_pos_width {GS moveto dup dup stringwidth pop exch length exch 3 index exch sub exch "
"div 0 2 index 1 -1 scale ashow pop pop GR} bind def\n" // spacing altered to match desired width
//"/show_pos_width {GS moveto dup stringwidth pop 3 2 roll exch div -1 matrix scale concat "
//"show GR } bind def\n" // horizontally scaled text to match desired width

;


static const char * prolog_2 =  // prolog relevant only if lang_level >1

// color image dictionaries
"/CII {GS /inter exch def /py exch def /px exch def /sy exch def /sx exch def \n"
"translate \n"
"sx sy scale\n"
"/DeviceRGB setcolorspace\n"
"/IDD 8 dict def\n"
"IDD begin\n"
"/ImageType 1 def\n"
"/Width px def\n"
"/Height py def\n"
"/BitsPerComponent 8 def\n"
"/Interpolate inter def\n"
"/DataSource currentfile /ASCIIHexDecode filter def\n"
"/MultipleDataSources false def\n"
"/ImageMatrix [ px 0 0 py neg 0 py ] def\n"
"/Decode [ 0 1 0 1 0 1 ] def\n"
"end\n"
"IDD image GR} bind def\n"

// gray image dict 


"/GII {GS /inter exch def /py exch def /px exch def /sy exch def /sx exch def \n"
"translate \n"
"sx sy scale\n"
"/DeviceGray setcolorspace\n"
"/IDD 8 dict def\n"
"IDD begin\n"
"/ImageType 1 def\n"
"/Width px def\n"
"/Height py def\n"
"/BitsPerComponent 8 def\n"

"/Interpolate inter def\n"
"/DataSource currentfile /ASCIIHexDecode filter def\n"
"/MultipleDataSources false def\n"
"/ImageMatrix [ px 0 0 py neg 0 py ] def\n"
"/Decode [ 0 1 ] def\n"
"end\n"
"IDD image GR} bind def\n"

// procedure to modify a font to use ISOLatin1 encoding (iso-8859-1)
// and to keep its name unchanged
"/ToLatin1 { dup findfont dup length dict "
"begin {def} forall /Encoding ISOLatin1Encoding def currentdict end definefont pop } def\n"
// modify all fonts to use ISOLatin1 encoding
"/Helvetica ToLatin1 "
"/Helvetica-Bold ToLatin1 "
"/Helvetica-Oblique ToLatin1 "
"/Helvetica-BoldOblique ToLatin1 \n"
"/Courier ToLatin1 "
"/Courier-Bold ToLatin1 "
"/Courier-Oblique ToLatin1 "
"/Courier-BoldOblique ToLatin1 \n"
"/Times ToLatin1 "
"/Times-Bold ToLatin1 "
"/Times-Italic ToLatin1 "
"/Times-BoldItalic ToLatin1 \n"
;

static const char * prolog_2_pixmap =  // prolog relevant only if lang_level == 2 for pixmaps
"/pixmap_size { /pixmap_sy exch def /pixmap_sx exch def } bind def\n"

"/pixmap_mat {[ pixmap_sx 0 0 pixmap_sy neg 0 pixmap_sy ]}  bind def\n"

"/pixmap_dict {"
"<< /PatternType 1 "
"/PaintType 1 "
"/TilingType 2 "
"/BBox [0  0  pixmap_sx  pixmap_sy] "
"/XStep pixmap_sx "
"/YStep pixmap_sy\n"
"/PaintProc "
"{ begin "
"pixmap_sx pixmap_sy scale "
"pixmap_sx pixmap_sy 8 "
"pixmap_mat "
"pixmap_data "
"false 3 "
"colorimage "
"end "
"} bind "
">>\n"
"} bind def\n"

"/pixmap_plot {"
"GS "
"/pixmap_y exch def /pixmap_x exch def\n"
"pixmap_x pixmap_y translate\n"
"pixmap_dict matrix makepattern setpattern\n"
"pixmap_sx pixmap_sy scale\n"
"pixmap_sx pixmap_sy\n"
"true\n"
"pixmap_mat\n"
"pixmap_mask\n"
"imagemask\n"
"GR\n"
"} bind def\n"

"/pixmap_loaddata { /pixmap_data currentfile pixmap_sx pixmap_sy 3 mul mul string readhexstring "
"} bind def\n"

"/pixmap_loadmask { "
"/pixmap_mask currentfile  pixmap_sx 8 div ceiling cvi pixmap_sy mul  string readhexstring "
"} bind def\n"
;



static const char * prolog_3 = // prolog relevant only if lang_level >2

// masked color images 
"/CIM {GS /inter exch def /my exch def /mx exch def /py exch def /px exch def /sy exch def /sx exch def \n"
"translate \n"
"sx sy scale\n"
"/DeviceRGB setcolorspace\n"

"/IDD 8 dict def\n"

"IDD begin\n"
"/ImageType 1 def\n"
"/Width px def\n"
"/Height py def\n"
"/BitsPerComponent 8 def\n"
"/Interpolate inter def\n"
"/DataSource currentfile /ASCIIHexDecode filter def\n"
"/MultipleDataSources false def\n"
"/ImageMatrix [ px 0 0 py neg 0 py ] def\n"

"/Decode [ 0 1 0 1 0 1 ] def\n"
"end\n"

"/IMD 8 dict def\n"
"IMD begin\n"
"/ImageType 1 def\n"
"/Width mx def\n"           
"/Height my def\n"
"/BitsPerComponent 1 def\n"
//  "/Interpolate inter def\n"
"/ImageMatrix [ mx 0 0 my neg 0 my ] def\n"
"/Decode [ 1 0 ] def\n"
"end\n"

"<<\n"
"/ImageType 3\n"
"/InterleaveType 2\n"
"/MaskDict IMD\n"
"/DataDict IDD\n"
">> image GR\n"
"} bind def\n"


//  masked gray images 
"/GIM {GS /inter exch def /my exch def /mx exch def /py exch def /px exch def /sy exch def /sx exch def \n"
"translate \n"
"sx sy scale\n"
"/DeviceGray setcolorspace\n"

"/IDD 8 dict def\n"

"IDD begin\n"
"/ImageType 1 def\n"
"/Width px def\n"
"/Height py def\n"
"/BitsPerComponent 8 def\n"
"/Interpolate inter def\n"
"/DataSource currentfile /ASCIIHexDecode filter def\n"
"/MultipleDataSources false def\n"
"/ImageMatrix [ px 0 0 py neg 0 py ] def\n"

"/Decode [ 0 1 ] def\n"
"end\n"

"/IMD 8 dict def\n"

"IMD begin\n"
"/ImageType 1 def\n"
"/Width mx def\n"           
"/Height my def\n"
"/BitsPerComponent 1 def\n"
"/ImageMatrix [ mx 0 0 my neg 0 my ] def\n"
"/Decode [ 1 0 ] def\n"
"end\n"

"<<\n"
"/ImageType 3\n"
"/InterleaveType 2\n"
"/MaskDict IMD\n"
"/DataDict IDD\n"
">> image GR\n"
"} bind def\n"


"\n"
;

// end prolog 

int Fl_PostScript_Graphics_Driver::start_postscript (int pagecount, enum Fl_PostScript_Graphics_Driver::Page_Format format, enum Fl_PostScript_Graphics_Driver::Page_Layout layout)
//returns 0 iff OK
{
  int w, h, x;
  if (format == A4) {
    left_margin = 18;
    top_margin = 18;
  }
  else {
    left_margin = 12;
    top_margin = 12;
  }
  page_format_ = (enum Page_Format)(format | layout);
  
  fputs("%!PS-Adobe-3.0\n", output);
  fputs("%%Creator: FLTK\n", output);
  if (lang_level_>1)
    fprintf(output, "%%%%LanguageLevel: %i\n" , lang_level_);
  if ((pages_ = pagecount))
    fprintf(output, "%%%%Pages: %i\n", pagecount);
  else
    fputs("%%Pages: (atend)\n", output);
  fprintf(output, "%%%%BeginFeature: *PageSize %s\n", page_formats[format].name );
  w = page_formats[format].width;
  h = page_formats[format].height;
  if (lang_level_ == 3 && (layout & LANDSCAPE) ) { x = w; w = h; h = x; }
  fprintf(output, "<</PageSize[%d %d]>>setpagedevice\n", w, h );
  fputs("%%EndFeature\n", output);
  fputs("%%EndComments\n", output);
  fputs(prolog, output);
  if (lang_level_ > 1) {
    fputs(prolog_2, output);
    }
  if (lang_level_ == 2) {
    fputs(prolog_2_pixmap, output);
    }
  if (lang_level_ > 2)
    fputs(prolog_3, output);
  if (lang_level_ >= 3) {
    fputs("/CS { clipsave } bind def\n", output);
    fputs("/CR { cliprestore } bind def\n", output);
  } else {
    fputs("/CS { GS } bind def\n", output);
    fputs("/CR { GR } bind def\n", output);
  }
  page_policy_ = 1;
  
  
  fputs("%%EndProlog\n",output);
  if (lang_level_ >= 2)
    fprintf(output,"<< /Policies << /Pagesize 1 >> >> setpagedevice\n");
  
  reset();
  nPages=0;
  return 0;
}

void Fl_PostScript_Graphics_Driver::recover(){
  color(cr_,cg_,cb_);
  line_style(linestyle_,linewidth_,linedash_);
  font(font_,size_);
}

void Fl_PostScript_Graphics_Driver::reset(){
  gap_=1;
  clip_=0;
  cr_=cg_=cb_=0;
  font_=FL_HELVETICA;
  size_=12;
  linewidth_=0;
  linestyle_=FL_SOLID;
  strcpy(linedash_,"");
  Clip *c=clip_;   ////just not to have memory leaks for badly writen code (forgotten clip popping)
  
  while(c){
    clip_=clip_->prev;
    delete c;
    c=clip_;
  }
  
}

void Fl_PostScript_Graphics_Driver::page_policy(int p){
  page_policy_ = p;
  if(lang_level_>=2)
    fprintf(output,"<< /Policies << /Pagesize %i >> >> setpagedevice\n", p);
}

// //////////////////// paging //////////////////////////////////////////



void Fl_PostScript_Graphics_Driver::page(double pw, double ph, int media) {
  
  if (nPages){
    fprintf(output, "CR\nGR\nGR\nGR\nSP\nrestore\n");
  }
  ++nPages;
  fprintf(output, "%%%%Page: %i %i\n" , nPages , nPages);
  if (pw>ph){
    fprintf(output, "%%%%PageOrientation: Landscape\n");
  }else{
    fprintf(output, "%%%%PageOrientation: Portrait\n");
  }
  
  fprintf(output, "%%%%BeginPageSetup\n");
  if((media & MEDIA) &&(lang_level_>1)){
    int r = media & REVERSED;
    if(r) r = 2;
    fprintf(output, "<< /PageSize [%i %i] /Orientation %i>> setpagedevice\n", (int)(pw+.5), (int)(ph+.5), r);
  }
  fprintf(output, "%%%%EndPageSetup\n");
  
  pw_ = pw;
  ph_ = ph;
  reset();
  
  fprintf(output, "save\n");
  fprintf(output, "GS\n");
  fprintf(output, "%g %g TR\n", (double)0 /*lm_*/ , ph_ /* - tm_*/);
  fprintf(output, "1 -1 SC\n");
  line_style(0);
  fprintf(output, "GS\n");
  
  if (!((media & MEDIA) &&(lang_level_>1))){
    if (pw > ph) {
      if(media & REVERSED) {
        fprintf(output, "-90 rotate %i 0 translate\n", int(-pw));
	}
      else {
        fprintf(output, "90 rotate -%i -%i translate\n", (lang_level_ == 2 ? int(pw - ph) : 0), int(ph));
	}
      }
      else {
	if(media & REVERSED)
	  fprintf(output, "180 rotate %i %i translate\n", int(-pw), int(-ph));
	}
  }
  fprintf(output, "GS\nCS\n");
};

void Fl_PostScript_Graphics_Driver::page(int format){
  
  
  if(format &  LANDSCAPE){
    ph_=Fl_PostScript_Graphics_Driver::page_formats[format & 0xFF].width;
    pw_=Fl_PostScript_Graphics_Driver::page_formats[format & 0xFF].height;
  }else{
    pw_=Fl_PostScript_Graphics_Driver::page_formats[format & 0xFF].width;
    ph_=Fl_PostScript_Graphics_Driver::page_formats[format & 0xFF].height;
  }
  page(pw_,ph_,format & 0xFF00);//,orientation only;
};

void Fl_PostScript_Graphics_Driver::rect(int x, int y, int w, int h) {
  // Commented code does not work, i can't find the bug ;-(
  // fprintf(output, "GS\n");
  //  fprintf(output, "%i, %i, %i, %i R\n", x , y , w, h);
  //  fprintf(output, "GR\n");
  fprintf(output, "GS\n");
  fprintf(output,"BP\n");
  fprintf(output, "%i %i MT\n", x , y);
  fprintf(output, "%i %i LT\n", x+w-1 , y);
  fprintf(output, "%i %i LT\n", x+w-1 , y+h-1);
  fprintf(output, "%i %i LT\n", x , y+h-1);
  fprintf(output, "ECP\n");
  fprintf(output, "GR\n");
}

void Fl_PostScript_Graphics_Driver::rectf(int x, int y, int w, int h) {
  fprintf(output, "%g %g %i %i FR\n", x-0.5, y-0.5, w, h);
}

void Fl_PostScript_Graphics_Driver::line(int x1, int y1, int x2, int y2) {
  fprintf(output, "GS\n");
  fprintf(output, "%i %i %i %i L\n", x1 , y1, x2 ,y2);
  fprintf(output, "GR\n");
}

void Fl_PostScript_Graphics_Driver::line(int x0, int y0, int x1, int y1, int x2, int y2) {
  fprintf(output, "GS\n");
  fprintf(output,"BP\n");
  fprintf(output, "%i %i MT\n", x0 , y0);
  fprintf(output, "%i %i LT\n", x1 , y1);
  fprintf(output, "%i %i LT\n", x2 , y2);
  fprintf(output, "ELP\n");
  fprintf(output, "GR\n");
}

void Fl_PostScript_Graphics_Driver::xyline(int x, int y, int x1, int y2, int x3){
  fprintf(output, "GS\n");
  fprintf(output,"BP\n");
  fprintf(output, "%i %i MT\n", x , y );
  fprintf(output, "%i %i LT\n", x1 , y );
  fprintf(output, "%i %i LT\n", x1 , y2);
  fprintf(output,"%i %i LT\n", x3 , y2);
  fprintf(output, "ELP\n");
  fprintf(output, "GR\n");
};


void Fl_PostScript_Graphics_Driver::xyline(int x, int y, int x1, int y2){
  
  fprintf(output, "GS\n");
  fprintf(output,"BP\n");
  fprintf(output, "%i %i MT\n", x , y);
  fprintf(output,"%i %i LT\n", x1 , y);
  fprintf(output, "%i %i LT\n", x1 , y2 );
  fprintf(output, "ELP\n");
  fprintf(output, "GR\n");
};

void Fl_PostScript_Graphics_Driver::xyline(int x, int y, int x1){
  fprintf(output, "GS\n");
  fprintf(output,"BP\n");
  fprintf(output, "%i %i MT\n", x , y);
  fprintf(output, "%i %i LT\n", x1 , y );
  fprintf(output, "ELP\n");
  
  fprintf(output, "GR\n");
};

void Fl_PostScript_Graphics_Driver::yxline(int x, int y, int y1, int x2, int y3){
  fprintf(output, "GS\n");
  
  fprintf(output,"BP\n");
  fprintf(output,"%i %i MT\n", x , y);
  fprintf(output, "%i %i LT\n", x , y1 );
  fprintf(output, "%i %i LT\n", x2 , y1 );
  fprintf(output , "%i %i LT\n", x2 , y3);
  fprintf(output, "ELP\n");
  fprintf(output, "GR\n");
};

void Fl_PostScript_Graphics_Driver::yxline(int x, int y, int y1, int x2){
  fprintf(output, "GS\n");
  fprintf(output,"BP\n");
  fprintf(output, "%i %i MT\n", x , y);
  fprintf(output, "%i %i LT\n", x , y1);
  fprintf(output, "%i %i LT\n", x2 , y1);
  fprintf(output, "ELP\n");
  fprintf(output, "GR\n");
};

void Fl_PostScript_Graphics_Driver::yxline(int x, int y, int y1){
  fprintf(output, "GS\n");
  fprintf(output,"BP\n");
  fprintf(output, "%i %i MT\n", x , y);
  fprintf(output, "%i %i LT\n", x , y1);
  fprintf(output, "ELP\n");
  fprintf(output, "GR\n");
};

void Fl_PostScript_Graphics_Driver::loop(int x0, int y0, int x1, int y1, int x2, int y2) {
  fprintf(output, "GS\n");
  fprintf(output,"BP\n");
  fprintf(output, "%i %i MT\n", x0 , y0);
  fprintf(output, "%i %i LT\n", x1 , y1);
  fprintf(output, "%i %i LT\n", x2 , y2);
  fprintf(output, "ECP\n");
  fprintf(output, "GR\n");
}

void Fl_PostScript_Graphics_Driver::loop(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3) {
  fprintf(output, "GS\n");
  fprintf(output,"BP\n");
  fprintf(output, "%i %i MT\n", x0 , y0);
  fprintf(output, "%i %i LT\n", x1 , y1);
  fprintf(output, "%i %i LT\n", x2 , y2);
  fprintf(output, "%i %i LT\n", x3 , y3);
  fprintf(output, "ECP\n");
  fprintf(output, "GR\n");
}

void Fl_PostScript_Graphics_Driver::polygon(int x0, int y0, int x1, int y1, int x2, int y2) {
  fprintf(output, "GS\n");
  fprintf(output,"BP\n");
  fprintf(output, "%i %i MT\n", x0 , y0);
  fprintf(output,"%i %i LT\n", x1 , y1);
  fprintf(output, "%i %i LT\n", x2 , y2);
  fprintf(output, "EFP\n");
  fprintf(output, "GR\n");
}

void Fl_PostScript_Graphics_Driver::polygon(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3) {
  fprintf(output, "GS\n");
  fprintf(output,"BP\n");
  fprintf(output, "%i %i MT\n", x0 , y0 );
  fprintf(output, "%i %i LT\n", x1 , y1 );
  fprintf(output, "%i %i LT\n", x2 , y2 );
  fprintf(output, "%i %i LT\n", x3 , y3 );
  
  fprintf(output, "EFP\n");
  fprintf(output, "GR\n");
}

void Fl_PostScript_Graphics_Driver::point(int x, int y){
  rectf(x,y,1,1);
}

static int dashes_flat[5][7]={
{-1,0,0,0,0,0,0},
{3,1,-1,0,0,0,0},
{1,1,-1,0,0,0,0},
{3,1,1,1,-1,0,0},
{3,1,1,1,1,1,-1}
};


//yeah, hack...
static double dashes_cap[5][7]={
{-1,0,0,0,0,0,0},
{2,2,-1,0,0,0,0},
{0.01,1.99,-1,0,0,0,0},
{2,2,0.01,1.99,-1,0,0},
{2,2,0.01,1.99,0.01,1.99,-1}
};


void Fl_PostScript_Graphics_Driver::line_style(int style, int width, char* dashes){
  //line_styled_=1;
  
  linewidth_=width;
  linestyle_=style;
  //dashes_= dashes;
  if(dashes){
    if(dashes != linedash_)
      strcpy(linedash_,dashes);
    
  }else
    linedash_[0]=0;
  char width0 = 0;
  if(!width){
    width=1; //for screen drawing compatability
    width0=1;
  }
  
  fprintf(output, "%i setlinewidth\n", width);
  
  if(!style && (!dashes || !(*dashes)) && width0) //system lines
    style = FL_CAP_SQUARE;
  
  int cap = (style &0xf00) >> 8;
  if(cap) cap--;
  fprintf(output,"%i setlinecap\n", cap);
  
  int join = (style & 0xf000) >> 12;
  
  if(join) join--;
  fprintf(output,"%i setlinejoin\n", join);
  
  
  fprintf(output, "[");
  if(dashes && *dashes){
    while(*dashes){
      fprintf(output, "%i ", *dashes);
      dashes++;
    }
  }else{
    int * ds; 
    if(style & 0x200){ // round and square caps, dash length need to be adjusted
      double *dt = dashes_cap[style & 0xff];
      while (*dt >= 0){
	fprintf(output, "%g ",width * (*dt));
	dt++;
      }
    }else{
      
      ds = dashes_flat[style & 0xff];
      while (*ds >= 0){
	fprintf(output, "%i ",width * (*ds));
        ds++;
      }
    }
  }
  fprintf(output, "] 0 setdash\n");
};

static const char *_fontNames[] = {
"Helvetica",
"Helvetica-Bold",
"Helvetica-Oblique",
"Helvetica-BoldOblique",
"Courier",
"Courier-Bold",
"Courier-Oblique",
"Courier-BoldOblique",
"Times",
"Times-Bold",
"Times-Italic",
"Times-BoldItalic",
"Symbol",
"Courier",
"Courier-Bold",
"ZapfDingbats"
};

void Fl_PostScript_Graphics_Driver::font(int f, int s) {
  if (f >= FL_FREE_FONT)
    f = FL_COURIER;
  fprintf(output, "/%s SF\n" , _fontNames[f]);
  fprintf(output,"%i FS\n", s);
  Fl_Display_Device::display_device()->driver()->font(f,s); // Use display fonts for font measurement
  font_ = f; size_ = s;
};

void Fl_PostScript_Graphics_Driver::color(Fl_Color c) {
  //colored_=1;
  color_=c;
  Fl::get_color(c, cr_, cg_, cb_);
  if (cr_==cg_ && cg_==cb_) {
    double gray = cr_/255.0;
    fprintf(output, "%g GL\n", gray);
    
  } else {
    double fr, fg, fb;
    fr = cr_/255.0;
    fg = cg_/255.0;
    fb = cb_/255.0;
    fprintf(output,"%g %g %g SRGB\n", fr , fg , fb);
  }
}

void Fl_PostScript_Graphics_Driver::color(unsigned char r, unsigned char g, unsigned char b) {
  //colored_=1;
  cr_=r;cg_=g;cb_=b;
  if (r==g && g==b) {
    double gray = r/255.0;
    fprintf(output, "%g GL\n", gray);
  } else {
    double fr, fg, fb;
    fr = r/255.0;
    fg = g/255.0;
    fb = b/255.0;
    fprintf(output, "%g %g %g SRGB\n", fr , fg , fb);
  }
}

void Fl_PostScript_Graphics_Driver::draw(int angle, const char *str, int n, int x, int y)
{
  fprintf(output, "GS %d %d translate %d rotate\n", x, y, - angle);
  this->transformed_draw(str, n, 0, 0);
  fprintf(output, "GR\n");
}

// outputs in PostScript a UTF8 string replacing non-Latin1 characters by ?
// and using the same width in points as on display
void Fl_PostScript_Graphics_Driver::transformed_draw(const char* str, int n, double x, double y) {
  int len;
  if (!n || !str || !*str) return;
  const char *last = str + n;
  // compute display width of string
  fprintf(output,"%g (", fl_width(str, n));
  while (str < last) {
    // Extract each unicode character of string.
    // Until 0xFF, UTF codes coincide with iso-Latin1 (iso-8859-1)
    unsigned utf = fl_utf8decode(str, last, &len);
    str += len;
    if (utf > 0xFF) {
      utf = '?'; // replace non Latin-1 unicodes by ?
      }
    else if (utf == '(' || utf == ')' || utf == '\\') {
      putc('\\' , output); // these chars need be escaped
     }
    putc(utf, output); // output the latin character
  }
  fprintf(output, ") %g %g show_pos_width\n", x, y);
}

struct matrix {double a, b, c, d, x, y;};
extern matrix * fl_matrix;

void Fl_PostScript_Graphics_Driver::concat(){
  fprintf(output,"[%g %g %g %g %g %g] CT\n", fl_matrix->a , fl_matrix->b , fl_matrix->c , fl_matrix->d , fl_matrix->x , fl_matrix->y);
}

void Fl_PostScript_Graphics_Driver::reconcat(){
  fprintf(output, "[%g %g %g %g %g %g] RCT\n" , fl_matrix->a , fl_matrix->b , fl_matrix->c , fl_matrix->d , fl_matrix->x , fl_matrix->y);
}

/////////////////  transformed (double) drawings ////////////////////////////////


void Fl_PostScript_Graphics_Driver::begin_points(){
  fprintf(output, "GS\n");
  concat();
  
  fprintf(output, "BP\n");
  gap_=1;
  shape_=POINTS;
};

void Fl_PostScript_Graphics_Driver::begin_line(){
  fprintf(output, "GS\n");
  concat();
  fprintf(output, "BP\n");
  gap_=1;
  shape_=LINE;
};

void Fl_PostScript_Graphics_Driver::begin_loop(){
  fprintf(output, "GS\n");
  concat();
  fprintf(output, "BP\n");
  gap_=1;
  shape_=LOOP;
};

void Fl_PostScript_Graphics_Driver::begin_polygon(){
  fprintf(output, "GS\n");
  concat();
  fprintf(output, "BP\n");
  gap_=1;
  shape_=POLYGON;
};

void Fl_PostScript_Graphics_Driver::vertex(double x, double y){
  if(shape_==POINTS){
    fprintf(output,"%g %g MT\n", x , y);
    gap_=1;
    return;
  }
  if(gap_){
    fprintf(output,"%g %g MT\n", x , y);
    gap_=0;
  }else
    fprintf(output, "%g %g LT\n", x , y);
};

void Fl_PostScript_Graphics_Driver::curve(double x, double y, double x1, double y1, double x2, double y2, double x3, double y3){
  if(shape_==NONE) return;
  if(gap_)
    fprintf(output,"%g %g MT\n", x , y);
  else
    fprintf(output, "%g %g LT\n", x , y);
  gap_=0;
  
  fprintf(output, "%g %g %g %g %g %g curveto \n", x1 , y1 , x2 , y2 , x3 , y3);
};


void Fl_PostScript_Graphics_Driver::circle(double x, double y, double r){
  if(shape_==NONE){
    fprintf(output, "GS\n");
    concat();
    //    fprintf(output, "BP\n");
    fprintf(output,"%g %g %g 0 360 arc\n", x , y , r);
    reconcat();
    //    fprintf(output, "ELP\n");
    fprintf(output, "GR\n");
  }else
    
    fprintf(output, "%g %g %g 0 360 arc\n", x , y , r);
  
};

void Fl_PostScript_Graphics_Driver::arc(double x, double y, double r, double start, double a){
  if(shape_==NONE) return;
  gap_=0;
  if(start>a)
    fprintf(output, "%g %g %g %g %g arc\n", x , y , r , -start, -a);
  else
    fprintf(output, "%g %g %g %g %g arcn\n", x , y , r , -start, -a);
  
};

void Fl_PostScript_Graphics_Driver::arc(int x, int y, int w, int h, double a1, double a2) {
  fprintf(output, "GS\n");
  //fprintf(output, "BP\n");
  begin_line();
  fprintf(output, "%g %g TR\n", x + w/2.0 -0.5 , y + h/2.0 - 0.5);
  fprintf(output, "%g %g SC\n", (w-1)/2.0 , (h-1)/2.0 );
  arc(0,0,1,a2,a1);
  //  fprintf(output, "0 0 1 %g %g arc\n" , -a1 , -a2);
  fprintf(output, "%g %g SC\n", 2.0/(w-1) , 2.0/(h-1) );
  fprintf(output, "%g %g TR\n", -x - w/2.0 +0.5 , -y - h/2.0 +0.5);
  end_line();
  
  //  fprintf(output, "%g setlinewidth\n",  2/sqrt(w*h));
  //  fprintf(output, "ELP\n");
  //  fprintf(output, 2.0/w , 2.0/w , " SC\n";
  //  fprintf(output, (-x - w/2.0) , (-y - h/2)  , " TR\n";
  fprintf(output, "GR\n");
}

void Fl_PostScript_Graphics_Driver::pie(int x, int y, int w, int h, double a1, double a2) {
  
  fprintf(output, "GS\n");
  fprintf(output, "%g %g TR\n", x + w/2.0 -0.5 , y + h/2.0 - 0.5);
  fprintf(output, "%g %g SC\n", (w-1)/2.0 , (h-1)/2.0 );
  begin_polygon();
  vertex(0,0);
  arc(0.0,0.0, 1, a2, a1);
  end_polygon();
  fprintf(output, "GR\n");
}

void Fl_PostScript_Graphics_Driver::end_points(){
  gap_=1;
  reconcat();
  fprintf(output, "ELP\n"); //??
  fprintf(output, "GR\n");
  shape_=NONE;
}

void Fl_PostScript_Graphics_Driver::end_line(){
  gap_=1;
  reconcat();
  fprintf(output, "ELP\n");
  fprintf(output, "GR\n");
  shape_=NONE;
}
void Fl_PostScript_Graphics_Driver::end_loop(){
  gap_=1;
  reconcat();
  fprintf(output, "ECP\n");
  fprintf(output, "GR\n");
  shape_=NONE;
}

void Fl_PostScript_Graphics_Driver::end_polygon(){
  
  gap_=1;
  reconcat();
  fprintf(output, "EFP\n");
  fprintf(output, "GR\n");
  shape_=NONE;
}

void Fl_PostScript_Graphics_Driver::transformed_vertex(double x, double y){
  reconcat();
  if(gap_){
    fprintf(output, "%g %g MT\n", x , y);
    gap_=0;
  }else
    fprintf(output, "%g %g LT\n", x , y);
  concat();
};

/////////////////////////////   Clipping /////////////////////////////////////////////

void Fl_PostScript_Graphics_Driver::push_clip(int x, int y, int w, int h) {
  Clip * c=new Clip();
  clip_box(x,y,w,h,c->x,c->y,c->w,c->h);
  c->prev=clip_;
  clip_=c;
  fprintf(output, "CR\nCS\n");
  if(lang_level_<3)
    recover();
  fprintf(output, "%g %g %i %i CL\n", clip_->x-0.5 , clip_->y-0.5 , clip_->w  , clip_->h);
  
}

void Fl_PostScript_Graphics_Driver::push_no_clip() {
  Clip * c = new Clip();
  c->prev=clip_;
  clip_=c;
  clip_->x = clip_->y = clip_->w = clip_->h = -1;
  fprintf(output, "CR\nCS\n");
  if(lang_level_<3)
    recover();
}

void Fl_PostScript_Graphics_Driver::pop_clip() {
  if(!clip_)return;
  Clip * c=clip_;
  clip_=clip_->prev;
  delete c;
  fprintf(output, "CR\nCS\n");
  if(clip_ && clip_->w >0)
    fprintf(output, "%g %g %i %i CL\n", clip_->x - 0.5, clip_->y - 0.5, clip_->w  , clip_->h);
  // uh, -0.5 is to match screen clipping, for floats there should be something beter
  if(lang_level_<3)
    recover();
}

int Fl_PostScript_Graphics_Driver::clip_box(int x, int y, int w, int h, int &X, int &Y, int &W, int &H){
  if(!clip_){
    X=x;Y=y;W=w;H=h;
    return 1;
  }
  if(clip_->w < 0){
    X=x;Y=y;W=w;H=h;
    return 1;
  }
  int ret=0;
  if (x > (X=clip_->x)) {X=x; ret=1;}
  if (y > (Y=clip_->y)) {Y=y; ret=1;}
  if ((x+w) < (clip_->x+clip_->w)) {
    W=x+w-X;
    
    ret=1;
    
  }else
    W = clip_->x + clip_->w - X;
  if(W<0){
    W=0;
    return 1;
  }
  if ((y+h) < (clip_->y+clip_->h)) {
    H=y+h-Y;
    ret=1;
  }else
    H = clip_->y + clip_->h - Y;
  if(H<0){
    W=0;
    H=0;
    return 1;
  }
  return ret;
};

int Fl_PostScript_Graphics_Driver::not_clipped(int x, int y, int w, int h){
  if(!clip_) return 1;
  if(clip_->w < 0) return 1;
  int X, Y, W, H;
  clip_box(x, y, w, h, X, Y, W, H);
  if(W) return 1;
  return 0;
};


void Fl_PostScript_File_Device::margins(int *left, int *top, int *right, int *bottom) // to implement
{
  Fl_PostScript_Graphics_Driver *ps = driver();
  if(left) *left = (int)(ps->left_margin / ps->scale_x + .5);
  if(right) *right = (int)(ps->left_margin / ps->scale_x + .5);
  if(top) *top = (int)(ps->top_margin / ps->scale_y + .5);
  if(bottom) *bottom = (int)(ps->top_margin / ps->scale_y + .5);
}

int Fl_PostScript_File_Device::printable_rect(int *w, int *h)
//returns 0 iff OK
{
  Fl_PostScript_Graphics_Driver *ps = driver();
  if(w) *w = (int)((ps->pw_ - 2 * ps->left_margin) / ps->scale_x + .5);
  if(h) *h = (int)((ps->ph_ - 2 * ps->top_margin) / ps->scale_y + .5);
  return 0;
}

void Fl_PostScript_File_Device::origin(int x, int y)
{
  x_offset = x;
  y_offset = y;
  Fl_PostScript_Graphics_Driver *ps = driver();
  fprintf(ps->output, "GR GR GS %d %d TR  %f %f SC %d %d TR %f rotate GS\n", 
	  ps->left_margin, ps->top_margin, ps->scale_x, ps->scale_y, x, y, ps->angle);
}

void Fl_PostScript_File_Device::scale (float s_x, float s_y)
{
  Fl_PostScript_Graphics_Driver *ps = driver();
  ps->scale_x = s_x;
  ps->scale_y = s_y;
  fprintf(ps->output, "GR GR GS %d %d TR  %f %f SC %f rotate GS\n", 
	  ps->left_margin, ps->top_margin, ps->scale_x, ps->scale_y, ps->angle);
}

void Fl_PostScript_File_Device::rotate (float rot_angle)
{
  Fl_PostScript_Graphics_Driver *ps = driver();
  ps->angle = - rot_angle;
  fprintf(ps->output, "GR GR GS %d %d TR  %f %f SC %d %d TR %f rotate GS\n", 
	  ps->left_margin, ps->top_margin, ps->scale_x, ps->scale_y, x_offset, y_offset, ps->angle);
}

void Fl_PostScript_File_Device::translate(int x, int y)
{
  fprintf(driver()->output, "GS %d %d translate GS\n", x, y);
}

void Fl_PostScript_File_Device::untranslate(void)
{
  fprintf(driver()->output, "GR GR\n");
}

int Fl_PostScript_File_Device::start_page (void)
{
  Fl_PostScript_Graphics_Driver *ps = driver();
  ps->page(ps->page_format_);
  x_offset = 0;
  y_offset = 0;
  ps->scale_x = ps->scale_y = 1.;
  ps->angle = 0;
  fprintf(ps->output, "GR GR GS %d %d translate GS\n", ps->left_margin, ps->top_margin);
  return 0;
}

int Fl_PostScript_File_Device::end_page (void)
{
  return 0;
}

void Fl_PostScript_File_Device::end_job (void)
// finishes PostScript & closes file
{
  Fl_PostScript_Graphics_Driver *ps = driver();
  if (ps->nPages) {  // for eps nPages is 0 so it is fine ....
    fprintf(ps->output, "CR\nGR\nGR\nGR\nSP\n restore\n");
    if (!ps->pages_){
      fprintf(ps->output, "%%%%Trailer\n");
      fprintf(ps->output, "%%%%Pages: %i\n" , ps->nPages);
    };
  } else
    fprintf(ps->output, "GR\n restore\n");
  fputs("%%EOF",ps->output);
  ps->reset();
  fflush(ps->output);
  if(ferror(ps->output)) {
    fl_alert ("Error during PostScript data output.");
    }
#if ! (defined(__APPLE__) || defined(WIN32) )
  if (print_pipe)
    pclose(ps->output);
  else
    fclose(ps->output);
#else
  fclose(ps->output);
#endif
  while (ps->clip_){
    Fl_PostScript_Graphics_Driver::Clip * c= ps->clip_;
    ps->clip_= ps->clip_->prev;
    delete c;
  }
  if (ps->close_cmd_) (*ps->close_cmd_)(ps->output);
  Fl_Display_Device::display_device()->set_current();
}

#if ! (defined(__APPLE__) || defined(WIN32) )
int Fl_Printer::start_job(int pages, int *firstpage, int *lastpage) {
  enum Fl_PostScript_Graphics_Driver::Page_Format format;
  enum Fl_PostScript_Graphics_Driver::Page_Layout layout;

  // first test version for print dialog
  if (!print_panel) make_print_panel();
  print_load();
  print_selection->deactivate();
  print_all->setonly();
  print_all->do_callback();
  print_from->value("1");
  { char tmp[10]; snprintf(tmp, sizeof(tmp), "%d", pages); print_to->value(tmp); }
  print_panel->show(); // this is modal
  while (print_panel->shown()) Fl::wait();
  
  if (!print_start) // user clicked cancel
    return 1;

  // get options

  format = print_page_size->value() ? Fl_PostScript_Graphics_Driver::A4 : Fl_PostScript_Graphics_Driver::LETTER;
  { // page range choice
    int from = 1, to = pages;
    if (print_pages->value()) {
      sscanf(print_from->value(), "%d", &from);
      sscanf(print_to->value(), "%d", &to);
    }
    if (from < 1) from = 1;
    if (to > pages) to = pages;
    if (to < from) to = from;
    if (firstpage) *firstpage = from;
    if (lastpage) *lastpage = to;
    pages = to - from + 1;
  }
  
  if (print_output_mode[0]->value()) layout = Fl_PostScript_Graphics_Driver::PORTRAIT;
  else if (print_output_mode[1]->value()) layout = Fl_PostScript_Graphics_Driver::LANDSCAPE;
  else if (print_output_mode[2]->value()) layout = Fl_PostScript_Graphics_Driver::PORTRAIT;
  else layout = Fl_PostScript_Graphics_Driver::LANDSCAPE;

  print_pipe = print_choice->value();	// 0 = print to file, >0 = printer (pipe)

  const char *media = print_page_size->text(print_page_size->value());
  const char *printer = (const char *)print_choice->menu()[print_choice->value()].user_data();
  if (!print_pipe) printer = "<File>";

  if (!print_pipe) // fall back to file printing
    return Fl_PostScript_File_Device::start_job (pages, format, layout);

  // Print: pipe the output into the lp command...

  char command[1024];
  snprintf(command, sizeof(command), "lp -s -d %s -n %d -t '%s' -o media=%s",
             printer, print_collate_button->value() ? 1 : (int)(print_copies->value() + 0.5),
	     "FLTK", media);

  Fl_PostScript_Graphics_Driver *ps = driver();
  ps->output = popen(command, "w");
  if (!ps->output) {
    fl_alert("could not run command: %s\n",command);
    return 1;
  }
  this->set_current();
  return ps->start_postscript(pages, format, layout); // start printing
}

#endif // ! (defined(__APPLE__) || defined(WIN32) )

#endif // FL_DOXYGEN

//
// End of "$Id: Fl_PostScript.cxx 7653 2010-06-24 08:55:04Z manolo $".
//
