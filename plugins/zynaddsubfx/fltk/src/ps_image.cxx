//
// "$Id: image.cxx 4324 2005-05-09 21:47:22Z rokan $"
//
// Postscript image drawing implementation for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2005 by Bill Spitzak and others.
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
// Please report all bugs and problems on the following page:
//
//     http://www.fltk.org/str.php
//

#ifndef FL_DOXYGEN

#include <stdio.h>
#include <math.h>
#include <string.h>

#include <FL/Fl_PostScript.H>
#include <FL/Fl.H>
#include <FL/Fl_Pixmap.H>
#include <FL/Fl_Bitmap.H>
 
int Fl_PostScript_Graphics_Driver::alpha_mask(const uchar * data, int w, int h, int D, int LD){

  mask = 0;
  if((D/2)*2 != D){ //no mask info
    return 0;
  }
  int xx;
  int i,j, k, l;
  LD += w*D;
  int V255=0;
  int V0 =0;
  int V_=0;
//  uchar d;
  for(j=0;j<h;j++){
    for(i=0;i<w;i++)
      switch(data[j*LD+D*i+D-1]){
        case 255: V255 = 1; break;
        case 0: V0 = 1; break;
        default: V_= 1;
      }
    if(V_) break;
  };
  if(!V_){
    if(V0)
      if(V255){// not true alpha, only masking
        xx = (w+7)/8;
        mask = new uchar[h * xx];
        for(i=0;i<h * xx;i++) mask[i]=0;
        for(j=0;j<h;j++)
          for(i=0;i<w;i++)
            if(data[j*LD+D*i+D-1])
              mask[j*xx+i/8] |= 1 << (i % 8);
        mx = w;
        my = h; //mask imensions
        return 0;
      }else{
        mask=0;
        return 1; //everything masked
      }
    else
      return 0;
  }



  /////   Alpha dither, generating (4*w) * 4 mask area       /////
  /////         with Floyd-Steinberg error diffusion         /////

  mask = new uchar[((w+1)/2) * h * 4];

  for(i=0;i<((w+1)/2) * h * 4; i++) mask[i] = 0; //cleaning



  mx= w*4;
  my=h*4; // mask dimensions

  xx = (w+1)/2;                //  mask line width in bytes

  short * errors1 = new short [w*4+2]; //  two rows of dither errors
  short * errors2 = new short [w*4+2]; //  two rows of dither errors

  for(i=0;i<w*4+2;i++) errors2[i] = 0; // cleaning,after first swap will become current
  for(i=0;i<w*4+2;i++) errors1[i] = 0; // cleaning,after first swap will become current

  short * current = errors1;
  short * next = errors2;
  short * swap;

  for(j=0;j<h;j++){
    for(l=0;l<4;){           // generating 4 rows of mask lines for 1 RGB line
      int jj = j*4+l;

      /// mask row index
      swap = next;
      next = current;
      current = swap;
      *(next+1) = 0;          // must clean the first cell, next are overriden by *1
      for(i=0;i<w;i++){
        for(k=0;k<4;k++){   // generating 4 x-pixels for 1 RGB
          short error, o1, o2, o3;
          int ii = i*4+k;   // mask cell index
          short val = data[j*LD+D*i+D-1] + current[1+ii];
          if (val>127){
            mask[jj*xx+ii/8]  |= 1 << (ii % 8); //set mask bit
            error =  val-255;
          }else
            error = val;

          ////// error spreading /////
          if(error >0){
            next[ii] +=  o1 = (error * 3 + 8)/16;
            current[ii+2] += o2 = (error * 7 + 8)/16;
            next[ii+2] = o3 =(error + 8)/16;  // *1 - ok replacing (cleaning)
          }else{
            next[ii] += o1 = (error * 3 - 8)/16;
            current[ii+2] += o2 = (error * 7 - 8)/16;
            next[ii+2] = o3 = (error - 8)/16;
          }
          next[1+ii] += error - o1 - o2 - o3;
        }
      }
      l++;

      ////// backward

      jj = j*4+l;
      swap = next;
      next = current;
      current = swap;
      *(next+1) = 0;          // must clean the first cell, next are overriden by *1

      for(i=w-1;i>=0;i--){

        for(k=3;k>=0;k--){   // generating 4 x-pixels for 1 RGB
          short error, o1, o2, o3;

          int ii = i*4+k;   // mask cell index
          short val = data[j*LD+D*i+D-1] + current[1+ii];
          if (val>127){

            mask[jj*xx+ii/8]  |= 1 << (ii % 8); //set mask bit
            error =  val-255;
          }else
            error = val;

          ////// error spreading /////
          if(error >0){
            next[ii+2] +=  o1 = (error * 3 + 8)/16;
            current[ii] += o2 = (error * 7 + 8)/16;
            next[ii] = o3 =(error + 8)/16;  // *1 - ok replacing (cleaning)
          }else{
            next[ii+2] += o1 = (error * 3 - 8)/16;

            current[ii] += o2 = (error * 7 - 8)/16;
            next[ii] = o3 = (error - 8)/16;
          }
          next[1+ii] += error - o1 - o2 - o3;
        }
      }
      l++;
    }
  }
  delete[] errors1;
  delete[] errors2;
  return 0;
}

// bitwise inversion of all 4-bit quantities
static const unsigned char swapped[16] = {0,8,4,12,2,10,6,14,1,9,5,13,3,11,7,15};

// bitwise inversion of a byte
static inline uchar swap_byte(const uchar b) {
  return (swapped[b & 0xF] << 4) | swapped[b >> 4];
}


extern uchar **fl_mask_bitmap;


void Fl_PostScript_Graphics_Driver::draw_scaled_image(const uchar *data, double x, double y, double w, double h, int iw, int ih, int D, int LD) {


  if(D<3){ //mono
    draw_scaled_image_mono(data, x, y, w, h, iw, ih, D, LD);
    return;
  }


  int i,j, k;

  fprintf(output,"save\n");

  const char * interpol;
  if(lang_level_>1){
    if(interpolate_)
      interpol="true";
    else
      interpol="false";
    if(mask && lang_level_>2)
      fprintf(output, "%g %g %g %g %i %i %i %i %s CIM\n", x , y+h , w , -h , iw , ih, mx, my, interpol);
    else
      fprintf(output, "%g %g %g %g %i %i %s CII\n", x , y+h , w , -h , iw , ih, interpol);
  }else
    fprintf(output , "%g %g %g %g %i %i CI", x , y+h , w , -h , iw , ih);


  if(!LD) LD = iw*D;
  uchar *curmask=mask;

  for (j=0; j<ih;j++){
    if(mask){

      for(k=0;k<my/ih;k++){
        for (i=0; i<((mx+7)/8);i++){
          if (!(i%80)) fprintf(output, "\n");
          fprintf(output, "%.2x",swap_byte(*curmask));
          curmask++;
        }
        fprintf(output,"\n");
      }
    }
    const uchar *curdata=data+j*LD;
    for(i=0 ; i<iw ; i++) {
      uchar r = curdata[0];
      uchar g =  curdata[1];
      uchar b =  curdata[2];
      if(lang_level_<3 && D>3) { //can do  mixing using bg_* colors)
        unsigned int a2 = curdata[3]; //must be int
        unsigned int a = 255-a2;
        r = (a2 * r + bg_r * a)/255;
        g = (a2 * g + bg_g * a)/255;
        b = (a2 * b + bg_b * a)/255;
      }
      if (!(i%40)) fprintf(output, "\n");
      fprintf(output, "%.2x%.2x%.2x", r, g, b);
      curdata +=D;
    }
    fprintf(output,"\n");

  }

  fprintf(output," >\nrestore\n" );


};

void Fl_PostScript_Graphics_Driver::draw_scaled_image(Fl_Draw_Image_Cb call, void *data, double x, double y, double w, double h, int iw, int ih, int D) {

  int level2_mask = 0;
  fprintf(output,"save\n");
  int i,j,k;
  const char * interpol;
  if (lang_level_ > 1) {
    if (interpolate_) interpol="true";
    else interpol="false";
    if (mask && lang_level_ > 2) {
      fprintf(output, "%g %g %g %g %i %i %i %i %s CIM\n", x , y+h , w , -h , iw , ih, mx, my, interpol);
      }
    else if (mask && lang_level_ == 2) {
      level2_mask = 1; // use method for drawing masked color image with PostScript level 2
      fprintf(output, "%d %d pixmap_size\n pixmap_loadmask\n", iw, ih);
    }
    else {
      fprintf(output, "%g %g %g %g %i %i %s CII\n", x , y+h , w , -h , iw , ih, interpol);
      }
  } else {
    fprintf(output , "%g %g %g %g %i %i CI", x , y+h , w , -h , iw , ih);
    }

  int LD=iw*D;
  uchar *rgbdata=new uchar[LD];
  uchar *curmask=mask;

  if(level2_mask) {
    for (j = ih - 1; j >= 0; j--) {
      curmask = mask + j * my/ih * ((mx+7)/8);
	for(k=0; k < my/ih; k++) { // output mask data
	  for (i=0; i < ((mx+7)/8); i++) {
	    fprintf(output, "%.2x",swap_byte(*curmask));
	    curmask++;
	  }
	  fprintf(output,"\n");
	}
      }
    fprintf(output,"pop def\n\npixmap_loaddata\n");
    for (j = ih - 1; j >= 0; j--) { // output full image data
      call(data,0,j,iw,rgbdata);
      uchar *curdata=rgbdata;
      for(i=0 ; i<iw ; i++) {
	uchar r = curdata[0];
	uchar g =  curdata[1];
	uchar b =  curdata[2];
	//if (!(i%40)) fprintf(output, "\n");
	fprintf(output, "%.2x%.2x%.2x", r, g, b);
	curdata += D;
	}
      fprintf(output,"\n");
      }
    fprintf(output,"pop def\n\n%g %g pixmap_plot\n", x, y); // draw the masked image
    }
  else {
    for (j=0; j<ih;j++) {
      if(mask && lang_level_ > 2) {  // InterleaveType 2 mask data
	for(k=0; k<my/ih;k++) { //for alpha pseudo-masking
	  for (i=0; i<((mx+7)/8);i++) {
	    if (!(i%40)) fprintf(output, "\n");
	    fprintf(output, "%.2x",swap_byte(*curmask));
	    curmask++;
	  }
	  fprintf(output,"\n");
	}
      }
      call(data,0,j,iw,rgbdata);
      uchar *curdata=rgbdata;
      for(i=0 ; i<iw ; i++) {
	uchar r = curdata[0];
	uchar g =  curdata[1];
	uchar b =  curdata[2];

	if (!(i%40)) fprintf(output, "\n");
	fprintf(output, "%.2x%.2x%.2x", r, g, b);

	curdata +=D;
      }
      fprintf(output,"\n");

    }
    fprintf(output,">\n");
    }

  fprintf(output,"restore\n");
  delete[] rgbdata;
}

void Fl_PostScript_Graphics_Driver::draw_scaled_image_mono(const uchar *data, double x, double y, double w, double h, int iw, int ih, int D, int LD) {

  fprintf(output,"save\n");

  int i,j, k;

  const char * interpol;
  if(lang_level_>1){
    if(interpolate_)
      interpol="true";
    else
      interpol="false";
    if(mask && lang_level_>2)
      fprintf(output, "%g %g %g %g %i %i %i %i %s GIM\n", x , y+h , w , -h , iw , ih, mx, my, interpol);
    else
      fprintf(output, "%g %g %g %g %i %i %s GII\n", x , y+h , w , -h , iw , ih, interpol);
  }else
    fprintf(output , "%g %g %g %g %i %i GI", x , y+h , w , -h , iw , ih);


  if(!LD) LD = iw*D;


  int bg = (bg_r + bg_g + bg_b)/3;

  uchar *curmask=mask;
  for (j=0; j<ih;j++){
    if(mask){
      for(k=0;k<my/ih;k++){
        for (i=0; i<((mx+7)/8);i++){
          if (!(i%80)) fprintf(output, "\n");
          fprintf(output, "%.2x",swap_byte(*curmask));
          curmask++;
        }
        fprintf(output,"\n");
      }
    }
    const uchar *curdata=data+j*LD;
    for(i=0 ; i<iw ; i++) {
      if (!(i%80)) fprintf(output, "\n");
      uchar r = curdata[0];
      if(lang_level_<3 && D>1) { //can do  mixing

        unsigned int a2 = curdata[1]; //must be int
        unsigned int a = 255-a2;
        r = (a2 * r + bg * a)/255;
      }
      if (!(i%120)) fprintf(output, "\n");
      fprintf(output, "%.2x", r);
      curdata +=D;
    }
    fprintf(output,"\n");

  }

  fprintf(output," >\nrestore\n" );

};



void Fl_PostScript_Graphics_Driver::draw_scaled_image_mono(Fl_Draw_Image_Cb call, void *data, double x, double y, double w, double h, int iw, int ih, int D) {

  fprintf(output,"save\n");
  int i,j,k;
  const char * interpol;
  if(lang_level_>1){
    if(interpolate_) interpol="true";
    else interpol="false";
    if(mask && lang_level_>2)
      fprintf(output, "%g %g %g %g %i %i %i %i %s GIM\n", x , y+h , w , -h , iw , ih, mx, my, interpol);
    else
      fprintf(output, "%g %g %g %g %i %i %s GII\n", x , y+h , w , -h , iw , ih, interpol);
  }else
    fprintf(output , "%g %g %g %g %i %i GI", x , y+h , w , -h , iw , ih);

  int LD=iw*D;
  uchar *rgbdata=new uchar[LD];
  uchar *curmask=mask;
  for (j=0; j<ih;j++){

    if(mask && lang_level_>2){  // InterleaveType 2 mask data
      for(k=0; k<my/ih;k++){ //for alpha pseudo-masking
        for (i=0; i<((mx+7)/8);i++){
          if (!(i%40)) fprintf(output, "\n");
          fprintf(output, "%.2x",swap_byte(*curmask));
          curmask++;
        }
        fprintf(output,"\n");
      }
    }
    call(data,0,j,iw,rgbdata);
    uchar *curdata=rgbdata;
    for(i=0 ; i<iw ; i++) {
      uchar r = curdata[0];
      if (!(i%120)) fprintf(output, "\n");
      fprintf(output, "%.2x", r);
      curdata +=D;
    }
    fprintf(output,"\n");
  }
  fprintf(output,">\n");
  fprintf(output,"restore\n");
  delete[] rgbdata;
}


////////////////////////////// Image classes //////////////////////


void Fl_PostScript_Graphics_Driver::draw(Fl_Pixmap * pxm,int XP, int YP, int WP, int HP, int cx, int cy){
  const char * const * di =pxm->data();
  int w,h;
  if (!fl_measure_pixmap(di, w, h)) return;
  mask=0;
  fl_mask_bitmap=&mask;
  mx = WP;
  my = HP;
  push_clip(XP, YP, WP, HP);
  fl_draw_pixmap(di,XP -cx, YP -cy, FL_BLACK );
  pop_clip();
  delete[] mask;
  mask=0;
  fl_mask_bitmap=0;
};

void Fl_PostScript_Graphics_Driver::draw(Fl_RGB_Image * rgb,int XP, int YP, int WP, int HP, int cx, int cy){
  const uchar  * di = rgb->array;
  int w = rgb->w();
  int h = rgb->h();
  mask=0;
  if(lang_level_>2) //when not true, not making alphamask, mixing colors instead...
  if (alpha_mask(di, w, h, rgb->d(),rgb->ld())) return; //everthing masked, no need for painting!
  push_clip(XP, YP, WP, HP);
  draw_scaled_image(di, XP + cx, YP + cy, w, h,  w,  h, rgb->d(), rgb->ld());
  pop_clip();
  delete[]mask;
  mask=0;
};

void Fl_PostScript_Graphics_Driver::draw(Fl_Bitmap * bitmap,int XP, int YP, int WP, int HP, int cx, int cy){
  const uchar  * di = bitmap->array;
  int w,h;
  int LD=(bitmap->w()+7)/8;
  int xx;

  if (WP> bitmap->w() - cx){// to assure that it does not go out of bounds;
     w = bitmap->w() - cx;
     xx = (bitmap->w()+7)/8 - cx/8; //length of mask in bytes
  }else{
    w =WP;
    xx = (w+7)/8 - cx/8;
  }
  if( HP > bitmap->h()-cy)
    h = bitmap->h() - cy;
  else
    h = HP;

  di += cy*LD + cx/8;
  int si = cx % 8; // small shift to be clipped, it is simpler than shifting whole mask

  int i,j;
  push_clip(XP, YP, WP, HP);
  fprintf(output , "%i %i %i %i %i %i MI", XP - si, YP + HP , WP , -HP , w , h);

  for (j=0; j<HP; j++){
    for (i=0; i<xx; i++){
      if (!(i%80)) fprintf(output, "\n"); // not have lines longer than 255 chars
      fprintf(output, "%.2x",swap_byte(~(*di)));
      di++;
    }
    fprintf(output,"\n");
  }
  fprintf(output,">\n");
  pop_clip();
};

#endif // FL_DOXYGEN

//
// End of "$Id: image.cxx 4324 2005-05-09 21:47:22Z rokan $"
//










  







