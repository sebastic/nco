/* $Header: /data/zender/nco_20150216/nco/src/nco/ncap.h,v 1.4 2000-01-17 01:53:56 zender Exp $ */

/* Header file for netCDF arithmetic processor */ 

/* Copyright (C) 1995--2000 Charlie Zender

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

   The file LICENSE contains the GNU General Public License, version 2
   It may be viewed interactively by typing, e.g., ncks -L

   The author of this software, Charlie Zender, would like to receive
   your suggestions, improvements, bug-reports, and patches for NCO.
   Please contact me via e-mail at zender@uci.edu or by writing

   Charlie Zender
   Department of Earth System Science
   University of California at Irvine
   Irvine, CA 92697-3100
 */

#ifndef NCAP_H /* This include file has not yet been defined in the current source file */ 

#define SYM_NBR_MAX 20 /* maximum number of symbols (should make table a linked list) */ 

/* Define a symbol table */ 
/* NB: YACC seems to initialize all user-defined pointers (like those in the symbol table) to NULL. 
   A symbol's type may, therefore, safely be determined by testing for non-NULL members.
   This technique is frequently used in the pigeon book. */
typedef struct {
  char *nm;
  var_sct *var;
  double (*fnc)();
  double val;
} sym_sct;
sym_sct sym_tbl[SYM_NBR_MAX];

/* These funtions are kept either in ncap.y or ncap_utl.c */ 
extern sym_sct *sym_look(char *sym_nm);
extern void fnc_add(char *nm, double (*fnc)());
extern sym_sct *scalar_mk_sym(double val);
extern var_sct *ncap_var_add(var_sct *var_1,var_sct *var_2);
extern int ncap_write_var(int,var_sct *);
extern int yyerror(char *sng);
extern void nc_lib_vrs_prn();

#endif /* NCAP_H */ 







