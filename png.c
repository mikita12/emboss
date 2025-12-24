#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <mmintrin.h>

#define ERROR                                                   \
	fprintf (stderr, "ERROR at %s:%d.\n", __FILE__, __LINE__) ;   \
	return -1 ;                                                   \

#include <mmintrin.h> // MMX intrinsics
#include <stdint.h>

extern void timer_start();
extern void timer_stop();

void filter(unsigned char* M, unsigned char* W, int width, int height) {
    // Kernel spłaszczony
    const int16_t kernel[9] = {
        -1, -1, 0,
        -1,  0, 1,
         0,  1, 1
    };

    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            uint8_t pixels[8];
            int16_t weights[8];

            int idx = 0;
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    if (idx < 8) { // pomijamy ostatni piksel
                        pixels[idx] = M[(y + ky) * width + (x + kx)];
                        weights[idx] = kernel[(ky + 1) * 3 + (kx + 1)];
                        idx++;
                    }
                }
            }

            __m64 mm_pixels = _mm_unpacklo_pi8(
                *(__m64*)pixels,
                _mm_setzero_si64()
            );

            __m64 mm_weights_low = *(__m64*)weights;

            __m64 mm_mul = _mm_mullo_pi16(mm_pixels, mm_weights_low);

            int16_t results[4];
            *(__m64*)results = mm_mul;

            int sum = 0;
            for (int i = 0; i < 4; i++) {
                sum += results[i];
            }

            __m64 mm_pixels_high = _mm_unpackhi_pi8(
                *(__m64*)pixels,
                _mm_setzero_si64()
            );
            __m64 mm_weights_high = *(__m64*)(weights + 4);

            __m64 mm_mul_high = _mm_mullo_pi16(mm_pixels_high, mm_weights_high);
            *(__m64*)results = mm_mul_high;

            for (int i = 0; i < 4; i++) {
                sum += results[i];
            }

            int last_pixel = M[(y + 1) * width + (x + 1)];
            sum += last_pixel * kernel[8];

            int normalized = (sum + 765)/6;
            if (normalized < 0) normalized = 0;
            if (normalized > 255) normalized = 255;

            W[y * width + x] = (unsigned char)normalized;
        }
    }

    for (int x = 0; x < width; x++) {
        W[x] = 0;
        W[(height - 1) * width + x] = 0;
    }
    for (int y = 0; y < height; y++) {
        W[y * width] = 0;
        W[y * width + (width - 1)] = 0;
    }

    _mm_empty();
}


/*void filter(unsigned char* M, unsigned char* W, int width, int height) {
    const int kernel[3][3] = {
        {-1, -1,  0},
        {-1,  0,  1},
        { 0,  1,  1}
    };

    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            int sum = 0;
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    int pixel = M[(y + ky) * width + (x + kx)];
                    sum += pixel * kernel[ky + 1][kx + 1];
                }
            }
            int normalized = (sum + 755) / 6;
            if (normalized < 0) normalized = 0;
            if (normalized > 255) normalized = 255;
            W[y * width + x] = (unsigned char)normalized;
        }
    }

    // Brzegi wypełniamy 0
    for (int x = 0; x < width; x++) {
        W[x] = 0;
        W[(height - 1) * width + x] = 0;
    }
    for (int y = 0; y < height; y++) {
        W[y * width] = 0;
        W[y * width + (width - 1)] = 0;
    }
}*/



int main (int argc, char ** argv)
{
	if (2 != argc)
	{
		printf ("\nUsage:\n\n%s file_name.png\n\n", argv[0]) ;

		return 0 ;
	}

	const char * file_name = argv [1] ;
	
	#define HEADER_SIZE (1)
	unsigned char header [HEADER_SIZE] ;

	FILE *fp = fopen (file_name, "rb");
	if (NULL == fp)
	{
		fprintf (stderr, "Can not open file \"%s\".\n", file_name) ;
		ERROR
	}

	if (fread (header, 1, HEADER_SIZE, fp) != HEADER_SIZE)
	{
		ERROR
	}

	if (0 != png_sig_cmp (header, 0, HEADER_SIZE))
	{
		ERROR
	}

	png_structp png_ptr = 
		png_create_read_struct
			(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
	if (NULL == png_ptr)
	{
		ERROR
	}

	png_infop info_ptr = png_create_info_struct (png_ptr);
	if (NULL == info_ptr)
	{
		png_destroy_read_struct (& png_ptr, NULL, NULL);

		ERROR
	}

	if (setjmp (png_jmpbuf (png_ptr))) 
	{
		png_destroy_read_struct (& png_ptr, & info_ptr, NULL);

		ERROR
	}

	png_init_io       (png_ptr, fp);
	png_set_sig_bytes (png_ptr, HEADER_SIZE);
	png_read_info     (png_ptr, info_ptr);

	png_uint_32  width, height;
	int  bit_depth, color_type;
	
	png_get_IHDR
	(
		png_ptr, info_ptr, 
		& width, & height, & bit_depth, & color_type,
		NULL, NULL, NULL
	);

	if (8 != bit_depth)
	{
		ERROR
	}
	if (0 != color_type)
	{
		ERROR
	}

	size_t size = width ;
	size *= height ;

	unsigned char * M = malloc (size) ;

	png_bytep ps [height] ;
	ps [0] = M ;
	for (unsigned i = 1 ; i < height ; i++)
	{
		ps [i] = ps [i-1] + width ;
	}
	png_set_rows (png_ptr, info_ptr, ps);
	png_read_image (png_ptr, ps) ;

	printf 
	(
		"Image %s loaded:\n"
		"\twidth      = %u\n"
		"\theight     = %u\n"
		"\tbit_depth  = %u\n"
		"\tcolor_type = %u\n"
		, file_name, width, height, bit_depth, color_type
	) ;

	unsigned char * W = malloc (size) ;


	timer_start();
	filter (M, W, width, height) ;
	timer_stop();



	png_structp write_png_ptr =
		png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (NULL == write_png_ptr)
	{
		ERROR
	}

	for (unsigned i = 0 ; i < height ; i++)
	{
		ps [i] += W - M ;
	}
	png_set_rows (write_png_ptr, info_ptr, ps);

	FILE *fwp = fopen ("out.png", "wb");
	if (NULL == fwp)
	{
		ERROR
	}

	png_init_io   (write_png_ptr, fwp);
	png_write_png (write_png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
	fclose (fwp);

	return 0;
}
