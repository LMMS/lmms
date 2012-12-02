/*=============================================================================
   GNU UnRTF, a command-line program to convert RTF documents to other formats.
   Copyright (C) 2000,2001,2004 by Zachary Smith

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

   The maintainer is reachable by electronic mail at daved@physiol.usyd.edu.au
=============================================================================*/


/*----------------------------------------------------------------------
 * Module name:    html
 * Author name:    Zachary Smith
 * Create date:    18 Sep 01
 * Purpose:        HTML-specific output module
 *----------------------------------------------------------------------
 * Changes:
 * 01 Aug 01, tuorfa@yahoo.com: code moved over from convert.c
 * 03 Aug 01, tuorfa@yahoo.com: removed null entries to save space
 * 08 Aug 01, tuorfa@yahoo.com, gommer@gmx.net: fixed/added some ANSI chars
 * 18 Sep 01, tuorfa@yahoo.com: moved character sets into html.c etc
 * 22 Sep 01, tuorfa@yahoo.com: added function-level comment blocks 
 * 08 Oct 03, daved@physiol.usyd.edu.au: mac special character fixes
 * 29 Mar 05, daved@physiol.usyd.edu.au: changes requested by ZT Smith
 * 29 Mar 05, daved@physiol.usyd.edu.au: more unicode characters
 * 21 Jul 05, daved@physiol.usyd.edu.au: added endash
 * 19 Aug 05, ax2groin@arbornet.org: added more chars and changes to ANSI
 * 05 Jan 06, marcossamaral@terra.com.br: fixed bugs #14982 and #14983
 * 31 Oct 07, jasp00@users.sourceforge.net: replaced deprecated conversions
 * 13 Dec 07, daved@physiol.usyd.edu.au: fixed some missing entity ';'
 * 16 Dec 07, daved@physiol.usyd.edu.au: updated to GPL v3
 *--------------------------------------------------------------------*/

#ifdef LMMS_HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef LMMS_HAVE_STDIO_H
#include <stdio.h>
#endif

#ifdef LMMS_HAVE_STRING_H
#include <string.h>
#endif

#include "ur_malloc.h"
#include "defs.h"
#include "error.h"
#include "main.h"
#include "output.h"


static const char* ascii [96] = {
	/* 0x20 */ " ", "!", "&quot;", "#", "$", "%", "&amp;", "'", 
	/* 0x28 */ "(", ")", "*", "+", ",", "-", ".", "/", 
	/* 0x30 */ "0", "1", "2", "3", "4", "5", "6", "7", 
	/* 0x38 */ "8", "9", ":", ";", "&lt;", "=", "&gt;", "?", 
	/* 0x40 */ "@", "A", "B", "C", "D", "E", "F", "G", 
	/* 0x48 */ "H", "I", "J", "K", "L", "M", "N", "O", 
	/* 0x50 */ "P", "Q", "R", "S", "T", "U", "V", "W", 
	/* 0x58 */ "X", "Y", "Z", "[", "\\", "]", "^", "_", 
	/* 0x60 */ "`", "a", "b", "c", "d", "e", "f", "g", 
	/* 0x68 */ "h", "i", "j", "k", "l", "m", "n", "o", 
	/* 0x70 */ "p", "q", "r", "s", "t", "u", "v", "w", 
	/* 0x78 */ "x", "y", "z", "{", "|", "}", "~", "" 
};


static const char* ansi [] = {
/* 0x78 */ "x",
/* 0x79 */ "y",
/* 0x7a */ "z",
/* 0x7b */ "{",
/* 0x7c */ "|",
/* 0x7d */ "}",
/* 0x7e */ "~",
/* 0x7f */ "&#127;",
/* 0x80 */ "&euro;", /* &#128; may be more widely recognized. */
/* 0x81 */ "&#129;",
/* 0x82 */ "&#130;", /* &lsquor; not implemented in any browsers I've seen. */
/* 0x83 */ "&fnof;", 
/* 0x84 */ "&#132;", /* &ldquor; not implemented in any browsers I've seen. */
/* 0x85 */ "&hellip;",
/* 0x86 */ "&dagger;",
/* 0x87 */ "&Dagger;",
/* 0x88 */ "&circ;",
/* 0x89 */ "&permil;",
/* 0x8a */ "&Scaron;",
/* 0x8b */ "&lsaquo;",
/* 0x8c */ "&OElig;",
/* 0x8d */ "&#141;",
/* 0x8e */ "&Zcaron;",
/* 0x8f */ "&#143;",
/* 0x90 */ "&#144;", "&lsquo;", "&rsquo;", "&ldquo;", "&rdquo;", "&bull;", "&ndash;", "&mdash;", 
/* 0x98 */ "&#152;",
/* 0x99 */ "&trade;",
/* 0x9a */ "&scaron;",
/* 0x9b */ "&rsaquo;",	/* daved - 0.9.6 */
/* 0x9c */ "&oelig;",
/* 0x9d */ "&#157;",
/* 0x9e */ "&zcaron;",
/* 0x9f */ "&Yuml;",
/* 0xa0 */ "&nbsp;","&iexcl;","&cent;","&pound;","&curren;","&yen;","&brvbar;","&sect;",
/* 0xa8 */ "&uml;","&copy;","&ordf;","&laquo;","&not;","&shy;","&reg;","&macr;",
/* 0xb0 */ "&deg;", "&plusmn;","&sup2;","&sup3;","&acute;","&micro;","&para;","&middot;",
/* 0xb8 */ "&cedil;","&sup1", "&ordm;","&raquo;", "&frac14;", "&frac12;","&frac34;","&iquest;",
/* 0xc0 */ "&Agrave;","&Aacute;","&Acirc;","&Atilde;","&Auml;","&Aring;","&AElig;","&Ccedil;",
/* 0xc8 */ "&Egrave;","&Eacute;","&Ecirc;","&Euml;","&Igrave;","&Iacute;","&Icirc;","&Iuml;",
/* 0xd0 */ "&ETH;","&Ntilde;","&Ograve;","&Oacute;","&Ocirc;","&Otilde;","&Ouml;","&times;",
/* 0xd8 */ "&Oslash;","&Ugrave;","&Uacute;","&Ucirc;","&Uuml;","&Yacute;","&THORN;","&szlig;",
/* 0xe0 */ "&agrave;","&aacute;","&acirc;","&atilde;","&auml;","&aring;","&aelig;","&ccedil;",
/* 0xe8 */ "&egrave;","&eacute;","&ecirc;","&euml;","&igrave;","&iacute;","&icirc;","&iuml;",
/* 0xf0 */ "&eth;","&ntilde;","&ograve;","&oacute;","&ocirc;","&otilde;","&ouml;","&divide;",
/* 0xf8 */ "&oslash;","&ugrave;","&uacute;","&ucirc;","&uuml;","&yacute;","&thorn;","&yuml;"
};

static const char* mac [] = {
/* 0xa4 */ "&bull;", NULL,NULL,NULL,
/* 0xa8 */ NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
/* 0xb0 */ NULL,NULL,NULL,NULL,NULL,"&mu;",NULL,NULL,
/* 0xb8 */ NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
/* 0xc0 */ NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
/* 0xc8 */ NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
/* 0xd0 */ "&mdash;","&ndash;","&ldquo;","&rdquo;","&lquo;","&rquo;"
};

#if 1 /* daved - 0.19.4 - unicode symbol character support */
static const char * unisymbol1[] = {
	/* 913 */	"&Alpha;",
	/* 914 */	"&Beta;",
	/* 915 */	"&Gamma;",
	/* 916 */	"&Delta;",
	/* 917 */	"&Epsilon;",
	/* 918 */	"&Zeta;",
	/* 919 */	"&Eta;",
	/* 920 */	"&Theta;",
	/* 921 */	"&Iota;",
	/* 922 */	"&Kappa;",
	/* 923 */	"&Lambda;",
	/* 924 */	"&Mu;",
	/* 925 */	"&Nu;",
	/* 926 */	"&Xi;",
	/* 927 */	"&Omicron;",
	/* 928 */	"&Pi;",
	/* 929 */	"&Rho;",
	/* 930 */	0,
	/* 931 */	"&Sigma;",
	/* 932 */	"&Tau;",
	/* 933 */	"&Upsilon;",
	/* 934 */	"&Phi;",
	/* 935 */	"&Chi;",
	/* 936 */	"&Psi;",
	/* 937 */	"&Omega;",
	/* 938 */	0,
	/* 939 */	0,
	/* 940 */	0,
	/* 941 */	0,
	/* 942 */	0,
	/* 943 */	0,
	/* 944 */	0,
	/* 945 */	"&alpha;",
	/* 946 */	"&beta;",
	/* 947 */	"&gamma;",
	/* 948 */	"&delta;",
	/* 949 */	"&epsilon;",
	/* 950 */	"&zeta;",
	/* 951 */	"&eta;",
	/* 952 */	"&theta;",
	/* 953 */	"&iota;",
	/* 954 */	"&kappa;",
	/* 955 */	"&lambda;",
	/* 956 */	"&mu;",
	/* 957 */	"&nu;",
	/* 958 */	"&xi;",
	/* 959 */	"&omicron;",
	/* 960 */	"&pi;",
	/* 961 */	"&rho;",
	/* 962 */	"&sigmaf;",
	/* 963 */	"&sigma;",
	/* 964 */	"&tau;",
	/* 965 */	"&upsilon;",
	/* 966 */	"&phi;",
	/* 967 */	"&chi;",
	/* 968 */	"&psi;",
	/* 969 */	"&omega;",
	/* 970 */	0,
	/* 971 */	0,
	/* 972 */	0,
	/* 973 */	0,
	/* 974 */	0,
	/* 975 */	0,
	/* 976 */	0,
	/* 977 */	"&thetasym;",
	/* 978 */	"&upsih;",
	/* 979 */	0,
	/* 980 */	0,
	/* 981 */	0,
	/* 982 */	"&piv;",
};
#endif
#if 1 /* daved - 0.19.4 - unicode symbol character support */
static const char * unisymbol2[] = {
/* 57516 */	"&Gamma;",
/* 57517 */	"&Delta;",
/* 57518 */	"&Theta;",
/* 57519 */	"&Lambda;",
/* 57520 */	"&Xi;",
/* 57521 */	"&Pi;",
/* 57522 */	"&Sigma;",
/* 57523 */	"&Upsilon;",
/* 57524 */	"&Phi;",
/* 57525 */	"&Psi;",
/* 57526 */	"&Omega;",
/* 57527 */	"&alpha;",
/* 57528 */	"&beta;",
/* 57529 */	"&gamma;",
/* 57530 */	"&delta;",
/* 57531 */	"&epsilon;",
/* 57532 */	"&zeta;",
/* 57533 */	"&eta;",
/* 57534 */	"&theta;",
/* 57535 */	"&iota;",
/* 57536 */	"&kappa;",
/* 57537 */	"&lambda;",
/* 57538 */	"&mu;",
/* 57539 */	"&nu;",
/* 57540 */	"&xi;",
/* 57541 */	"&omicron;",
/* 57542 */	"&pi;",
/* 57543 */	"&rho;",
/* 57544 */	"&sigma;",
/* 57545 */	"&tau;",
/* 57546 */	"&upsilon;",
/* 57547 */	"&phi;",
/* 57548 */	"&chi;",
/* 57549 */	"&psi;",
/* 57550 */	"&omega;",
/* 57551 */	"&epsiv;",
/* 57552 */	"&thetav;",
/* 57553 */	"&piv;",
/* 57554 */	0,
/* 57555 */	"&sigmaf;",
/* 57556 */	"&phiv;",
/* 57557 */	"&delta;",
};

static const char * unisymbol3[] = {
 /* 61505 */   "&Alpha;",
 /* 61506 */   "&Beta;",
 /* 61507 */   "&Chi;",
 /* 61508 */   "&Delta;",
 /* 61509 */   "&Epsilon;",
 /* 61510 */   "&Phi;",
 /* 61511 */   "&Gamma;",
 /* 61512 */   "&Eta;",
 /* 61513 */   "&Iota;",
 /* 61514 */   "&phiv;",
 /* 61515 */   "&Kappa;",
 /* 61516 */   "&Lambda;",
 /* 61517 */   "&Mu;",
 /* 61518 */   "&Nu;",
 /* 61519 */   "&Omicron;",
 /* 61520 */   "&Pi;",
 /* 61521 */   "&Theta;",
 /* 61522 */   "&Rho;",
 /* 61523 */   "&Sigma;",
 /* 61524 */   "&Tau;",
 /* 61525 */   "&Upsilon;",
 /* 61526 */   "&sigmaf;",
 /* 61527 */   "&Omega;",
 /* 61528 */   "&Xi;",
 /* 61529 */   "&Psi;",
 /* 61530 */   "&Zeta;",
 /* 61531 */   0,
 /* 61532 */   0,
 /* 61533 */   0,
 /* 61534 */   0,
 /* 61535 */   0,
 /* 61536 */   0,
 /* 61537 */   "&alpha;",
 /* 61538 */   "&beta;",
 /* 61539 */   "&chi;",
 /* 61540 */   "&delta;",
 /* 61541 */   "&epsilon;",
 /* 61542 */   "&phi;",
 /* 61543 */   "&gamma;",
 /* 61544 */   "&eta;",
 /* 61545 */   "&tau;",
 /* 61546 */   "&phiv;",
 /* 61547 */   "&kappa;",
 /* 61548 */   "&lambda;",
 /* 61549 */   "&mu;",
 /* 61550 */   "&nu;",
 /* 61551 */   "&omicron;",
 /* 61552 */   "&pi;",
 /* 61553 */   "&theta;",
 /* 61554 */   "&rho;",
 /* 61555 */   "&sigma;",
 /* 61556 */   "&tau;",
 /* 61557 */   "&upsilon;",
 /* 61558 */   "&piv;",
 /* 61559 */   "&omega;",
 /* 61560 */   "&xi;",
 /* 61561 */   "&psi;",
 /* 61562 */   "&zeta;",
};
#endif

#if 1 /* 0.19.5 more unicode characters */
static const char * unisymbol4[] = {
 /* 61600 */    "&euro;",
 /* 61601 */	"&upsih;",
 /* 61602 */	"&prime;",
 /* 61603 */	"&le;",
 /* 61604 */	"&frasl;",
 /* 61605 */	"&infin;",
 /* 61606 */	"&fnof;",
 /* 61607 */	"&clubs;",
 /* 61608 */	"&diams;",
 /* 61609 */	"&hearts;",
 /* 61610 */	"&spades;",
 /* 61611 */	"&harr;",
 /* 61612 */	"&larr",
 /* 61613 */	"&uarr;",
 /* 61614 */	"&rarr;",
 /* 61615 */	"&darr;",
 /* 61616 */	"&deg;",
 /* 61617 */	"&plusmn;",
 /* 61618 */	"&Prime;",
 /* 61619 */	"&ge;",
 /* 61620 */	"&times;",
 /* 61621 */	"&prop;",
 /* 61622 */	"&part;",
 /* 61623 */	"&bull;",
 /* 61624 */	"&divide;",
 /* 61625 */	"&ne;",
 /* 61626 */	"&equiv;",
 /* 61627 */	"&asymp;",
 /* 61628 */	"&hellip;",
 /* 61629 */	0,		/* vertical bar */
 /* 61630 */	"&mdash;",
 /* 61631 */	"&crarr;",
 /* 61632 */	"&alefsym;",
 /* 61633 */	"&image;",
 /* 61634 */	"&real;",
 /* 61635 */	"&weierp;",
 /* 61636 */	"&otimes;",
 /* 61637 */	"&oplus;",
 /* 61638 */	"&empty;",
 /* 61639 */	"&cap;",
 /* 61640 */	"&cup;",
 /* 61641 */	"&sup;",
 /* 61642 */	"&supe;",
 /* 61643 */	"&nsub;",
 /* 61644 */	"&sub;",
 /* 61645 */	"&sube;",
 /* 61646 */	"&isin;",
 /* 61647 */	"&notin;",
 /* 61648 */	"&ang;",
 /* 61649 */	"&nabla;",
 /* 61650 */	"&reg;",
 /* 61651 */	"&copy;",
 /* 61652 */	"&trade;",
 /* 61653 */	"&prod;",
 /* 61654 */	"&radic;",
 /* 61655 */	"&middot;",
 /* 61656 */	"&not;",
 /* 61657 */	"&and;",
 /* 61658 */	"&or;",
 /* 61659 */	"&hArr;",
 /* 61660 */	"&lArr;",
 /* 61661 */	"&uArr;",
 /* 61662 */	"&rArr;",
 /* 61663 */	"&dArr;",
 /* 61664 */	"&loz;",
 /* 61665 */	"&lang;",
 /* 61666 */	"&reg;",
 /* 61667 */	"&copy;",
 /* 61668 */	"&trade;",
 /* 61669 */	"&sum;",
 /* 61670 */	0,		/* large right parenthesis ceiling */
 /* 61671 */	0,		/* large parenthesis middle */
 /* 61672 */	0,		/* large left parenthesis floor */
 /* 61673 */	"&lceil;",	/* large left square bracket ceiling */
 /* 61674 */	0,		/* large left square bracket middle */
 /* 61675 */	"&lfloor;",	/* large left square bracket floor */
 /* 61676 */	0,		/* large left bracket ceiling */
 /* 61677 */	0,		/* large left bracket middle */
 /* 61678 */	0,		/* large left bracket floor */
 /* 61679 */	0,		/* large vertical bar */
 /* 61680 */	0,		/* appears blank */
 /* 61681 */	"&rang;",
 /* 61682 */	"&int;",	/* integral */
 /* 61683 */	0,		/* large integral ceiling */
 /* 61684 */	0,		/* large integral middle */
 /* 61685 */	0,		/* large integral floor */
 /* 61686 */	0,		/* large right parenthesis ceiling */
 /* 61687 */	0,		/* large right parenthesis middle */
 /* 61688 */	0,		/* large right parenthesis floor */
 /* 61689 */	"&rceil;",	/* large right square bracket ceiling */
 /* 61690 */	0,		/* large right square bracket middle */
 /* 61691 */	"&rfloor;",	/* large right square bracket floor */
 /* 61692 */	0,		/* large right bracket middle */
 /* 61694 */	0		/* large right bracket floot */
};
#endif
#if 1 /* daved - SYMBOL font characters */
static const char* symbol[] = {
/*  60 */	"&lt;",
/*  61 */	"=",
/*  62 */	"&gt;",
/*  63 */	"?",
/*  64 */	"&cong;",
/*  65 */	"&Alpha;",
/*  66 */	"&Beta;",
/*  67 */	"&Beta;",
/*  68 */	"&Delta;",
/*  69 */	"&Epsilon;",
/*  70 */	"&Phi;",
/*  71 */	"&Gamma;",
/*  72 */	"&Eta;",
/*  73 */	"&Iota;",
/*  74 */	"&thetasym;",
/*  75 */	"&Kappa;",
/*  76 */	"&Lambda;",
/*  77 */	"&Mu;",
/*  78 */	"&Nu;",
/*  79 */	"&Omicron;",
/*  80 */	"&Pi;",
/*  81 */	"&Theta;",
/*  82 */	"&Rho;",
/*  83 */	"&Sigma;",
/*  84 */	"&Tau;",
/*  85 */	"&Upsilon;",
/*  86 */	"&sigmaf;",
/*  87 */	"&Omega;",
/*  88 */	"&Xi;",
/*  89 */	"&Psi;",
/*  90 */	"&Zeta;",
/*  91 */	"[",
/*  92 */	"&there4;",
/*  93 */	"]",
/*  94 */	"&perp;",
/*  95 */	"_",
/*  96 */	"&oline;",
/*  97 */	"&alpha;",
/*  98 */	"&beta;",
/*  99 */	"&chi;",
/*  100 */	"&delta;",
/*  101 */	"&epsilon;",
/*  102 */	"&phi;",
/*  103 */	"&gamma;",
/*  104 */	"&eta;",
/*  105 */	"&iota;",
/*  106 */	"",		/* ? */
/*  107 */	"&kappa;",
/*  108 */	"&lambda;",
/*  109 */	"&mu;",
/*  110 */	"&nu;",
/*  111 */	"&omicron;",
/*  112 */	"&pi;",
/*  113 */	"&theta;",
/*  114 */	"&rho;",
/*  115 */	"&sigma;",
/*  116 */	"&tau;",
/*  117 */	"&upsilon;",
/*  118 */	"&piv;",
/*  119 */	"&omega;",
/*  120 */	"&xi;",
/*  121 */	"&psi;",
/*  122 */	"&zeta;",
/*  123 */	"{",
/*  124 */	"|",
/*  125 */	"}",
/*  126 */	"&sim;",
/*  127 */	0,
/*  128 */	0,
/*  129 */	0,
/*  130 */	0,
/*  131 */	0,
/*  132 */	0,
/*  133 */	0,
/*  134 */	0,
/*  135 */	0,
/*  136 */	0,
/*  137 */	0,
/*  138 */	0,
/*  139 */	0,
/*  140 */	0,
/*  141 */	0,
/*  142 */	0,
/*  143 */	0,
/*  144 */	0,
/*  145 */	0,
/*  146 */	0,
/*  147 */	0,
/*  148 */	0,
/*  149 */	0,
/*  150 */	0,
/*  151 */	0,
/*  152 */	0,
/*  153 */	0,
/*  154 */	0,
/*  155 */	0,
/*  156 */	0,
/*  157 */	0,
/*  158 */	0,
/*  159 */	0,
/*  160 */	0,
/*  161 */	"&upsih;",
/*  162 */	"&prime;",
/*  163 */	"&le;",
/*  164 */	"&frasl;",
/*  165 */	"&infin;",
/*  166 */	"&fnof;",
/*  167 */	"&clubs;",
/*  168 */	"&diams;",
/*  169 */	"&hearts;",
/*  170 */	"&spades;",
/*  171 */	"&harr;",
/*  172 */	"&larr;",
/*  173 */	0,
/*  174 */	"&rarr;",
/*  175 */	"&darr;",
/*  176 */	"&deg;",
/*  177 */	"&plusmn;",
/*  178 */	"&Prime;",
/*  179 */	"&ge;",
/*  180 */	"&times;",
/*  181 */	"&prop;",
/*  182 */	"&part;",
/*  183 */	"&bull;",
/*  184 */	"&divide;",
/*  185 */	"&ne;",
/*  186 */	"&equiv;",
/*  187 */	"&asymp;",
/*  188 */	"&hellip;",
/*  189 */	"&#9474;",		/* vertical line */
/*  190 */	"&mdash;",
/*  191 */	"&crarr;",
/*  192 */	"&alefsym;",
/*  193 */	"&image;",
/*  194 */	"&real;",
/*  195 */	"&weierp;",
/*  196 */	"&otimes;",
/*  197 */	"&oplus;",
/*  198 */	"&empty;",
/*  199 */	"&cap;",
/*  200 */	"&cup;",
/*  201 */	"&sup;",
/*  202 */	"&supe;",
/*  203 */	"&nsub;",
/*  204 */	"&sub;",
/*  205 */	"&sube;",
/*  206 */	"&isin;",
/*  207 */	"&notin;",
/*  208 */	"&ang;",
/*  209 */	"&nabla;",
/*  210 */	"&reg;",	/* serif */
/*  211 */	"&copy;",	/* serif */
/*  212 */	"&trade;",	/* serif */
/*  213 */	"&prod;",
/*  214 */	"&radic;",
/*  215 */	"&middot;",
/*  216 */	"&not;",
/*  217 */	"&and;",
/*  218 */	"&or;",
/*  219 */	"&hArr;",
/*  220 */	"&lArr;",
/*  221 */	"&uArr;",
/*  222 */	"&rArr;",
/*  223 */	"&dArr;",
/*  224 */	"&loz;",
/*  225 */	"&lang;",
/*  226 */	"&reg;",	/* sans serif */
/*  227 */	"&copy;",	/* sans serif */
/*  228 */	"&trade;",	/* sans serif */
/*  229 */	"&sum;",
/*  230 */	0,
/*  231 */	0,
/*  232 */	0,
/*  233 */	"&lceil;",
/*  234 */	"|",
/*  235 */	"&lfloor;",
/*  236 */	0,
/*  237 */	0,
/*  238 */	0,
/*  239 */	"|",
/*  240 */	"&eth;",
/*  241 */	"&rang;",
/*  242 */	"&int;",
/*  243 */	0,
/*  244 */	0,
/*  245 */	0,
/*  246 */	0,
/*  247 */	0,
/*  248 */	0,
/*  249 */	"&rceil;",
/*  250 */	"|",
/*  251 */	"&rfloor;",
/*  252 */	0,
/*  253 */	0,
/*  254 */	0,
};
#endif 
static const char* cp437 [] = {
/* 0x80 */ "&ccedil;",
/* 0x81 */ "&uuml;",
/* 0x82 */ "&eacute;",
/* 0x83 */ "&acirc;",
/* 0x84 */ "&auml;",
/* 0x85 */ "&agrave;",
/* 0x86 */ "&aring;",
/* 0x87 */ "&ccedil;",
/* 0x88 */ "&ecirc;",
/* 0x89 */ "&euml;",
/* 0x8a */ "&egrave;",
/* 0x8b */ "&iuml;",
/* 0x8c */ "&icirc;",
/* 0x8d */ "&igrave;",
/* 0x8e */ "&auml;",
/* 0x8f */ "&aring;",
/* 0x90 */ "&eacute;",
/* 0x91 */ "&aelig;",
/* 0x92 */ "&aelig;",
/* 0x93 */ "&ocirc;",
/* 0x94 */ "&ouml;",
/* 0x95 */ "&ograve;",
/* 0x96 */ "&ucirc;",
/* 0x97 */ "&ugrave;",
/* 0x98 */ "&yuml;",
/* 0x99 */ "&ouml;",
/* 0x9a */ "&uuml;",
/* 0x9b */ "&cent;",
/* 0x9c */ "&pound;",
/* 0x9d */ "&yen;",
/* 0x9e */ "&#8359;", /* peseta */
/* 0x9f */ "&#402;", /* small f with hook */
/* 0xa0 */ "&aacute;",
/* 0xa1 */ "&iacute;",
/* 0xa2 */ "&oacute;",
/* 0xa3 */ "&uacute;",
/* 0xa4 */ "&ntilde;",
/* 0xa5 */ "&ntilde;",
/* 0xa6 */ "&ordf;",
/* 0xa7 */ "&frac14;",
/* 0xa8 */ "&iquest;",
/* 0xa9 */ "&#8976;", /* reversed not */
/* 0xaa */ "&not;",
/* 0xab */ "&frac12;",
/* 0xac */ "&raquo;",
/* 0xad */ "&iexcl;",
/* 0xae */ "&laquo;",
/* 0xaf */ "&ordm;",
/* 0xb0 */ "&#9617;", /* light shade */
/* 0xb1 */ "&#9618;", /* med. shade */
/* 0xb2 */ "&#9619;", /* dark shade */
/* 0xb3 */ "&#9474;", /* box-draw light vert. */
/* 0xb4 */ "&#9508;", /* box-draw light vert. + lt. */
/* 0xb5 */ "&#9569;", /* box-draw vert. sgl. + lt. dbl. */
/* 0xb6 */ "&#9570;", /* box-draw vert. dbl. + lt. sgl. */
/* 0xb7 */ "&#9558;", /* box-draw dn. dbl. + lt. sgl. */
/* 0xb8 */ "&#9557;", /* box-draw dn. sgl. + lt. dbl. */
/* 0xb9 */ "&#9571;", /* box-draw dbl. vert. + lt. */
/* 0xba */ "&#9553;", /* box-draw dbl. vert. */
/* 0xbb */ "&#9559;", /* box-draw dbl. dn. + lt. */
/* 0xbc */ "&#9565;", /* box-draw dbl. up + lt. */
/* 0xbd */ "&#9564;", /* box-draw up dbl. + lt. sgl. */
/* 0xbe */ "&#9563;", /* box-draw up sgl. + lt. dbl. */
/* 0xbf */ "&#9488;", /* box-draw light dn. + lt. */
/* 0xc0 */ "&#9492;", /* box-draw light up + rt. */
/* 0xc1 */ "&#9524;", /* box-draw light up + horiz. */
/* 0xc2 */ "&#9516;", /* box-draw light dn. + horiz. */
/* 0xc3 */ "&#9500;", /* box-draw light vert. + rt. */
/* 0xc4 */ "&#9472;", /* box-draw light horiz. */
/* 0xc5 */ "&#9532;", /* box-draw light vert. + horiz. */
/* 0xc6 */ "&#9566;", /* box-draw vert. sgl. + rt. dbl. */
/* 0xc7 */ "&#9567;", /* box-draw vert. dbl. + rt. sgl. */
/* 0xc8 */ "&#9562;", /* box-draw dbl. up + rt. */
/* 0xc9 */ "&#9556;", /* box-draw dbl. dn. + rt. */
/* 0xca */ "&#9577;", /* box-draw dbl. up + horiz. */
/* 0xcb */ "&#9574;", /* box-draw dbl. dn. + horiz. */
/* 0xcc */ "&#9568;", /* box-draw dbl. vert. + rt. */
/* 0xcd */ "&#9552;", /* box-draw dbl. horiz. */
/* 0xce */ "&#9580;", /* box-draw dbl. vert. + horiz. */
/* 0xcf */ "&#9575;", /* box-draw up sgl. + horiz. dbl. */
/* 0xd0 */ "&#9576;", /* box-draw up dbl. + horiz. sgl. */
/* 0xd1 */ "&#9572;", /* box-draw dn. sgl. + horiz. dbl. */
/* 0xd2 */ "&#9573;", /* box-draw dn. dbl. + horiz. sgl. */
/* 0xd3 */ "&#9561;", /* box-draw up dbl. + rt. sgl. */
/* 0xd4 */ "&#9560;", /* box-draw up sgl. + rt. dbl. */
/* 0xd5 */ "&#9554;", /* box-draw dn. sgl. + rt. dbl. */
/* 0xd6 */ "&#9555;", /* box-draw dn. dbl. + rt. sgl. */
/* 0xd7 */ "&#9579;", /* box-draw vert. dbl. + horiz. sgl. */
/* 0xd8 */ "&#9578;", /* box-draw vert. sgl. + horiz. dbl. */
/* 0xd9 */ "&#9496;", /* box-draw light up + lt. */
/* 0xda */ "&#9484;", /* box-draw light dn. + rt. */
/* 0xdb */ "&#9608;", /* full block */
/* 0xdc */ "&#9604;", /* lower 1/2 block */
/* 0xdd */ "&#9612;", /* lt. 1/2 block */
/* 0xde */ "&#9616;", /* rt. 1/2 block */
/* 0xdf */ "&#9600;", /* upper 1/2 block */
/* 0xe0 */ "&#945;", /* greek small alpha */
/* 0xe1 */ "&szlig;",
/* 0xe2 */ "&#915;", /* greek cap gamma */
/* 0xe3 */ "&#960;", /* greek small pi */
/* 0xe4 */ "&#931;", /* greek cap sigma */
/* 0xe5 */ "&#963;", /* greek small sigma */
/* 0xe6 */ "&micro;",
/* 0xe7 */ "&#964;", /* greek small tau */
/* 0xe8 */ "&#934;", /* greek cap phi */
/* 0xe9 */ "&#920;", /* greek cap theta */
/* 0xea */ "&#937;", /* greek cap omega */
/* 0xeb */ "&#948;", /* greek small delta */
/* 0xec */ "&#8734;", /* inf. */
/* 0xed */ "&#966;", /* greek small phi */
/* 0xee */ "&#949;", /* greek small epsilon */
/* 0xef */ "&#8745;", /* intersect */
/* 0xf0 */ "&#8801;", /* identical */
/* 0xf1 */ "&plusmn;",
/* 0xf2 */ "&#8805;", /* greater-than or equal to */
/* 0xf3 */ "&#8804;", /* less-than or equal to */
/* 0xf4 */ "&#8992;", /* top 1/2 integral */
/* 0xf5 */ "&#8993;", /* bottom 1/2 integral */
/* 0xf6 */ "&divide;",
/* 0xf7 */ "&#8776;", /* almost = */
/* 0xf8 */ "&plus;",
/* 0xf9 */ "&#8729;", /* bullet op */
/* 0xfa */ "&middot;",
/* 0xfb */ "&#8730;", /* sqrt */
/* 0xfc */ "&#8319;", /* super-script small n */
/* 0xfd */ "&sup2;",
/* 0xfe */ "&#9632;", /* black square */
/* 0xff */ "&nbsp;",
};

static const char* cp850 [] = {
/* 0x80 */  "&ccedil;",
/* 0x81 */  "&uuml;",
/* 0x82 */  "&eacute;",
/* 0x83 */  "&acirc;",
/* 0x84 */  "&auml;",
/* 0x85 */  "&agrave;",
/* 0x86 */  "&aring;",
/* 0x87 */  "&ccedil;",
/* 0x88 */  "&ecirc;",
/* 0x89 */  "&euml;",
/* 0x8a */  "&egrave;",
/* 0x8b */  "&iuml;",
/* 0x8c */  "&icirc;",
/* 0x8d */  "&igrave;",
/* 0x8e */  "&auml;",
/* 0x8f */  "&aring;",
/* 0x90 */  "&eacute;",
/* 0x91 */  "&aelig;",
/* 0x92 */  "&aelig;",
/* 0x93 */  "&ocirc;",
/* 0x94 */  "&ouml;",
/* 0x95 */  "&ograve;",
/* 0x96 */  "&ucirc;",
/* 0x97 */  "&ugrave;",
/* 0x98 */  "&yuml;",
/* 0x99 */  "&ouml;",
/* 0x9a */  "&uuml;",
/* 0x9b */  "&oslash;",
/* 0x9c */  "&pound;",
/* 0x9d */  "&oslash;",
/* 0x9e */  "&times;",
/* 0x9f */  "&#402;", /* small f with hook */
/* 0xa0 */  "&aacute;",
/* 0xa1 */  "&iacute;",
/* 0xa2 */  "&oacute;",
/* 0xa3 */  "&uacute;",
/* 0xa4 */  "&ntilde;",
/* 0xa5 */  "&ntilde;",
/* 0xa6 */  "&ordf;",
/* 0xa7 */  "&frac14;",
/* 0xa8 */  "&iquest;",
/* 0xa9 */  "&reg;",
/* 0xaa */  "&not;",
/* 0xab */  "&frac12;",
/* 0xac */  "&raquo;",
/* 0xad */  "&iexcl;",
/* 0xae */  "&laquo;",
/* 0xaf */  "&ordm;",
/* 0xb0 */  "&#9617;", /* light shade */
/* 0xb1 */  "&#9618;", /* med. shade */
/* 0xb2 */  "&#9619;", /* dark shade */
/* 0xb3 */  "&#9474;", /* box-draw light vert. */
/* 0xb4 */  "&#9508;", /* box-draw light vert. + lt. */
/* 0xb5 */  "&aacute;",
/* 0xb6 */  "&acirc;",
/* 0xb7 */  "&agrave;",
/* 0xb8 */  "&copy;",
/* 0xb9 */  "&#9571;", /* box-draw dbl. vert. + lt. */
/* 0xba */  "&#9553;", /* box-draw dbl. vert. */
/* 0xbb */  "&#9559;", /* box-draw dbl. dn. + lt. */
/* 0xbc */  "&#9565;", /* box-draw dbl. up + lt. */
/* 0xbd */  "&cent;",
/* 0xbe */  "&yen;",
/* 0xbf */  "&#9488;", /* box-draw light dn. + lt. */
/* 0xc0 */  "&#9492;", /* box-draw light up + rt. */
/* 0xc1 */  "&#9524;", /* box-draw light up + horiz. */
/* 0xc2 */  "&#9516;", /* box-draw light dn. + horiz. */
/* 0xc3 */  "&#9500;", /* box-draw light vert. + rt. */
/* 0xc4 */  "&#9472;", /* box-draw light horiz. */
/* 0xc5 */  "&#9532;", /* box-draw light vert. + horiz. */
/* 0xc6 */  "&atilde;",
/* 0xc7 */  "&atilde;",
/* 0xc8 */  "&#9562;", /* box-draw dbl. up + rt. */
/* 0xc9 */  "&#9556;", /* box-draw dbl. dn. + rt. */
/* 0xca */  "&#9577;", /* box-draw dbl. up + horiz. */
/* 0xcb */  "&#9574;", /* box-draw dbl. dn. + horiz. */
/* 0xcc */  "&#9568;", /* box-draw dbl. vert. + rt. */
/* 0xcd */  "&#9552;", /* box-draw dbl. horiz. */
/* 0xce */  "&#9580;", /* box-draw dbl. vert. + horiz. */
/* 0xcf */  "&curren;",
/* 0xd0 */  "&eth;",
/* 0xd1 */  "&eth;",
/* 0xd2 */  "&ecirc;",
/* 0xd3 */  "&euml;",
/* 0xd4 */  "&egrave;",
/* 0xd5 */  "&#305;", /* small dotless i */
/* 0xd6 */  "&iacute;",
/* 0xd7 */  "&icirc;",
/* 0xd8 */  "&iuml;",
/* 0xd9 */  "&#9496;", /* box-draw light up + lt. */
/* 0xda */  "&#9484;", /* box-draw light dn. + rt. */
/* 0xdb */  "&#9608;", /* full-block */
/* 0xdc */  "&#9604;", /* lower 1/2 block */
/* 0xdd */  "&brvbar;",
/* 0xde */  "&igrave;",
/* 0xdf */  "&#9600;", /* upper 1/2 block */
/* 0xe0 */  "&oacute;",
/* 0xe1 */  "&szlig;",
/* 0xe2 */  "&ocirc;",
/* 0xe3 */  "&ograve;",
/* 0xe4 */  "&otilde;",
/* 0xe5 */  "&otilde;",
/* 0xe6 */  "&micro;",
/* 0xe7 */  "&thorn;",
/* 0xe8 */  "&thorn;",
/* 0xe9 */  "&uacute;",
/* 0xea */  "&ucirc;",
/* 0xeb */  "&ugrave;",
/* 0xec */  "&yacute;",
/* 0xed */  "&yacute;",
/* 0xee */  "&macr;",
/* 0xef */  "&acute;",
/* 0xf0 */  "&shy;",
/* 0xf1 */  "&plusmn;",
/* 0xf2 */  "&#8215;", /* dbl. lowline */
/* 0xf3 */  "&frac34;",
/* 0xf4 */  "&para;",
/* 0xf5 */  "&sect;",
/* 0xf6 */  "&divide;",
/* 0xf7 */  "&cedil;",
/* 0xf8 */  "&plus;",
/* 0xf9 */  "&uml;",
/* 0xfa */  "&middot;",
/* 0xfb */  "&sup1;",
/* 0xfc */  "&sup3;",
/* 0xfd */  "&sup2;",
/* 0xfe */  "&#9632;", /* black square */
/* 0xff */  "&nbsp;",
};
#if 1 /* daved - 0.20.3 */
static const char * Greek[] =
{
/* 0x80 */  "&ccedil;",
/* 0x81 */  "&uuml;",
/* 0x82 */  "&eacute;",
/* 0x83 */  "&acirc;",
/* 0x84 */  "&auml;",
/* 0x85 */  "&agrave;",
/* 0x86 */  "&aring;",
/* 0x87 */  "&ccedil;",
/* 0x88 */  "&ecirc;",
/* 0x89 */  "&euml;",
/* 0x8a */  "&egrave;",
/* 0x8b */  "&iuml;",
/* 0x8c */  "&icirc;",
/* 0x8d */  "&igrave;",
/* 0x8e */  "&auml;",
/* 0x8f */  "&aring;",
/* 0x90 */  "&eacute;",
/* 0x91 */  "&aelig;",
/* 0x92 */  "&aelig;",
/* 0x93 */  "&ocirc;",
/* 0x94 */  "&ouml;",
/* 0x95 */  "&ograve;",
/* 0x96 */  "&ucirc;",
/* 0x97 */  "&ugrave;",
/* 0x98 */  "&yuml;",
/* 0x99 */  "&ouml;",
/* 0x9a */  "&uuml;",
/* 0x9b */  "&oslash;",
/* 0x9c */  "&pound;",
/* 0x9d */  "&oslash;",
/* 0x9e */  "&times;",
/* 0x9f */  "&#402;", /* small f with hook */
/* 0xa0 */  "&aacute;",
/* 0xa1 */  "&iacute;",
/* 0xa2 */  "&oacute;",
/* 0xa3 */  "&uacute;",
/* 0xa4 */  "&ntilde;",
/* 0xa5 */  "&ntilde;",
/* 0xa6 */  "&ordf;",
/* 0xa7 */  "&frac14;",
/* 0xa8 */  "&iquest;",
/* 0xa9 */  "&reg;",
/* 0xaa */  "&not;",
/* 0xab */  "&frac12;",
/* 0xac */  "&raquo;",
/* 0xad */  "&iexcl;",
/* 0xae */  "&laquo;",
/* 0xaf */  "&ordm;",
/* 0xb0 */  "&#9617;", /* light shade */
/* 0xb1 */  "&#9618;", /* med. shade */
/* 0xb2 */  "&#9619;", /* dark shade */
/* 0xb3 */  "&#9474;", /* box-draw light vert. */
/* 0xb4 */  "&#9508;", /* box-draw light vert. + lt. */
/* 0xb5 */  "&aacute;",
/* 0xb6 */  "&acirc;",
/* 0xb7 */  "&agrave;",
/* 0xb8 */  "&copy;",
/* 0xb9 */  "&#9571;", /* box-draw dbl. vert. + lt. */
/* 0xba */  "&#9553;", /* box-draw dbl. vert. */
/* 0xbb */  "&#9559;", /* box-draw dbl. dn. + lt. */
/* 0xbc */  "&#9565;", /* box-draw dbl. up + lt. */
/* 0xbd */  "&cent;",
/* 0xbe */  "&yen;",
/* 0xbf */  "&#9488;", /* box-draw light dn. + lt. */
/* 0xc0 */  "&#9492;", /* box-draw light up + rt. */
/* 0xc1 */  "&#9524;", /* box-draw light up + horiz. */
/* 0xc2 */  "&#9516;", /* box-draw light dn. + horiz. */
/* 0xc3 */  "&#9500;", /* box-draw light vert. + rt. */
/* 0xc4 */  "&#9472;", /* box-draw light horiz. */
/* 0xc5 */  "&#9532;", /* box-draw light vert. + horiz. */
/* 0xc6 */  "&atilde;",
/* 0xc7 */  "&atilde;",
/* 0xc8 */  "&#9562;", /* box-draw dbl. up + rt. */
/* 0xc9 */  "&#9556;", /* box-draw dbl. dn. + rt. */
/* 0xca */  "&#9577;", /* box-draw dbl. up + horiz. */
/* 0xcb */  "&#9574;", /* box-draw dbl. dn. + horiz. */
/* 0xcc */  "&#9568;", /* box-draw dbl. vert. + rt. */
/* 0xcd */  "&#9552;", /* box-draw dbl. horiz. */
/* 0xce */  "&#9580;", /* box-draw dbl. vert. + horiz. */
/* 0xcf */  "&curren;",
/* 0xd0 */  "&eth;",
/* 0xd1 */  "&eth;",
/* 0xd2 */  "&ecirc;",
/* 0xd3 */  "&euml;",
/* 0xd4 */  "&egrave;",
/* 0xd5 */  "&#305;", /* small dotless i */
/* 0xd6 */  "&iacute;",
/* 0xd7 */  "&icirc;",
/* 0xd8 */  "&iuml;",
/* 0xd9 */  "&#9496;", /* box-draw light up + lt. */
/* 0xda */  "&#9484;", /* box-draw light dn. + rt. */
/* 0xdb */  "&#9608;", /* full-block */
/* 0xdc */  "&#9604;", /* lower 1/2 block */
/* 0xdd */  "&brvbar;",
/* 0xde */  "&igrave;",
/* 0xdf */  "&#9600;", /* upper 1/2 block */
/* 0xe0 */  "&oacute;",
/* above here not done */
/* 0xe1 */  "&alpha;",
/* 0xe2 */  "&beta;",
/* 0xe3 */  "&gamma;",
/* 0xe4 */  "&delta;",
/* 0xe5 */  "&epsilon;",
/* 0xe6 */  "&zeta;",
/* 0xe7 */  "&eta;",
/* 0xe8 */  "&theta;",
/* 0xe9 */  "&iota;",
/* 0xea */  "&kappa;",
/* 0xeb */  "&lambda;",
/* 0xec */  "&mu;",
/* 0xed */  "&nu;",
/* 0xee */  "&xi;",
/* 0xef */  "&omicron;",
/* 0xf0 */  "&pi;",
/* 0xf1 */  "&rho;",
/* 0xf2 */  "&sigmaf;",
/* 0xf3 */  "&sigma;",
/* 0xf4 */  "&tau;",
/* 0xf5 */  "&upsilon;",
/* 0xf6 */  "&phi;",
/* 0xf7 */  "&chi;",
/* 0xf8 */  "&psi;",
/* 0xf9 */  "&omiga;",
/* 0xfa */  "&iotauml;",
/* 0xfb */  "&nuuml;",
/* 0xfc */  "&omicronacute;",
/* 0xfd */  "&nuacute;",
/* 0xfe */  "&omegaacute;", /* black square */
/* 0xff */  "&nbsp;",
};
#endif





/*========================================================================
 * Name:	html_unisymbol_print
 * Purpose:	Outputs arbitrary unicode symbol
 * Args:	Unsigned Short.
 * Returns:	String representing symbol.
 *=======================================================================*/

char *
html_unisymbol_print (unsigned short c)
{
	char r[12];
	snprintf(r, 9, "&#%04d;", c);
	return my_strdup(r);
}




/*========================================================================
 * Name:	html_init
 * Purpose:	Generates the HTML output personality.
 * Args:	None.
 * Returns:	OutputPersonality.
 *=======================================================================*/

OutputPersonality *
html_init (void) 
{
	OutputPersonality* op;

	op = op_create();

	op->comment_begin = "<!-- ";
	op->comment_end = " -->\n";

	op->document_begin = "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n<html>\n";
	op->document_end = "</html>\n";

	op->header_begin = "<head>\n";
	op->header_end = "</head>\n";

	op->document_title_begin = "<title>";
	op->document_title_end = "</title>\n";

	op->document_author_begin = "<!-- author: ";
	op->document_author_end = "-->\n";

	op->document_changedate_begin = "<!-- changed: ";
	op->document_changedate_end = "-->\n";

	op->body_begin = "<body>";
	op->body_end = "</body>\n";

	op->paragraph_begin = "<p>";
	op->paragraph_end = "</p>\n";

	op->center_begin = "<center>";
	op->center_end = "</center>\n";

	op->justify_begin = "<div align=justify>\n"; 
	op->justify_end = "</div>\n";

	op->align_left_begin = "<div align=left>\n"; 
	op->align_left_end = "</div>\n";

	op->align_right_begin = "<div align=right>\n"; 
	op->align_right_end = "</div>\n";

	op->forced_space = "&nbsp;";
	op->line_break = "<br>\n";
	op->page_break = "<p><hr><p>\n";

	op->hyperlink_begin = "<a href=\"";
	op->hyperlink_end = "\">hyperlink</a>";

	op->imagelink_begin = "<img src=\"";
	op->imagelink_end = "\">";

	op->table_begin = "<table border=2>\n";
	op->table_end = "</table>\n";

	op->table_row_begin = "<tr>";
	op->table_row_end = "</tr>\n";

	op->table_cell_begin = "<td>";
	op->table_cell_end = "</td>\n";

	/* Character attributes */
	op->font_begin = "<font face=\"%s\">";
	op->font_end = "</font>";

	op->fontsize_begin = "<span style=\"font-size:%spt\">";
	op->fontsize_end = "</span>";

	op->fontsize8_begin = "<font size=1>";
	op->fontsize8_end = "</font>";
	op->fontsize10_begin = "<font size=2>";
	op->fontsize10_end = "</font>";
	op->fontsize12_begin = "<font size=3>";
	op->fontsize12_end = "</font>";
	op->fontsize14_begin = "<font size=4>";
	op->fontsize14_end = "</font>";
	op->fontsize18_begin = "<font size=5>";
	op->fontsize18_end = "</font>";
	op->fontsize24_begin = "<font size=6>";
	op->fontsize24_end = "</font>";

	op->smaller_begin = "<small>";
	op->smaller_end = "</small>";

	op->bigger_begin = "<big>";
	op->bigger_end = "</big>";

	op->foreground_begin = "<font color=\"%s\">";
	op->foreground_end = "</font>";

	op->background_begin = "<span style=\"background:%s\">";
	op->background_end = "</span>";

	op->bold_begin = "<b>";
	op->bold_end = "</b>";

	op->italic_begin = "<i>";
	op->italic_end = "</i>";

	op->underline_begin = "<u>";
	op->underline_end = "</u>";

	op->dbl_underline_begin = "<u>";
	op->dbl_underline_end = "</u>";

	op->superscript_begin = "<sup>";
	op->superscript_end = "</sup>";

	op->subscript_begin = "<sub>";
	op->subscript_end = "</sub>";

	op->strikethru_begin = "<s>";
	op->strikethru_end = "</s>";

	op->dbl_strikethru_begin = "<s>";
	op->dbl_strikethru_end = "</s>";

	op->emboss_begin="<span style=\"background:gray\"><font color=black>";
	op->emboss_end = "</font></span>";

	op->engrave_begin = "<span style=\"background:gray\"><font color=navyblue>";
	op->engrave_end = "</font></span>";

	op->shadow_begin= "<span style=\"background:gray\">";
	op->shadow_end= "</span>";

	op->outline_begin= "<span style=\"background:gray\">";
	op->outline_end= "</span>";

	op->expand_begin = "<span style=\"letter-spacing: %s\">";
	op->expand_end = "</span>";

	op->pointlist_begin = "<ol>\n";
	op->pointlist_end = "</ol>\n";
	op->pointlist_item_begin = "<li>";
	op->pointlist_item_end = "</li>\n";

	op->numericlist_begin = "<ul>\n";
	op->numericlist_end = "</ul>\n";
	op->numericlist_item_begin = "<li>";
	op->numericlist_item_end = "</li>\n";

	op->simulate_small_caps = true;
	op->simulate_all_caps = true;
	op->simulate_word_underline = true;

	op->ascii_translation_table = ascii;

	op->ansi_translation_table = ansi;
#if 1 /* daved - 0.9.6 */
	op->ansi_first_char = 0x78;
#else
	op->ansi_first_char = 0x82;
#endif
	op->ansi_last_char = 0xff;

	op->cp437_translation_table = cp437;
	op->cp437_first_char = 0x80;
	op->cp437_last_char = 0xff;

	op->cp850_translation_table = cp850;
	op->cp850_first_char = 0x80;
	op->cp850_last_char = 0xff;

	op->mac_translation_table = mac;
	op->mac_first_char = 0xa4;
	op->mac_last_char = 0xd5;

#if 1 /* daved 0.19.8 */
	op->chars.right_quote = "&rsquo;";
	op->chars.left_quote = "&lsquo;";
	op->chars.right_dbl_quote = "&rdquo;";
	op->chars.left_dbl_quote = "&ldquo;";
#else
	op->chars.right_quote = "'";
	op->chars.left_quote = "`";
	op->chars.right_dbl_quote = "\"";
	op->chars.left_dbl_quote = "\"";
#endif
#if 1 /* daved - 0.19.8 */
	op->chars.endash = "&ndash;";
	op->chars.emdash = "&mdash;";
	op->chars.bullet = "&bull;";
	op->chars.lessthan = "&lt;";
	op->chars.greaterthan = "&gt;";
	op->chars.amp = "&amp;";
	op->chars.copyright = "&copy;";
	op->chars.trademark = "&trade;";
	op->chars.nonbreaking_space = "&nbsp;";
#endif

#if 1 /* daved - 0.19.4 - unicode symbol character support */
	op->unisymbol1_first_char = 913;
        op->unisymbol1_last_char = 982;
	op->unisymbol1_translation_table = unisymbol1;
	op->unisymbol2_first_char = 57516;
        op->unisymbol2_last_char = 57557;
	op->unisymbol2_translation_table = unisymbol2;
	op->unisymbol3_first_char = 61505;
        op->unisymbol3_last_char = 61562;
	op->unisymbol3_translation_table = unisymbol3;
#endif
#if 1 /* daved - 0.19.5 - more unicode symbol character support */
	op->unisymbol4_first_char = 61600;
        op->unisymbol4_last_char = 61694;
	op->unisymbol4_translation_table = unisymbol4;
#endif
#if 1 /* daved - 0.19.5 - SYMBOL font support */
	op->symbol_first_char = 60;
    op->symbol_last_char = 254;
	op->symbol_translation_table = symbol;
#endif
#if 1 /* daved - 0.20.3 - GREEK font support */
	op->greek_first_char = 0x80;
    op->greek_last_char = 0xff;
	op->greek_translation_table = Greek;
#endif

	op->unisymbol_print = html_unisymbol_print;

	return op;
}
