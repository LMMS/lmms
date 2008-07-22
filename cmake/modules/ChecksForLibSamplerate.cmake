#
# some tests migrated from libsamplerate's acinclude.m4 - Tobias Doerffel, 2008
#

INCLUDE(CheckCSourceCompiles)
INCLUDE(CheckCSourceRuns)

SET(CMAKE_REQUIRED_LIBRARIES_ORIG ${CMAKE_REQUIRED_LIBRARIES})
SET(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} -lm)
SET(C99_MATH "	#define         _ISOC9X_SOURCE  1
		#define         _ISOC99_SOURCE  1
		#define         __USE_ISOC99    1
		#define         __USE_ISOC9X    1

		#include <math.h>
					")
SET(TEST_LRINT "int main( void )
		{
			if (!lrint(3.14159)) lrint(2.7183);
			return( 0 );
		}")
SET(TEST_LRINTF "int main( void )
		{
			if (!lrintf(3.14159)) lrintf(2.7183);
			return( 0 );
		}")
CHECK_C_SOURCE_COMPILES("${C99_MATH}${TEST_LRINT}" HAVE_LRINT)
CHECK_C_SOURCE_COMPILES("${C99_MATH}${TEST_LRINTF}" HAVE_LRINTF)


CHECK_C_SOURCE_RUNS("
        #define _ISOC9X_SOURCE  1
        #define _ISOC99_SOURCE  1
        #define __USE_ISOC99    1
        #define __USE_ISOC9X    1
        #include <math.h>
        int main (void)
        {       double  fval ;
                int k, ival ;

                fval = 1.0 * 0x7FFFFFFF ;
                for (k = 0 ; k < 100 ; k++)
                {       ival = (lrint (fval)) >> 24 ;
                        if (ival != 127)
                                return 1 ;
                
                        fval *= 1.2499999 ;
                        } ;
                
                        return 0 ;
                }
				" CPU_CLIPS_POSITIVE)
CHECK_C_SOURCE_RUNS("
       #define _ISOC9X_SOURCE  1
        #define _ISOC99_SOURCE  1
        #define __USE_ISOC99    1
        #define __USE_ISOC9X    1
        #include <math.h>
        int main (void)
        {       double  fval ;
                int k, ival ;

                fval = -8.0 * 0x10000000 ;
                for (k = 0 ; k < 100 ; k++)
                {       ival = (lrint (fval)) >> 24 ;
                        if (ival != -128)
                                return 1 ;
                
                        fval *= 1.2499999 ;
                        } ;
                
                        return 0 ;
                }
				" CPU_CLIPS_NEGATIVE)
SET(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES_ORIG})

