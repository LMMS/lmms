/*
 * Copyright 1992 by Jutta Degener and Carsten Bormann, Technische
 * Universitaet Berlin.  See the accompanying file "COPYRIGHT" for
 * details.  THERE IS ABSOLUTELY NO WARRANTY FOR THIS SOFTWARE.
 */

/*$Header: /home/cvs/giga/ladspa-swh/gsm/config.h,v 1.1 2001/06/10 21:36:51 swh Exp $*/

#ifndef	CONFIG_H
#define	CONFIG_H

//*efine	SIGHANDLER_T	int 		/* signal handlers are void	*/
//*efine HAS_SYSV_SIGNAL	1		/* sigs not blocked/reset?	*/

#define	HAS_STDLIB_H	1		/* /usr/include/stdlib.h	*/
//*efine	HAS_LIMITS_H	1		/* /usr/include/limits.h	*/
#define	HAS_FCNTL_H	1		/* /usr/include/fcntl.h		*/
//*efine	HAS_ERRNO_DECL	1		/* errno.h declares errno	*/

#define	HAS_FSTAT 	1		/* fstat syscall		*/
#define	HAS_FCHMOD 	1		/* fchmod syscall		*/
#define	HAS_CHMOD 	1		/* chmod syscall		*/
#define	HAS_FCHOWN 	1		/* fchown syscall		*/
#define	HAS_CHOWN 	1		/* chown syscall		*/
//*efine	HAS__FSETMODE 	1		/* _fsetmode -- set file mode	*/

#define	HAS_STRING_H 	1		/* /usr/include/string.h 	*/
//*efine	HAS_STRINGS_H	1		/* /usr/include/strings.h 	*/

#define	HAS_UNISTD_H	1		/* /usr/include/unistd.h	*/
#define	HAS_UTIME	1		/* POSIX utime(path, times)	*/
//*efine	HAS_UTIMES	1		/* use utimes()	syscall instead	*/
#define	HAS_UTIME_H	1		/* UTIME header file		*/
//*efine	HAS_UTIMBUF	1		/* struct utimbuf		*/
//*efine	HAS_UTIMEUSEC   1		/* microseconds in utimbuf?	*/

#endif	/* CONFIG_H */
