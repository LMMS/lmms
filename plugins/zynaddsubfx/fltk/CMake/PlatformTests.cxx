#ifdef HAVE_LIBZ

#include <zlib.h>

int main()
{
	unsigned long compressedSize = 0;
	unsigned char cd[100];
	const unsigned char ud[100] = "";
	unsigned long uncompressedSize = 0;

	// Call zlib's compress function.
	if(compress(cd, &compressedSize, ud, uncompressedSize) != Z_OK)
		{
		return 0;
		}
	return 1;
}


#endif

#ifdef HAVE_LIBJPEG

#include <stdio.h>
#include <jpeglib.h>

int main()
{
	struct jpeg_decompress_struct cinfo;
	jpeg_create_decompress(&cinfo);
	jpeg_read_header(&cinfo, TRUE);
	return 1;
}

#endif

#ifdef HAVE_LIBPNG
#include <png.h>
int main()
{
	png_structp png_ptr = png_create_read_struct
		(PNG_LIBPNG_VER_STRING, (png_voidp)NULL,
		 NULL, NULL);
	png_infop info_ptr = png_create_info_struct(png_ptr);
	png_set_sig_bytes(png_ptr, 8);
	png_read_info(png_ptr, info_ptr);

	return 0;
}
#endif

#ifdef HAVE_PNG_H
#include <png.h>
int main() { retunr 0;}
#endif

#ifdef HAVE_PNG_GET_VALID
#include <png.h>
int main()
{
	png_structp png_ptr = png_create_read_struct
		(PNG_LIBPNG_VER_STRING, (png_voidp)NULL,
		 NULL, NULL);
	png_infop info_ptr = png_create_info_struct(png_ptr);
	png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS);
	return 0;
}
#endif

#ifdef HAVE_PNG_SET_TRNS_TO_ALPHA
#include <png.h>
int main()
{
	png_structp png_ptr = png_create_read_struct
		(PNG_LIBPNG_VER_STRING, (png_voidp)NULL,
		 NULL, NULL);
	png_set_tRNS_to_alpha(png_ptr);
	return 0;
}
#endif
