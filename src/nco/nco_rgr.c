/* $Header$ */

/* Purpose: NCO utilities for regridding */

/* Copyright (C) 2015--2015 Charlie Zender
   This file is part of NCO, the netCDF Operators. NCO is free software.
   You may redistribute and/or modify NCO under the terms of the 
   GNU General Public License (GPL) Version 3 with exceptions described in the LICENSE file */

#include "nco_rgr.h" /* Regridding */

int /* O [enm] Return code */
nco_rgr_ctl /* [fnc] Control regridding logic */
(rgr_sct * const rgr, /* I/O [sct] Regridding structure */
 trv_tbl_sct * const trv_tbl) /* I/O [sct] Traversal Table */
{
  /* Purpose: Control regridding logic */
  int rcd=NCO_NOERR;
  const char fnc_nm[]="nco_rgr_ctl()";

  nco_bool flg_map=False; /* [flg] Weight-based regridding */
  nco_bool flg_smf=False; /* [flg] ESMF regridding */
  nco_bool flg_tps=False; /* [flg] Tempest regridding */

  /* Main control branching occurs here
     Branching complexity and utility will increase as regridding features are added */
  if(rgr->flg_map) flg_map=True;
  if(rgr->flg_grd_src && rgr->flg_grd_dst) flg_smf=True;
  if(rgr->drc_tps && !flg_map) flg_tps=True;
  assert(!(flg_smf && flg_map));
  assert(!(flg_smf && flg_tps));
  assert(!(flg_map && flg_tps));
  
  if(flg_map){
    /* Regrid using external mapping weights */
    rcd=nco_rgr_map(rgr,trv_tbl);
  } /* !flg_map */

  if(flg_smf){
#ifdef ENABLE_ESMF
    /* Regrid using ESMF library */
    (void)fprintf(stderr,"%s: %s calling nco_rgr_esmf() to generate and apply regridding map\n",nco_prg_nm_get(),fnc_nm);
    rcd=nco_rgr_esmf(rgr);
    /* Close output and free dynamic memory */
    (void)nco_fl_out_cls(rgr->fl_out,rgr->fl_out_tmp,rgr->out_id);
#else /* !ENABLE_ESMF */
    (void)fprintf(stderr,"%s: ERROR %s reports attempt to use ESMF regridding without built-in support. Re-configure with --enable_esmf.\n",nco_prg_nm_get(),fnc_nm);
    nco_exit(EXIT_FAILURE);
#endif /* !ENABLE_ESMF */
} /* !flg_smf */
  
  if(flg_tps){
    /* Regrid using Tempest regridding */
    rcd=nco_rgr_tps(rgr);
  } /* !flg_map */

  return rcd;
} /* end nco_rgr_ctl() */

rgr_sct * /* O [sct] Pointer to free'd regridding structure */
nco_rgr_free /* [fnc] Deallocate regridding structure */
(rgr_sct *rgr) /* I/O [sct] Regridding structure */
{
  /* Purpose: Free all dynamic memory in regridding structure */
  if(rgr->fl_grd_src) rgr->fl_grd_src=(char *)nco_free(rgr->fl_grd_src);
  if(rgr->fl_grd_dst) rgr->fl_grd_dst=(char *)nco_free(rgr->fl_grd_dst);
  if(rgr->fl_in) rgr->fl_in=(char *)nco_free(rgr->fl_in);
  if(rgr->fl_out) rgr->fl_out=(char *)nco_free(rgr->fl_out);
  if(rgr->fl_out_tmp) rgr->fl_out_tmp=(char *)nco_free(rgr->fl_out_tmp);
  if(rgr->fl_map) rgr->fl_map=(char *)nco_free(rgr->fl_map);
  if(rgr->var_nm) rgr->var_nm=(char *)nco_free(rgr->var_nm);

  /* Tempest */
  if(rgr->drc_tps) rgr->drc_tps=(char *)nco_free(rgr->drc_tps);

  /* free() memory used to construct KVMs */
  if(rgr->rgr_nbr > 0) rgr->rgr_arg=nco_sng_lst_free(rgr->rgr_arg,rgr->rgr_nbr);

  /* free() memory copied from KVMs */
  if(rgr->area_nm) rgr->area_nm=(char *)nco_free(rgr->area_nm);
  if(rgr->bnd_nm) rgr->bnd_nm=(char *)nco_free(rgr->bnd_nm);
  if(rgr->bnd_tm_nm) rgr->bnd_tm_nm=(char *)nco_free(rgr->bnd_tm_nm);
  if(rgr->col_nm) rgr->col_nm=(char *)nco_free(rgr->col_nm);
  if(rgr->lat_bnd_nm) rgr->lat_bnd_nm=(char *)nco_free(rgr->lat_bnd_nm);
  if(rgr->lat_nm) rgr->lat_nm=(char *)nco_free(rgr->lat_nm);
  if(rgr->lat_vrt_nm) rgr->lat_vrt_nm=(char *)nco_free(rgr->lat_vrt_nm);
  if(rgr->lat_wgt_nm) rgr->lat_wgt_nm=(char *)nco_free(rgr->lat_wgt_nm);
  if(rgr->lon_bnd_nm) rgr->lon_bnd_nm=(char *)nco_free(rgr->lon_bnd_nm);
  if(rgr->lon_nm) rgr->lon_nm=(char *)nco_free(rgr->lon_nm);
  if(rgr->lon_vrt_nm) rgr->lon_vrt_nm=(char *)nco_free(rgr->lon_vrt_nm);
  if(rgr->vrt_nm) rgr->vrt_nm=(char *)nco_free(rgr->vrt_nm);

  /* Lastly, free() regrid structure itself */
  if(rgr) rgr=(rgr_sct *)nco_free(rgr);

  return rgr;
} /* end nco_rfr_free() */
  
rgr_sct * /* O [sct] Regridding structure */
nco_rgr_ini /* [fnc] Initialize regridding structure */
(const int in_id, /* I [id] Input netCDF file ID */
 char **rgr_arg, /* [sng] Regridding arguments */
 const int rgr_arg_nbr, /* [nbr] Number of regridding arguments */
 char * const rgr_in, /* I [sng] File containing fields to be regridded */
 char * const rgr_out, /* I [sng] File containing regridded fields */
 char * const rgr_grd_src, /* I [sng] File containing input grid */
 char * const rgr_grd_dst, /* I [sng] File containing destination grid */
 char * const rgr_map, /* I [sng] File containing mapping weights from source to destination grid */
 char * const rgr_var, /* I [sng] Variable for special regridding treatment */
 const double wgt_vld_thr) /* I [frc] Weight threshold for valid destination value */
{
  /* Purpose: Initialize regridding structure */
     
  /* Sample calls:
     Debugging:
     ncks -O -D 6 --rgr=Y ${DATA}/rgr/dstmch90_clm.nc ~/foo.nc

     T62->T42 from scratch, minimal arguments:
     ncks -O -D 6 --rgr=Y ${DATA}/rgr/dstmch90_clm.nc ~/foo.nc
     T62->T42 from scratch, more arguments:
     ncks -O -D 6 --rgr=Y --rgr_grd_dst=${DATA}/scrip/grids/remap_grid_T42.nc ${DATA}/rgr/dstmch90_clm.nc ~/foo.nc
     T62->T42 from scratch, explicit arguments:
     ncks -O --rgr=Y --rgr_grd_dst=${DATA}/scrip/grids/remap_grid_T42.nc --rgr_out=${DATA}/rgr/rgr_out.nc ${DATA}/rgr/dstmch90_clm.nc ~/foo.nc
     T42->T42 from scratch:
     ncks -O --rgr=Y --rgr_grd_src=${DATA}/scrip/grids/remap_grid_T42.nc --rgr_grd_dst=${DATA}/scrip/grids/remap_grid_T42.nc --rgr_out=${DATA}/rgr/rgr_out.nc ${DATA}/rgr/essgcm14_clm.nc ~/foo.nc
     T42->POP43 from existing weights:
     ncks -O --rgr=Y --rgr_map=${DATA}/scrip/rmp_T42_to_POP43_conserv.nc --rgr_out=${DATA}/rgr/rgr_out.nc ${DATA}/rgr/essgcm14_clm.nc ~/foo.nc */

  const char fnc_nm[]="nco_rgr_ini()";
  
  rgr_sct *rgr;

  /* Allocate */
  rgr=(rgr_sct *)nco_malloc(sizeof(rgr_sct));
  
  /* Initialize */
  rgr->flg_usr_rqs=False; /* [flg] User requested regridding */
  rgr->out_id=int_CEWI; /* [id] Output netCDF file ID */

  rgr->in_id=in_id; /* [id] Input netCDF file ID */
  rgr->rgr_arg=rgr_arg; /* [sng] Regridding arguments */
  rgr->rgr_nbr=rgr_arg_nbr; /* [nbr] Number of regridding arguments */

  rgr->drc_tps=NULL; /* [sng] Directory where Tempest grids, meshes, and weights are stored */

  rgr->flg_grd_src= rgr_grd_src ? True : False; /* [flg] User-specified input grid */
  rgr->fl_grd_src=rgr_grd_src; /* [sng] File containing input grid */

  rgr->flg_grd_dst= rgr_grd_dst ? True : False; /* [flg] User-specified destination grid */
  rgr->fl_grd_dst=rgr_grd_dst; /* [sng] File containing destination grid */

  rgr->fl_in=rgr_in; /* [sng] File containing fields to be regridded */
  rgr->fl_out=rgr_out; /* [sng] File containing regridded fields */
  rgr->fl_out_tmp=NULL_CEWI; /* [sng] Temporary file containing regridded fields */

  rgr->flg_map= rgr_map ? True : False; /* [flg] User-specified mapping weights */
  rgr->fl_map=rgr_map; /* [sng] File containing mapping weights from source to destination grid */

  rgr->var_nm=rgr_var; /* [sng] Variable for special regridding treatment */
  
  /* Did user explicitly request regridding? */
  if(rgr_arg_nbr > 0 || rgr_grd_src != NULL || rgr_grd_dst != NULL || rgr_map != NULL) rgr->flg_usr_rqs=True;

  /* Initialize arguments after copying */
  if(!rgr->fl_out) rgr->fl_out=(char *)strdup("/data/zender/rgr/rgr_out.nc");
  if(!rgr->fl_grd_dst) rgr->fl_grd_dst=(char *)strdup("/data/zender/scrip/grids/remap_grid_T42.nc");
  if(!rgr->var_nm) rgr->var_nm=(char *)strdup("ORO");
  
  if(nco_dbg_lvl_get() >= nco_dbg_crr){
    (void)fprintf(stderr,"%s: INFO %s reports ",nco_prg_nm_get(),fnc_nm);
    (void)fprintf(stderr,"flg_usr_rqs = %d, ",rgr->flg_usr_rqs);
    (void)fprintf(stderr,"rgr_nbr = %d, ",rgr->rgr_nbr);
    (void)fprintf(stderr,"fl_grd_src = %s, ",rgr->fl_grd_src ? rgr->fl_grd_src : "NULL");
    (void)fprintf(stderr,"fl_grd_dst = %s, ",rgr->fl_grd_dst ? rgr->fl_grd_dst : "NULL");
    (void)fprintf(stderr,"fl_in = %s, ",rgr->fl_in ? rgr->fl_in : "NULL");
    (void)fprintf(stderr,"fl_out = %s, ",rgr->fl_out ? rgr->fl_out : "NULL");
    (void)fprintf(stderr,"fl_out_tmp = %s, ",rgr->fl_out_tmp ? rgr->fl_out_tmp : "NULL");
    (void)fprintf(stderr,"fl_map = %s, ",rgr->fl_map ? rgr->fl_map : "NULL");
    (void)fprintf(stderr,"\n");
  } /* endif dbg */
  
  /* Flags */
  if(wgt_vld_thr == NC_MIN_DOUBLE){
    rgr->flg_rnr=False;
  }else if(wgt_vld_thr >= 0.0 && wgt_vld_thr <= 1.0){
    /* NB: Weight thresholds of 0.0 or nearly zero can lead to underflow or divide-by-zero errors */
    // const double wgt_vld_thr_min=1.0e-10; /* [frc] Minimum weight threshold for valid destination value */
    rgr->flg_rnr=True;
    rgr->wgt_vld_thr=wgt_vld_thr;
  }else{
    (void)fprintf(stderr,"%s: ERROR weight threshold must be in [0.0,1.0] and user supplied wgt_vld_thr = %g\n",nco_prg_nm_get(),wgt_vld_thr);
    nco_exit(EXIT_FAILURE);
  } /* endif */
  
  /* Parse extended kvm options */
  int rgr_arg_idx; /* [idx] Index over rgr_arg (i.e., separate invocations of "--rgr var1[,var2]=val") */
  int rgr_var_idx; /* [idx] Index over rgr_lst (i.e., all names explicitly specified in all "--rgr var1[,var2]=val" options) */
  int rgr_var_nbr=0;
  kvm_sct *rgr_lst; /* [sct] List of all regrid specifications */
  kvm_sct kvm;

  rgr_lst=(kvm_sct *)nco_malloc(NC_MAX_VARS*sizeof(kvm_sct));

  /* Parse RGRs */
  for(rgr_arg_idx=0;rgr_arg_idx<rgr_arg_nbr;rgr_arg_idx++){
    if(!strstr(rgr_arg[rgr_arg_idx],"=")){
      (void)fprintf(stdout,"%s: Invalid --rgr specification: %s. Must contain \"=\" sign, e.g., \"key=value\".\n",nco_prg_nm_get(),rgr_arg[rgr_arg_idx]);
      if(rgr_lst) rgr_lst=(kvm_sct *)nco_free(rgr_lst);
      nco_exit(EXIT_FAILURE);
    } /* endif */
    kvm=nco_sng2kvm(rgr_arg[rgr_arg_idx]);
    /* nco_sng2kvm() converts argument "--rgr one,two=3" into kvm.key="one,two" and kvm.val=3
       Then nco_lst_prs_2D() converts kvm.key into two items, "one" and "two", with the same value, 3 */
    if(kvm.key){
      int var_idx; /* [idx] Index over variables in current RGR argument */
      int var_nbr; /* [nbr] Number of variables in current RGR argument */
      char **var_lst;
      var_lst=nco_lst_prs_2D(kvm.key,",",&var_nbr);
      for(var_idx=0;var_idx<var_nbr;var_idx++){ /* Expand multi-variable specification */
        rgr_lst[rgr_var_nbr].key=strdup(var_lst[var_idx]);
        rgr_lst[rgr_var_nbr].val=strdup(kvm.val);
        rgr_var_nbr++;
      } /* end for */
      var_lst=nco_sng_lst_free(var_lst,var_nbr);
    } /* end if */
  } /* end for */

  /* NULL-initialize */
  rgr->area_nm=NULL; /* [sng] Name of variable containing gridcell area */
  rgr->bnd_nm=NULL; /* [sng] Name of dimension to employ for spatial bounds */
  rgr->bnd_tm_nm=NULL; /* [sng] Name of dimension to employ for temporal bounds */
  rgr->col_nm=NULL; /* [sng] Name of horizontal spatial dimension on unstructured grid */
  rgr->lat_bnd_nm=NULL; /* [sng] Name of rectangular boundary variable for latitude */
  rgr->lat_nm=NULL; /* [sng] Name of dimension to recognize as latitude */
  rgr->lat_vrt_nm=NULL; /* [sng] Name of non-rectangular boundary variable for latitude */
  rgr->lat_wgt_nm=NULL; /* [sng] Name of variable containing latitude weights */
  rgr->lon_bnd_nm=NULL; /* [sng] Name of rectangular boundary variable for longitude */
  rgr->lon_nm=NULL; /* [sng] Name of dimension to recognize as longitude */
  rgr->lon_vrt_nm=NULL; /* [sng] Name of non-rectangular boundary variable for longitude */
  rgr->vrt_nm=NULL; /* [sng] Name of dimension to employ for vertices */

  for(rgr_var_idx=0;rgr_var_idx<rgr_var_nbr;rgr_var_idx++){
    if(!strcasecmp(rgr_lst[rgr_var_idx].key,"area_nm")){
      rgr->area_nm=(char *)strdup(rgr_lst[rgr_var_idx].val);
      continue;
    } /* endif */
    if(!strcasecmp(rgr_lst[rgr_var_idx].key,"bnd_nm")){
      rgr->bnd_nm=(char *)strdup(rgr_lst[rgr_var_idx].val);
      continue;
    } /* endif */
    if(!strcasecmp(rgr_lst[rgr_var_idx].key,"bnd_tm_nm")){
      rgr->bnd_tm_nm=(char *)strdup(rgr_lst[rgr_var_idx].val);
      continue;
    } /* endif */
    if(!strcasecmp(rgr_lst[rgr_var_idx].key,"col_nm")){
      rgr->col_nm=(char *)strdup(rgr_lst[rgr_var_idx].val);
      continue;
    } /* endif */
    if(!strcasecmp(rgr_lst[rgr_var_idx].key,"lat_bnd_nm")){
      rgr->lat_bnd_nm=(char *)strdup(rgr_lst[rgr_var_idx].val);
      continue;
    } /* endif */
    if(!strcasecmp(rgr_lst[rgr_var_idx].key,"lat_nm")){
      rgr->lat_nm=(char *)strdup(rgr_lst[rgr_var_idx].val);
      continue;
    } /* endif */
    if(!strcasecmp(rgr_lst[rgr_var_idx].key,"lat_vrt_nm")){
      rgr->lat_vrt_nm=(char *)strdup(rgr_lst[rgr_var_idx].val);
      continue;
    } /* endif */
    if(!strcasecmp(rgr_lst[rgr_var_idx].key,"lat_wgt_nm")){
      rgr->lat_wgt_nm=(char *)strdup(rgr_lst[rgr_var_idx].val);
      continue;
    } /* endif */
    if(!strcasecmp(rgr_lst[rgr_var_idx].key,"lon_bnd_nm")){
      rgr->lon_bnd_nm=(char *)strdup(rgr_lst[rgr_var_idx].val);
      continue;
    } /* endif */
    if(!strcasecmp(rgr_lst[rgr_var_idx].key,"lon_nm")){
      rgr->lon_nm=(char *)strdup(rgr_lst[rgr_var_idx].val);
      continue;
    } /* endif */
    if(!strcasecmp(rgr_lst[rgr_var_idx].key,"lon_vrt_nm")){
      rgr->lon_vrt_nm=(char *)strdup(rgr_lst[rgr_var_idx].val);
      continue;
    } /* endif */
    if(!strcasecmp(rgr_lst[rgr_var_idx].key,"vrt_nm")){
      rgr->vrt_nm=(char *)strdup(rgr_lst[rgr_var_idx].val);
      continue;
    } /* endif */
    (void)fprintf(stderr,"%s: ERROR %s reports unrecognized key-value option to --rgr switch: %s\n",nco_prg_nm_get(),fnc_nm,rgr_lst[rgr_var_idx].key);
    nco_exit(EXIT_FAILURE);
  } /* end for */

  /* Revert to defaults for any not specified on command-line */
  if(!rgr->area_nm) rgr->area_nm=(char *)strdup("area"); /* [sng] Name of variable containing gridcell area */
  if(!rgr->bnd_nm) rgr->bnd_nm=(char *)strdup("nbnd"); /* [sng] Name of dimension to employ for spatial bounds */
  /* NB: CESM uses nbnd for temporal bounds. NCO defaults to nbnd for all bounds with two endpoints */
  if(!rgr->bnd_tm_nm) rgr->bnd_tm_nm=(char *)strdup("nbnd"); /* [sng] Name of dimension to employ for spatial bounds */
  if(!rgr->col_nm) rgr->col_nm=(char *)strdup("ncol"); /* [sng] Name of horizontal spatial dimension on unstructured grid */
  if(!rgr->lat_bnd_nm) rgr->lat_bnd_nm=(char *)strdup("lat_bnds"); /* [sng] Name of rectangular boundary variable for latitude */
  if(!rgr->lat_nm) rgr->lat_nm=(char *)strdup("lat"); /* [sng] Name of dimension to recognize as latitude */
  if(!rgr->lat_vrt_nm) rgr->lat_vrt_nm=(char *)strdup("lat_vertices"); /* [sng] Name of non-rectangular boundary variable for latitude */
  if(!rgr->lat_wgt_nm) rgr->lat_wgt_nm=(char *)strdup("gw"); /* [sng] Name of variable containing latitude weights */
  if(!rgr->lon_bnd_nm) rgr->lon_bnd_nm=(char *)strdup("lon_bnds"); /* [sng] Name of rectangular boundary variable for longitude */
  if(!rgr->lon_nm) rgr->lon_nm=(char *)strdup("lon"); /* [sng] Name of dimension to recognize as longitude */
  if(!rgr->lon_vrt_nm) rgr->lon_vrt_nm=(char *)strdup("lon_vertices"); /* [sng] Name of non-rectangular boundary variable for longitude */
  if(!rgr->vrt_nm) rgr->vrt_nm=(char *)strdup("nv"); /* [sng] Name of dimension to employ for vertices */

  /* Free kvms */
  if(rgr_lst) rgr_lst=nco_kvm_lst_free(rgr_lst,rgr_var_nbr);

  return rgr;
} /* end nco_rgr_ini() */
  
int /* O [enm] Return code */
nco_rgr_map /* [fnc] Regrid using external weights */
(rgr_sct * const rgr, /* I/O [sct] Regridding structure */
 trv_tbl_sct * const trv_tbl) /* I/O [sct] Traversal Table */
{
  /* Purpose: Regrid fields using external weights (i.e., a mapping file)

     Examine SCRIP remap file:
     ncks --cdl -m ${DATA}/scrip/rmp_T42_to_POP43_conserv.nc | m

     Test SCRIP remapping file:
     ncks -D 6 -O --rgr=Y --rgr_map=${DATA}/scrip/rmp_T42_to_POP43_conserv.nc ${DATA}/rgr/essgcm14_clm.nc ~/foo.nc
     Test ESMF remapping file:
     ncks -D 6 -O --rgr=Y --rgr_map=${DATA}/rgr/map_ne120np4_to_181x360_aave.nc ${DATA}/rgr/essgcm14_clm.nc ~/foo.nc
 
     Conventions:
     grid_size: Number of grid cells (product of lat*lon)
     address: Source and destination index for each link pair
     num_links: Number of unique address pairs in remapping, i.e., size of sparse matrix
     num_wgts: Number of weights per vertice for given remapping
     = 1 Bilinear
         Destination grid value determined by weights times known source grid values 
         at vertices of source quadrilateral that bounds destination point P
         One weight per vertice guarantees fxm but is not conservative
         Bilinear requires logically rectangular grid
     = 1 Distance-based:
	 Distance-weighted uses values at num_neighbors points
	 The weight is inversely proportional to the angular distance from 
	 the destination point to each neighbor on the source grid
     = 3 Second-order conservative:
         Described in Jones, P. W. (1999), Monthly Weather Review, 127, 2204-2210
         First-order conservative schemes assume fluxes are constant within gridcell
	 Destination fluxes are simple summations of sources fluxes weighted by overlap areas
	 Old clm and bds remappers use a first-order algorithm
	 Second-order improves this by using a first-order Taylor expansion of flux
	 Source flux is centroid value plus directional offset determined by dot product
	 of directional gradient and vector pointing from vertice to centroid.
         Three weights per vertice are centroid weight, weight times local theta-gradient from
	 centroid to vertice, and weight times local phi-gradient from centroid to vertice.
     = 4 Bicubic: 
         The four weights are gradients in each direction plus a cross-gradient term
         Same principle as bilinear, but more weights per vertice
         Bicubic requires logically rectangular grid
     
     wgt: 
     Maximum number of source cells contributing to destination cell is not a dimension
     in SCRIP remapping files because SCRIP stores everying in 1-D sparse matrix arrays
     Sparse matrix formulations:

     for(lnk_idx=0;lnk_idx<lnk_nbr;lnk_idx++){
       // Normalization: fractional area
       dst[ddr_dst[lnk_idx]]+=src[ddr_src[lnk_idx]]*remap_matrix[lnk_idx,0];
       // Normalization: destination area
       dst[ddr_dst[lnk_idx]]+=src[ddr_src[lnk_idx]]*remap_matrix[lnk_idx,0]/dst_area[ddr_dst[lnk_idx]];
       // Normalization: none
       dst[ddr_dst[lnk_idx]]+=src[ddr_src[lnk_idx]]*remap_matrix[lnk_idx,0]/(dst_area[ddr_dst[lnk_idx]]*dst_frc[ddr_dst[lnk_idx]);
     } // end loop over lnk

     Documentation:
     NCL special cases described in popRemap.ncl, e.g., at
     https://github.com/yyr/ncl/blob/master/ni/src/examples/gsun/popRemap.ncl */

  const char fnc_nm[]="nco_rgr_map()"; /* [sng] Function name */

  char *fl_in;
  char *fl_pth_lcl=NULL;

  int in_id; /* I [id] Input netCDF file ID */
  int out_id; /* I [id] Output netCDF file ID */
  int md_open; /* [enm] Mode flag for nc_open() call */
  int rcd=NC_NOERR;

  int dst_grid_corners_id; /* [id] Destination grid corners dimension ID */
  int dst_grid_rank_id; /* [id] Destination grid rank dimension ID */
  int dst_grid_size_id; /* [id] Destination grid size dimension ID */
  int num_links_id; /* [id] Number of links dimension ID */
  int num_wgts_id; /* [id] Number of weights dimension ID */
  int src_grid_corners_id; /* [id] Source grid corners dimension ID */
  int src_grid_rank_id; /* [id] Source grid rank dimension ID */
  int src_grid_size_id; /* [id] Source grid size dimension ID */

  nco_bool FL_RTR_RMT_LCN;
  nco_bool RAM_OPEN=False; /* [flg] Open (netCDF3-only) file(s) in RAM */
  nco_bool RM_RMT_FL_PST_PRC=True; /* Option R */
  
  nco_map_sct rgr_map;

  size_t bfr_sz_hnt=NC_SIZEHINT_DEFAULT; /* [B] Buffer size hint */
  
  if(nco_dbg_lvl_get() >= nco_dbg_crr) (void)fprintf(stderr,"%s: INFO %s obtaining mapping weights from %s\n",nco_prg_nm_get(),fnc_nm,rgr->fl_map);

  /* Duplicate (because nco_fl_mk_lcl() free()'s fl_in) */
  fl_in=(char *)strdup(rgr->fl_map);
  /* Make sure file is on local system and is readable or die trying */
  fl_in=nco_fl_mk_lcl(fl_in,fl_pth_lcl,&FL_RTR_RMT_LCN);
  /* Open file using appropriate buffer size hints and verbosity */
  if(RAM_OPEN) md_open=NC_NOWRITE|NC_DISKLESS; else md_open=NC_NOWRITE;
  rcd+=nco_fl_open(fl_in,md_open,&bfr_sz_hnt,&in_id);

  /* Identify mapping file type
     Generated by SCRIP: conventions = "SCRIP"
     Generated by ESMF_Regridder: conventions = "NCAR-CSM" */
  char *att_val;
  char *cnv_sng=NULL_CEWI;
  /* netCDF standard is uppercase Conventions, though some models user lowercase */
  char cnv_sng_UC[]="Conventions"; /* Unidata standard     string (uppercase) */
  char cnv_sng_LC[]="conventions"; /* Unidata non-standard string (lowercase) */
  
  long att_sz;

  nc_type att_typ;

  nco_rgr_mpf_typ_enm nco_rgr_mpf_typ=nco_rgr_mpf_nil; /* [enm] Type of remapping file */
  nco_rgr_grd_typ_enm nco_rgr_grd_typ=nco_rgr_grd_nil; /* [enm] Type of grid conversion */
  
  /* Look for map-type signature in Conventions attribute */
  cnv_sng=cnv_sng_UC;
  rcd=nco_inq_att_flg(in_id,NC_GLOBAL,cnv_sng,&att_typ,&att_sz);
  if(rcd != NC_NOERR){
    /* Re-try with lowercase string because some models, e.g., CLM, user lowercase "conventions" */
    cnv_sng=cnv_sng_LC;
    rcd=nco_inq_att_flg(in_id,NC_GLOBAL,cnv_sng,&att_typ,&att_sz);
  } /* endif lowercase */
  
  if(rcd == NC_NOERR && att_typ == NC_CHAR){
    /* Add one for NUL byte */
    att_val=(char *)nco_malloc(att_sz*nco_typ_lng(att_typ)+1L);
    rcd+=nco_get_att(in_id,NC_GLOBAL,cnv_sng,att_val,att_typ);
    /* NUL-terminate convention attribute before using strcmp() */
    att_val[att_sz]='\0';
    /* ESMF conventions */
    if(strstr(att_val,"NCAR-CSM")) nco_rgr_mpf_typ=nco_rgr_mpf_ESMF;
    if(strstr(att_val,"SCRIP")) nco_rgr_mpf_typ=nco_rgr_mpf_SCRIP;
    if(nco_rgr_mpf_typ == nco_rgr_mpf_nil) (void)fprintf(stderr,"%s: ERROR %s unrecognized map-type specified in attribute Conventions = %s\n",nco_prg_nm_get(),fnc_nm,att_val);
    if(att_val) att_val=(char *)nco_free(att_val);
  } /* end rcd && att_typ */

  switch(nco_rgr_mpf_typ){
  case nco_rgr_mpf_SCRIP:
    rcd+=nco_inq_dimid(in_id,"src_grid_size",&src_grid_size_id);
    rcd+=nco_inq_dimid(in_id,"dst_grid_size",&dst_grid_size_id);
    rcd+=nco_inq_dimid(in_id,"src_grid_corners",&src_grid_corners_id);
    rcd+=nco_inq_dimid(in_id,"dst_grid_corners",&dst_grid_corners_id);
    rcd+=nco_inq_dimid(in_id,"src_grid_rank",&src_grid_rank_id);
    rcd+=nco_inq_dimid(in_id,"dst_grid_rank",&dst_grid_rank_id);
    rcd+=nco_inq_dimid(in_id,"num_links",&num_links_id);
    rcd+=nco_inq_dimid(in_id,"num_wgts",&num_wgts_id);
  break;
  case nco_rgr_mpf_ESMF:
    rcd+=nco_inq_dimid(in_id,"n_a",&src_grid_size_id);
    rcd+=nco_inq_dimid(in_id,"n_b",&dst_grid_size_id);
    rcd+=nco_inq_dimid(in_id,"nv_a",&src_grid_corners_id);
    rcd+=nco_inq_dimid(in_id,"nv_b",&dst_grid_corners_id);
    rcd+=nco_inq_dimid(in_id,"src_grid_rank",&src_grid_rank_id);
    rcd+=nco_inq_dimid(in_id,"dst_grid_rank",&dst_grid_rank_id);
    rcd+=nco_inq_dimid(in_id,"n_s",&num_links_id);
    rcd+=nco_inq_dimid(in_id,"num_wgts",&num_wgts_id);
    break;
  default:
    (void)fprintf(stderr,"%s: ERROR %s unknown map file type\n",nco_prg_nm_get(),fnc_nm);
    nco_dfl_case_generic_err(); break;
  } /* end switch */
    
  /* Now we have dimension IDs, get dimension sizes */
  rcd+=nco_inq_dimlen(in_id,src_grid_size_id,&rgr_map.src_grid_size);
  rcd+=nco_inq_dimlen(in_id,dst_grid_size_id,&rgr_map.dst_grid_size);
  rcd+=nco_inq_dimlen(in_id,src_grid_corners_id,&rgr_map.src_grid_corners);
  rcd+=nco_inq_dimlen(in_id,dst_grid_corners_id,&rgr_map.dst_grid_corners);
  rcd+=nco_inq_dimlen(in_id,src_grid_rank_id,&rgr_map.src_grid_rank);
  rcd+=nco_inq_dimlen(in_id,dst_grid_rank_id,&rgr_map.dst_grid_rank);
  rcd+=nco_inq_dimlen(in_id,num_links_id,&rgr_map.num_links);
  rcd+=nco_inq_dimlen(in_id,num_wgts_id,&rgr_map.num_wgts);
  
  cnv_sng=strdup("normalization");
  nco_rgr_nrm_typ_enm nco_rgr_nrm_typ=nco_rgr_nrm_nil;
  rcd=nco_inq_att_flg(in_id,NC_GLOBAL,cnv_sng,&att_typ,&att_sz);
  if(rcd == NC_NOERR && att_typ == NC_CHAR){
    /* Add one for NUL byte */
    att_val=(char *)nco_malloc(att_sz*nco_typ_lng(att_typ)+1L);
    rcd+=nco_get_att(in_id,NC_GLOBAL,cnv_sng,att_val,att_typ);
  } /* endif */
  /* NUL-terminate convention attribute before using strcmp() */
  att_val[att_sz]='\0';
  if(strstr(att_val,"fracarea")) nco_rgr_nrm_typ=nco_rgr_nrm_fracarea;
  if(strstr(att_val,"destarea")) nco_rgr_nrm_typ=nco_rgr_nrm_destarea;
  if(strstr(att_val,"none")) nco_rgr_nrm_typ=nco_rgr_nrm_none;
  assert(nco_rgr_nrm_typ == nco_rgr_nrm_destarea);
  if(att_val) att_val=(char *)nco_free(att_val);
  if(cnv_sng) cnv_sng=(char *)nco_free(cnv_sng);
  assert(rgr_map.src_grid_size < INT_MAX && rgr_map.dst_grid_size < INT_MAX);

  cnv_sng=strdup("map_method");
  nco_rgr_mth_typ_enm nco_rgr_mth_typ=nco_rgr_mth_nil;
  rcd=nco_inq_att_flg(in_id,NC_GLOBAL,cnv_sng,&att_typ,&att_sz);
  if(rcd == NC_NOERR && att_typ == NC_CHAR){
    /* Add one for NUL byte */
    att_val=(char *)nco_malloc(att_sz*nco_typ_lng(att_typ)+1L);
    rcd+=nco_get_att(in_id,NC_GLOBAL,cnv_sng,att_val,att_typ);
  } /* endif */
  /* NUL-terminate convention attribute before using strcmp() */
  att_val[att_sz]='\0';
  if(strstr(att_val,"Conservative remapping")) nco_rgr_mth_typ=nco_rgr_mth_conservative;
  if(strstr(att_val,"Bilinear remapping")) nco_rgr_mth_typ=nco_rgr_mth_bilinear;
  if(strstr(att_val,"none")) nco_rgr_mth_typ=nco_rgr_mth_none;
  assert(nco_rgr_mth_typ != nco_rgr_mth_nil);
  if(att_val) att_val=(char *)nco_free(att_val);
  if(cnv_sng) cnv_sng=(char *)nco_free(cnv_sng);

  if(nco_dbg_lvl_get() >= nco_dbg_scl){
    (void)fprintf(stderr,"%s: INFO %s regridding input metadata and grid sizes: ",nco_prg_nm_get(),fnc_nm);
    (void)fprintf(stderr,"map_method = %s, normalization = %s, src_grid_size = %li, dst_grid_size = %li, src_grid_corners = %li, dst_grid_corners = %li, src_grid_rank = %li, dst_grid_rank = %li, num_links = %li, num_wgts = %li\n",nco_rgr_mth_sng(nco_rgr_mth_typ),nco_rgr_nrm_sng(nco_rgr_nrm_typ),rgr_map.src_grid_size,rgr_map.dst_grid_size,rgr_map.src_grid_corners,rgr_map.dst_grid_corners,rgr_map.src_grid_rank,rgr_map.dst_grid_rank,rgr_map.num_links,rgr_map.num_wgts);
  } /* endif dbg */

  /* Set type of grid conversion */
  if(rgr_map.src_grid_rank == 1 && rgr_map.dst_grid_rank == 1) nco_rgr_grd_typ=nco_rgr_grd_1D_to_1D;
  if(rgr_map.src_grid_rank == 1 && rgr_map.dst_grid_rank == 2) nco_rgr_grd_typ=nco_rgr_grd_1D_to_2D;
  if(rgr_map.src_grid_rank == 2 && rgr_map.dst_grid_rank == 1) nco_rgr_grd_typ=nco_rgr_grd_2D_to_1D;
  if(rgr_map.src_grid_rank == 2 && rgr_map.dst_grid_rank == 2) nco_rgr_grd_typ=nco_rgr_grd_2D_to_2D;
  assert(nco_rgr_grd_typ != nco_rgr_grd_nil);
  /* Save typing later */
  nco_bool flg_grd_in_1D=False;
  nco_bool flg_grd_in_2D=False;
  nco_bool flg_grd_out_1D=False;
  nco_bool flg_grd_out_2D=False;
  if(nco_rgr_grd_typ == nco_rgr_grd_1D_to_1D || nco_rgr_grd_typ == nco_rgr_grd_1D_to_2D) flg_grd_in_1D=True;
  if(nco_rgr_grd_typ == nco_rgr_grd_2D_to_1D || nco_rgr_grd_typ == nco_rgr_grd_2D_to_2D) flg_grd_in_2D=True;
  if(nco_rgr_grd_typ == nco_rgr_grd_1D_to_1D || nco_rgr_grd_typ == nco_rgr_grd_2D_to_1D) flg_grd_out_1D=True;
  if(nco_rgr_grd_typ == nco_rgr_grd_1D_to_2D || nco_rgr_grd_typ == nco_rgr_grd_2D_to_2D) flg_grd_out_2D=True;

  /* Obtain grid values necessary to compute output latitude and longitude coordinates */
  int area_dst_id; /* [id] Area variable ID */
  int col_src_adr_id; /* [id] Source address (col) variable ID */
  int dmn_sz_in_int_id; /* [id] Source grid dimension sizes ID */
  int dmn_sz_out_int_id; /* [id] Destination grid dimension sizes ID */
  int dst_grd_crn_lat_id; /* [id] Destination grid corner latitudes  variable ID */
  int dst_grd_crn_lon_id; /* [id] Destination grid corner longitudes variable ID */
  int dst_grd_ctr_lat_id; /* [id] Destination grid center latitudes  variable ID */
  int dst_grd_ctr_lon_id; /* [id] Destination grid center longitudes variable ID */
  int row_dst_adr_id; /* [id] Destination address (row) variable ID */
  int wgt_raw_id; /* [id] Remap matrix variable ID */

  switch(nco_rgr_mpf_typ){
    /* Obtain fields whose name depends on mapfile type */
  case nco_rgr_mpf_SCRIP:
    rcd+=nco_inq_varid(in_id,"dst_grid_area",&area_dst_id); /* ESMF: area_b */
    rcd+=nco_inq_varid(in_id,"dst_grid_center_lon",&dst_grd_ctr_lon_id); /* ESMF: xc_b */
    rcd+=nco_inq_varid(in_id,"dst_grid_center_lat",&dst_grd_ctr_lat_id); /* ESMF: yc_b */
    rcd+=nco_inq_varid(in_id,"dst_grid_corner_lon",&dst_grd_crn_lon_id); /* ESMF: xv_b */
    rcd+=nco_inq_varid(in_id,"dst_grid_corner_lat",&dst_grd_crn_lat_id); /* ESMF: yv_b */
    rcd+=nco_inq_varid(in_id,"dst_address",&row_dst_adr_id); /* ESMF: row */
    rcd+=nco_inq_varid(in_id,"src_address",&col_src_adr_id); /* ESMF: col */
    rcd+=nco_inq_varid(in_id,"remap_matrix",&wgt_raw_id); /* fxm: remap_matrix[num_links,num_wgts] != S[n_s] */
    break;
  case nco_rgr_mpf_ESMF:
    rcd+=nco_inq_varid(in_id,"area_b",&area_dst_id); /* SCRIP: dst_grid_area */
    rcd+=nco_inq_varid(in_id,"xc_b",&dst_grd_ctr_lon_id); /* SCRIP: dst_grid_center_lon */
    rcd+=nco_inq_varid(in_id,"yc_b",&dst_grd_ctr_lat_id); /* SCRIP: dst_grid_center_lat */
    rcd+=nco_inq_varid(in_id,"xv_b",&dst_grd_crn_lon_id); /* SCRIP: dst_grid_corner_lon */
    rcd+=nco_inq_varid(in_id,"yv_b",&dst_grd_crn_lat_id); /* SCRIP: dst_grid_corner_lat */
    rcd+=nco_inq_varid(in_id,"row",&row_dst_adr_id); /* SCRIP: dst_address */
    rcd+=nco_inq_varid(in_id,"col",&col_src_adr_id); /* SCRIP: src_address */
    rcd+=nco_inq_varid(in_id,"S",&wgt_raw_id); /* fxm: remap_matrix[num_links,num_wgts] != S[n_s] */
    break;
  default:
    (void)fprintf(stderr,"%s: ERROR %s unknown map file type\n",nco_prg_nm_get(),fnc_nm);
    nco_dfl_case_generic_err(); break;
  } /* end switch */
  /* Obtain fields whose name is independent of mapfile type */
  rcd+=nco_inq_varid(in_id,"src_grid_dims",&dmn_sz_in_int_id);
  rcd+=nco_inq_varid(in_id,"dst_grid_dims",&dmn_sz_out_int_id);

  const int lon_psn_dst=0; /* [idx] Ordinal position of longitude size in rectangular destination grid */
  const int lat_psn_dst=1; /* [idx] Ordinal position of latitude  size in rectangular destination grid */
  const int dmn_nbr_1D=1; /* [nbr] Rank of 1-D grid variables */
  const int dmn_nbr_2D=2; /* [nbr] Rank of 2-D grid variables */
  const int dmn_nbr_grd_max=dmn_nbr_2D; /* [nbr] Maximum rank of grid variables */
  double *area_out; /* [sr] Area of destination grid */
  double *lon_ctr_out; /* [dgr] Longitude centers of rectangular destination grid */
  double *lat_ctr_out; /* [dgr] Latitude  centers of rectangular destination grid */
  double *lat_wgt_out=NULL; /* [dgr] Latitude  weights of rectangular destination grid */
  double *lon_crn_out=NULL; /* [dgr] Longitude corners of rectangular destination grid */
  double *lat_crn_out=NULL; /* [dgr] Latitude  corners of rectangular destination grid */
  double *lon_ntf_out=NULL; /* [dgr] Longitude interfaces of rectangular destination grid */
  double *lat_ntf_out=NULL; /* [dgr] Latitude  interfaces of rectangular destination grid */
  double *lon_bnd_out; /* [dgr] Longitude boundaries of rectangular destination grid */
  double *lat_bnd_out; /* [dgr] Latitude  boundaries of rectangular destination grid */
  double *wgt_raw; /* [frc] Remapping weights */
  int *col_src_adr; /* [idx] Source address (col) */
  int *row_dst_adr; /* [idx] Destination address (row) */
  int *dmn_sz_in_int; /* [nbr] Array of dimension sizes of source grid */
  int *dmn_sz_out_int; /* [nbr] Array of dimension sizes of destination grid */
  long *dmn_cnt;
  long *dmn_srt;
  long *dmn_srd;
  long idx; /* [idx] Counting index for unrolled grids */
    
  /* Allocate space to hold dimension metadata for rectangular destination grid */
  dmn_srt=(long *)nco_malloc(dmn_nbr_grd_max*sizeof(long));
  dmn_cnt=(long *)nco_malloc(dmn_nbr_grd_max*sizeof(long));
  dmn_srd=(long *)nco_malloc(dmn_nbr_grd_max*sizeof(long));

  dmn_srt[0]=0L;
  dmn_cnt[0]=rgr_map.src_grid_rank;
  dmn_sz_in_int=(int *)nco_malloc(rgr_map.src_grid_rank*nco_typ_lng((nc_type)NC_INT));
  rcd=nco_get_vara(in_id,dmn_sz_in_int_id,dmn_srt,dmn_cnt,dmn_sz_in_int,(nc_type)NC_INT);
  dmn_srt[0]=0L;
  dmn_cnt[0]=rgr_map.dst_grid_rank;
  dmn_sz_out_int=(int *)nco_malloc(rgr_map.dst_grid_rank*nco_typ_lng((nc_type)NC_INT));
  rcd=nco_get_vara(in_id,dmn_sz_out_int_id,dmn_srt,dmn_cnt,dmn_sz_out_int,(nc_type)NC_INT);

  long lon_nbr_in; /* [idx] Number of longitudes in rectangular source grid */
  long lat_nbr_in; /* [idx] Number of latitudes  in rectangular source grid */
  long ncol_nbr_in; /* [idx] Number of columns in source grid */
  if(flg_grd_in_1D){
    lon_nbr_in=dmn_sz_in_int[0];
    lat_nbr_in=dmn_sz_in_int[0];
    ncol_nbr_in=dmn_sz_in_int[0];
  }else if(flg_grd_in_2D){
    lon_nbr_in=dmn_sz_in_int[lon_psn_dst];
    lat_nbr_in=dmn_sz_in_int[lat_psn_dst];
    ncol_nbr_in=0;
  } /* !src_grid_rank */

  const int bnd_tm_nbr_out=2; /* [nbr] Number of boundaries for output time */
  int bnd_nbr_out; /* [nbr] Number of boundaries for output time and rectangular grid coordinates, and number of vertices for output non-rectangular grid coordinates */
  long lon_nbr_out; /* [nbr] Number of longitudes in rectangular destination grid */
  long lat_nbr_out; /* [nbr] Number of latitudes  in rectangular destination grid */
  long ncol_nbr_out; /* [nbr] Number of columns in destination grid */
  if(flg_grd_out_1D){
    bnd_nbr_out=rgr_map.dst_grid_corners; /* NB: this assumes rectangular latitude and longitude and is invalid for other quadrilaterals */
    lon_nbr_out=dmn_sz_out_int[0];
    lat_nbr_out=dmn_sz_out_int[0];
    ncol_nbr_out=dmn_sz_out_int[0];
  }else if(flg_grd_out_2D){
    bnd_nbr_out=2; /* NB: this assumes rectangular latitude and longitude and is invalid for other quadrilaterals */
    lon_nbr_out=dmn_sz_out_int[lon_psn_dst];
    lat_nbr_out=dmn_sz_out_int[lat_psn_dst];
    ncol_nbr_out=0;
  } /* !dst_grid_rank */

  if(nco_dbg_lvl_get() >= nco_dbg_scl){
    (void)fprintf(stderr,"%s: INFO %s grid conversion type = %s with expected input and prescribed output grid sizes: ",nco_prg_nm_get(),fnc_nm,nco_rgr_grd_sng(nco_rgr_grd_typ));
    (void)fprintf(stderr,"lat_in = %li, lon_in  %li, ncol_in = %li, lat_out = %li, lon_out = %li, ncol_out = %li\n",lat_nbr_in,lon_nbr_in,ncol_nbr_in,lat_nbr_out,lon_nbr_out,ncol_nbr_out);
  } /* endif dbg */

  /* Allocate space for and obtain coordinates and weights */
  nc_type crd_typ_out=NC_DOUBLE;
  area_out=(double *)nco_malloc(rgr_map.dst_grid_size*nco_typ_lng(crd_typ_out));

  if(flg_grd_out_1D){
    lon_ctr_out=(double *)nco_malloc(ncol_nbr_out*nco_typ_lng(crd_typ_out));
    lat_ctr_out=(double *)nco_malloc(ncol_nbr_out*nco_typ_lng(crd_typ_out));
    lon_bnd_out=(double *)nco_malloc(ncol_nbr_out*bnd_nbr_out*nco_typ_lng(crd_typ_out));
    lat_bnd_out=(double *)nco_malloc(ncol_nbr_out*bnd_nbr_out*nco_typ_lng(crd_typ_out));
  } /* !flg_grd_out_1D */
  if(flg_grd_out_2D){
    lon_ctr_out=(double *)nco_malloc(lon_nbr_out*nco_typ_lng(crd_typ_out));
    lat_ctr_out=(double *)nco_malloc(lat_nbr_out*nco_typ_lng(crd_typ_out));
    lat_wgt_out=(double *)nco_malloc(lat_nbr_out*nco_typ_lng(crd_typ_out));
    lon_crn_out=(double *)nco_malloc(rgr_map.dst_grid_corners*lon_nbr_out*nco_typ_lng(crd_typ_out));
    lat_crn_out=(double *)nco_malloc(rgr_map.dst_grid_corners*lat_nbr_out*nco_typ_lng(crd_typ_out));
    lon_ntf_out=(double *)nco_malloc((lon_nbr_out+1L)*nco_typ_lng(crd_typ_out));
    lat_ntf_out=(double *)nco_malloc((lat_nbr_out+1L)*nco_typ_lng(crd_typ_out));
    lon_bnd_out=(double *)nco_malloc(lon_nbr_out*bnd_nbr_out*nco_typ_lng(crd_typ_out));
    lat_bnd_out=(double *)nco_malloc(lat_nbr_out*bnd_nbr_out*nco_typ_lng(crd_typ_out));
  } /* !flg_grd_out_2D */

  wgt_raw=(double *)nco_malloc_dbg(rgr_map.num_links*nco_typ_lng(NC_DOUBLE),"Unable to malloc() value buffer for remapping weights",fnc_nm);
  col_src_adr=(int *)nco_malloc_dbg(rgr_map.num_links*nco_typ_lng(NC_INT),"Unable to malloc() value buffer for remapping addresses",fnc_nm);
  row_dst_adr=(int *)nco_malloc_dbg(rgr_map.num_links*nco_typ_lng(NC_INT),"Unable to malloc() value buffer for remapping addresses",fnc_nm);
  
  /* Arrays unroll into all longitudes for first latitude, then second latitude, ...
     Thus longitudes obtained by reading first block contiguously (unstrided)
     Latitudes obtained by reading unrolled data with stride of lon_nbr */
  dmn_srt[0]=0L;
  dmn_cnt[0]=rgr_map.dst_grid_size;
  rcd=nco_get_vara(in_id,area_dst_id,dmn_srt,dmn_cnt,area_out,crd_typ_out);
  if(flg_grd_out_1D){
    dmn_srt[0]=0L;
    dmn_cnt[0]=ncol_nbr_out;
    rcd=nco_get_vara(in_id,dst_grd_ctr_lon_id,dmn_srt,dmn_cnt,lon_ctr_out,crd_typ_out);
    dmn_srt[0]=0L;
    dmn_cnt[0]=ncol_nbr_out;
    rcd=nco_get_vara(in_id,dst_grd_ctr_lat_id,dmn_srt,dmn_cnt,lat_ctr_out,crd_typ_out);
    dmn_srt[0]=dmn_srt[1]=0L;
    dmn_cnt[0]=ncol_nbr_out;
    dmn_cnt[1]=bnd_nbr_out;
    rcd=nco_get_vara(in_id,dst_grd_crn_lon_id,dmn_srt,dmn_cnt,lon_bnd_out,crd_typ_out);
    dmn_srt[0]=dmn_srt[1]=0L;
    dmn_cnt[0]=ncol_nbr_out;
    dmn_cnt[1]=bnd_nbr_out;
    rcd=nco_get_vara(in_id,dst_grd_crn_lat_id,dmn_srt,dmn_cnt,lat_bnd_out,crd_typ_out);
    if(nco_dbg_lvl_get() >= nco_dbg_crr){
      for(idx=0;idx<lon_nbr_out;idx++){
	(void)fprintf(stdout,"lon[%li] = %g, vertices = ", idx,lon_ctr_out[idx]);
	for(int bnd_idx=0;bnd_idx<bnd_nbr_out;bnd_idx++)
	  (void)fprintf(stdout,"%s%g%s",bnd_idx == 0 ? "[" : "",lon_bnd_out[bnd_nbr_out*idx+bnd_idx],bnd_idx == bnd_nbr_out-1 ? "]\n" : ", ");
      } /* end loop over lon */
      for(idx=0;idx<lat_nbr_out;idx++){
	(void)fprintf(stdout,"lat[%li] = %g, vertices = ", idx,lat_ctr_out[idx]);
	for(int bnd_idx=0;bnd_idx<bnd_nbr_out;bnd_idx++)
	  (void)fprintf(stdout,"%s%g%s",bnd_idx == 0 ? "[" : "",lat_bnd_out[bnd_nbr_out*idx+bnd_idx],bnd_idx == bnd_nbr_out-1 ? "]\n" : ", ");
      } /* end loop over lat */
    } /* endif dbg */
  } /* !flg_grd_out_1D */

  if(flg_grd_out_2D){
    dmn_srt[0]=0L;
    dmn_cnt[0]=lon_nbr_out;
    rcd=nco_get_vara(in_id,dst_grd_ctr_lon_id,dmn_srt,dmn_cnt,lon_ctr_out,crd_typ_out);
    dmn_srt[0]=0L;
    dmn_cnt[0]=lat_nbr_out;
    dmn_srd[0]=lon_nbr_out;
    rcd=nco_get_vars(in_id,dst_grd_ctr_lat_id,dmn_srt,dmn_cnt,dmn_srd,lat_ctr_out,crd_typ_out);
    dmn_srt[0]=dmn_srt[1]=0L;
    dmn_cnt[0]=lon_nbr_out;
    dmn_cnt[1]=rgr_map.dst_grid_corners;
    rcd=nco_get_vara(in_id,dst_grd_crn_lon_id,dmn_srt,dmn_cnt,lon_crn_out,crd_typ_out);
    dmn_srt[0]=0L;
    dmn_cnt[0]=lat_nbr_out;
    dmn_srd[0]=lon_nbr_out;
    dmn_srt[1]=0L;
    dmn_cnt[1]=rgr_map.dst_grid_corners;
    dmn_srd[1]=1L;
    rcd=nco_get_vars(in_id,dst_grd_crn_lat_id,dmn_srt,dmn_cnt,dmn_srd,lat_crn_out,crd_typ_out);
  } /* !flg_grd_out_2D */
    
  /* Derive 2D interface boundaries from lat and lon grid-center values
     NB: Procedures to derive interfaces from midpoints on rectangular grids are theoretically possible 
     However, ESMF often outputs interfaces values (e.g., yv_b) for midpoint coordinates (e.g., yc_b)
     For example, ACME standard map from ne120np4 to 181x360 has yc_b[0] = yv_b[0] = -90.0
     Latitude = -90 is, by definition, not a midpoint coordinate
     This appears to be an artifact of the non-physical representation of the FV grid, i.e.,
     a grid center located at the pole where longitudes collapse in the model, but cannot be
     represented as collapsed on a rectangular 2D grid with non-zero areas.
     Unfortunately, ESMF supports this nonsense by labeling the grid center as at the pole
     so that applications can easily diagnose an FV grid when they read-in datasets.
     A superior application could diagnose FV just fine from actual non-polar gridcell centers
     Maybe ESMF could introduce a flag or something to indicate/avoid this special case?
     Safer to read boundary interfaces directly from grid corner/vertice arrays in map file
     
     Derivation of boundaries xv_b, yv_b from _correct_ xc_b, yc_b is follows
     Do not implement this procedure until resolving midpoint/center issue described above:
     lon_ntf_out[0]=0.5*(lon_ctr_out[0]+lon_ctr_out[lon_nbr_out-1])-180.0; // Extrapolation
     lat_ntf_out[0]=lat_ctr_out[0]-0.5*(lat_ctr_out[1]-lat_ctr_out[0]); // Extrapolation
     for(idx=1;idx<lon_nbr_out;idx++) lon_ntf_out[idx]=0.5*(lon_ctr_out[idx-1]+lon_ctr_out[idx]);
     for(idx=1;idx<lat_nbr_out;idx++) lat_ntf_out[idx]=0.5*(lat_ctr_out[idx-1]+lat_ctr_out[idx]);
     lon_ntf_out[lon_nbr_out]=lon_ntf_out[0]+360.0;
     lat_ntf_out[lat_nbr_out]=lat_ctr_out[lat_nbr_out-1]+0.5*(lat_ctr_out[lat_nbr_out-1]-lat_ctr_out[lat_nbr_out-2]); */

  if(flg_grd_out_2D){
    /* Obtain 1-D rectangular interfaces from unrolled 1-D vertice arrays */
    for(idx=0;idx<lon_nbr_out;idx++) lon_ntf_out[idx]=lon_crn_out[rgr_map.dst_grid_corners*idx];
    lon_ntf_out[lon_nbr_out]=lon_crn_out[rgr_map.dst_grid_corners*lon_nbr_out-(rgr_map.dst_grid_corners-1L)];
    for(idx=0;idx<lat_nbr_out;idx++) lat_ntf_out[idx]=lat_crn_out[rgr_map.dst_grid_corners*idx];
    lat_ntf_out[lat_nbr_out]=lat_crn_out[rgr_map.dst_grid_corners*lat_nbr_out-1L];
    
    /* Place 1-D rectangular interfaces into 2-D coordinate boundaries */
    for(idx=0;idx<lon_nbr_out;idx++){
      lon_bnd_out[2*idx]=lon_ntf_out[idx];
      lon_bnd_out[2*idx+1]=lon_ntf_out[idx+1];
    } /* end loop over longitude */
    for(idx=0;idx<lat_nbr_out;idx++){
      lat_bnd_out[2*idx]=lat_ntf_out[idx];
      lat_bnd_out[2*idx+1]=lat_ntf_out[idx+1];
    } /* end loop over latitude */
  } /* !flg_grd_out_2D */
  
  if(flg_grd_out_2D){

    if(nco_dbg_lvl_get() >= nco_dbg_crr){
      for(idx=0;idx<lon_nbr_out;idx++) (void)fprintf(stdout,"lon[%li] = [%g, %g, %g]\n",idx,lon_bnd_out[2*idx],lon_ctr_out[idx],lon_bnd_out[2*idx+1]);
      for(idx=0;idx<lat_nbr_out;idx++) (void)fprintf(stdout,"lat[%li] = [%g, %g, %g]\n",idx,lat_bnd_out[2*idx],lat_ctr_out[idx],lat_bnd_out[2*idx+1]);
    } /* endif dbg */

    /* Diagnose type of two-dimensional output grid by testing second latitude center against formulae */
    nco_grd_2D_typ_enm nco_grd_2D_typ=nco_grd_2D_nil; /* [enm] Two-dimensional grid-type enum */
    const double lat_ctr_tst_ngl_eqi_pol=-90.0+180.0/(lat_nbr_out-1);
    const double lat_ctr_tst_ngl_eqi_fst=-90.0+180.0*1.5/lat_nbr_out;
    double lat_ctr_tst_gss;
    /* In diagnosing grids, agreement with input to single-precision is "good enough for government work"
       Hence some comparisons cast from double to float before comparison
       20150526: T42 grid from SCRIP and related maps
       20150611: map_ne120np4_to_fv801x1600_bilin.150418.nc has yc_b[1600]=-89.775000006 not expected exact value lat_ctr[1]=-89.775000000000006 */
    if((float)lat_ctr_out[1] == (float)lat_ctr_tst_ngl_eqi_fst) nco_grd_2D_typ=nco_grd_2D_ngl_eqi_fst;
    if((float)lat_ctr_out[1] == (float)lat_ctr_tst_ngl_eqi_pol) nco_grd_2D_typ=nco_grd_2D_ngl_eqi_pol;
    double *wgt_Gss_out=NULL; // [frc] Gaussian weights double precision
    if(nco_grd_2D_typ == nco_grd_2D_nil){
      /* Check for Gaussian grid */
      const double rdn2dgr=180.0/M_PI;
      double *lat_sin_out; // [frc] Sine of Gaussian latitudes double precision
      lat_sin_out=(double *)nco_malloc(lat_nbr_out*sizeof(double));
      wgt_Gss_out=(double *)nco_malloc(lat_nbr_out*sizeof(double));
      (void)nco_lat_wgt_gss(lat_nbr_out,lat_sin_out,wgt_Gss_out);
      lat_ctr_tst_gss=rdn2dgr*asin(lat_sin_out[1]);
      /* Gaussian weights on output grid will be double-precision accurate
	 Grid itself is kept as user-specified so area diagnosed by ESMF_RegridWeightGen may be slightly inconsistent with weights */
      if((float)lat_ctr_out[1] == (float)lat_ctr_tst_gss) nco_grd_2D_typ=nco_grd_2D_gss;
      if(lat_sin_out) lat_sin_out=(double *)nco_free(lat_sin_out);
    } /* !Gaussian */
    if(nco_dbg_lvl_get() >= nco_dbg_std) (void)fprintf(stderr,"%s: INFO %s diagnosed output latitude grid-type: %s\n",nco_prg_nm_get(),fnc_nm,nco_grd_2D_sng(nco_grd_2D_typ));
    
    const double dgr2rdn=M_PI/180.0;
    switch(nco_grd_2D_typ){
    case nco_grd_2D_ngl_eqi_fst:
      for(idx=0;idx<lat_nbr_out;idx++) lat_wgt_out[idx]=cos(dgr2rdn*lat_ctr_out[idx]);
      break;
    case nco_grd_2D_ngl_eqi_pol:
      for(idx=0;idx<lat_nbr_out;idx++) lat_wgt_out[idx]=sin(dgr2rdn*lat_bnd_out[2*idx+1])-sin(dgr2rdn*lat_bnd_out[2*idx]);
      break;
    case nco_grd_2D_gss:
      for(idx=0;idx<lat_nbr_out;idx++) lat_wgt_out[idx]=wgt_Gss_out[idx];
      break;
      if(wgt_Gss_out) wgt_Gss_out=(double *)nco_free(wgt_Gss_out);
    default:
      (void)fprintf(stderr,"%s: ERROR %s unknown output latitude grid-type\n",nco_prg_nm_get(),fnc_nm);
      nco_dfl_case_generic_err(); break;
    } /* end nco_grd_2D_typ switch */
    
    /* Fuzzy test of latitude weight normalization */
    const double eps_wgt_ttl=1.0e-14; /* [frc] Roundoff tolerance for sum of quadrature weights */
    double lat_wgt_ttl=0.0; /* [frc] Actual sum of quadrature weights */
    for(idx=0;idx<lat_nbr_out;idx++) lat_wgt_ttl+=lat_wgt_out[idx];
    assert(1.0-lat_wgt_ttl/2.0 < eps_wgt_ttl);
    
    if(nco_dbg_lvl_get() >= nco_dbg_sbr){
      (void)fprintf(stderr,"%s: INFO %s reports destination rectangular latitude grid:\n",nco_prg_nm_get(),fnc_nm);
      double area_out_ttl=0.0; /* [frc] Exact sum of area */
      lat_wgt_ttl=0.0;
      area_out_ttl=0.0;
      for(idx=0;idx<lat_nbr_out;idx++)
	lat_wgt_ttl+=lat_wgt_out[idx];
      for(long int lat_idx=0;lat_idx<lat_nbr_out;lat_idx++)
	for(long int lon_idx=0;lon_idx<lon_nbr_out;lon_idx++)
	  area_out_ttl+=area_out[lat_idx*lon_nbr_out+lon_idx];
      (void)fprintf(stdout,"lat_wgt_ttl = %20.15f, area_ttl = %20.15f\n",lat_wgt_ttl,area_out_ttl);
      for(idx=0;idx<lon_nbr_out;idx++) (void)fprintf(stdout,"lon[%li] = [%g, %g, %g]\n",idx,lon_bnd_out[2*idx],lon_ctr_out[idx],lon_bnd_out[2*idx+1]);
      for(idx=0;idx<lat_nbr_out;idx++) (void)fprintf(stdout,"lat[%li] = [%g, %g, %g]\n",idx,lat_bnd_out[2*idx],lat_ctr_out[idx],lat_bnd_out[2*idx+1]);
      for(idx=0;idx<lat_nbr_out;idx++) (void)fprintf(stdout,"lat[%li], wgt[%li] = %20.15f, %20.15f\n",idx,idx,lat_ctr_out[idx],lat_wgt_out[idx]);
      if(nco_dbg_lvl_get() > nco_dbg_crr)
	for(long int lat_idx=0;lat_idx<lat_nbr_out;lat_idx++)
	  for(long int lon_idx=0;lon_idx<lon_nbr_out;lon_idx++)
	    (void)fprintf(stdout,"lat[%li] = %g, lon[%li] = %g, area[%li,%li] = %g\n",lat_idx,lat_ctr_out[lat_idx],lon_idx,lon_ctr_out[lon_idx],lat_idx,lon_idx,area_out[lat_idx*lon_nbr_out+lon_idx]);
    } /* endif dbg */
  } /* !flg_grd_out_2D */

  /* Obtain remap matrix addresses and weights from map file */
  dmn_srt[0]=0L;
  dmn_cnt[0]=rgr_map.num_links;
  rcd=nco_get_vara(in_id,wgt_raw_id,dmn_srt,dmn_cnt,wgt_raw,NC_DOUBLE);
  rcd=nco_get_vara(in_id,col_src_adr_id,dmn_srt,dmn_cnt,col_src_adr,NC_INT);
  rcd=nco_get_vara(in_id,row_dst_adr_id,dmn_srt,dmn_cnt,row_dst_adr,NC_INT);
  /* Optimize row/column access by pre-subtracting one to account for Fortran index offset relative to C */
  size_t lnk_nbr; /* [nbr] Number of links */
  size_t lnk_idx; /* [idx] Link index */
  lnk_nbr=rgr_map.num_links;
  for(lnk_idx=0;lnk_idx<lnk_nbr;lnk_idx++) row_dst_adr[lnk_idx]--;
  for(lnk_idx=0;lnk_idx<lnk_nbr;lnk_idx++) col_src_adr[lnk_idx]--;
  if(nco_dbg_lvl_get() >= nco_dbg_io){
    (void)fprintf(stdout,"idx row_dst col_src wgt_raw\n");
    for(lnk_idx=0;lnk_idx<lnk_nbr;lnk_idx++) (void)fprintf(stdout,"%li %d %d %g\n",lnk_idx,row_dst_adr[lnk_idx],col_src_adr[lnk_idx],wgt_raw[lnk_idx]);
  } /* endif dbg */

  /* Free memory associated with input file */
  if(dmn_srt) dmn_srt=(long *)nco_free(dmn_srt);
  if(dmn_cnt) dmn_cnt=(long *)nco_free(dmn_cnt);
  if(dmn_srd) dmn_srd=(long *)nco_free(dmn_srd);

  /* Close input netCDF file */
  nco_close(in_id);

  /* Remove local copy of file */
  if(FL_RTR_RMT_LCN && RM_RMT_FL_PST_PRC) (void)nco_fl_rm(fl_in);

  /* Above this line, fl_in and in_id refer to map file
     Below this line, fl_in and in_id refer to input file to be regridded */

  /* Initialize */
  in_id=rgr->in_id;
  out_id=rgr->out_id;

  /* Do not extract extensive variables like lon, lat, and area
     If necessary, create them from scratch from remap data
     I decided not to include "date_written" and "time_written" in this list, this is debatable... */
  const int var_xcl_lst_nbr=13; /* [nbr] Number of objects on exclusion list */
  const char *var_xcl_lst[]={"/area","/gridcell_area","/gw","/lat","/latitude","/lat_bnds","/lat_vertices","/bounds_lat","/lon","/longitude","/lon_bnds","/lon_vertices","/bounds_lon"};
  int var_cpy_nbr=0; /* [nbr] Number of copied variables */
  int var_rgr_nbr=0; /* [nbr] Number of regridded variables */
  int var_xcl_nbr=0; /* [nbr] Number of deleted variables */
  int var_crt_nbr=0; /* [nbr] Number of created variables */
  unsigned int idx_tbl; /* [idx] Counter for traversal table */
  const unsigned int trv_nbr=trv_tbl->nbr; /* [idx] Number of traversal table entries */
  for(idx=0;idx<var_xcl_lst_nbr;idx++){
    for(idx_tbl=0;idx_tbl<trv_nbr;idx_tbl++)
      if(!strcmp(trv_tbl->lst[idx_tbl].nm_fll,var_xcl_lst[idx])) break;
    if(idx_tbl < trv_nbr){
      if(trv_tbl->lst[idx_tbl].flg_xtr){
	if(nco_dbg_lvl_get() >= nco_dbg_var) (void)fprintf(stdout,"%s: INFO automatically omitting (not copying or regridding from input) variable %s\n",nco_prg_nm_get(),trv_tbl->lst[idx_tbl].nm_fll);
	var_xcl_nbr++;
      } /* endif */
      trv_tbl->lst[idx_tbl].flg_xtr=False;
    } /* endif */
  } /* end loop */
  
  char *col_nm=rgr->col_nm; /* [sng] Name of horizontal spatial dimension on unstructured grid */
  char *lat_nm=rgr->lat_nm; /* [sng] Name of dimension to recognize as latitude */
  char *lon_nm=rgr->lon_nm; /* [sng] Name of dimension to recognize as longitude */
  char *dmn_nm_cp; /* [sng] Dimension name as char * to reduce indirection */
  int dmn_idx; /* [idx] Dimension index */
  int dmn_nbr_in; /* [nbr] Number of dimensions in input variable */
  int dmn_nbr_out; /* [nbr] Number of dimensions in output variable */
  trv_sct trv; /* [sct] Traversal table object structure to reduce indirection */
  /* Define regridding flag for each variable */
  for(idx_tbl=0;idx_tbl<trv_nbr;idx_tbl++){
    trv=trv_tbl->lst[idx_tbl];
    dmn_nbr_in=trv_tbl->lst[idx_tbl].nbr_dmn;
    if(trv.nco_typ == nco_obj_typ_var && trv.flg_xtr){
      for(dmn_idx=0;dmn_idx<dmn_nbr_in;dmn_idx++){
	dmn_nm_cp=trv.var_dmn[dmn_idx].dmn_nm;
	/* Regrid variables containing a horizontal spatial dimension */
	/* fxm: generalize to include any variable containing a dimension in a coordinate with "standard_name" = "latitude" or "longitude" */
	if(!strcmp(dmn_nm_cp,col_nm) || !strcmp(dmn_nm_cp,lat_nm) || !strcmp(dmn_nm_cp,lon_nm)){
	  trv_tbl->lst[idx_tbl].flg_rgr=True;
	  var_rgr_nbr++;
	  break;
	} /* endif */
      } /* end loop over dimensions */
      if(dmn_idx == dmn_nbr_in) var_cpy_nbr++;
    } /* end nco_obj_typ_var */
  } /* end idx_tbl */
  if(!var_rgr_nbr) (void)fprintf(stdout,"%s: WARNING %s reports no variables fit regridding criteria\n",nco_prg_nm_get(),fnc_nm);
  
  if(nco_dbg_lvl_get() >= nco_dbg_sbr){
    for(idx_tbl=0;idx_tbl<trv_nbr;idx_tbl++){
      trv=trv_tbl->lst[idx_tbl];
      if(trv.nco_typ == nco_obj_typ_var && trv.flg_xtr) (void)fprintf(stderr,"Regrid %s? %s\n",trv.nm,trv.flg_rgr ? "Yes" : "No");
    } /* end idx_tbl */
  } /* end dbg */

  /* Layout regridded file */
  aed_sct aed_mtd;
  char *area_nm_out;
  char *att_nm;
  char *bnd_nm_out;
  char *bnd_tm_nm_out;
  char *col_nm_out;
  char *lat_bnd_nm_out;
  char *lat_nm_out;
  char *lat_wgt_nm;
  char *lon_bnd_nm_out;
  char *lon_nm_out;
  int dmn_id_lat; /* [id] Dimension ID */
  int dmn_id_ncol; /* [id] Dimension ID */
  int dmn_id_lon; /* [id] Dimension ID */
  int dmn_id_bnd; /* [id] Dimension ID */
  int dmn_id_bnd_tm; /* [id] Dimension ID */
  int area_out_id; /* [id] Variable ID for area */
  int lon_out_id; /* [id] Variable ID for longitude */
  int lat_out_id; /* [id] Variable ID for latitude */
  int lat_wgt_id; /* [id] Variable ID for latitude weight */
  int lon_bnd_id; /* [id] Variable ID for lon_bnds/lon_vertices */
  int lat_bnd_id; /* [id] Variable ID for lat_bnds/lat_vertices */
  int dmn_ids_out[dmn_nbr_grd_max]; /* [id] Dimension IDs array for output variable */
  long dmn_srt_out[dmn_nbr_grd_max];
  long dmn_cnt_out[dmn_nbr_grd_max];

  /* Name output dimensions */
  area_nm_out=rgr->area_nm;
  bnd_nm_out=rgr->bnd_nm;
  bnd_tm_nm_out=rgr->bnd_tm_nm;
  col_nm_out=rgr->col_nm;
  lat_bnd_nm_out=rgr->lat_bnd_nm;
  lat_nm_out=rgr->lat_nm;
  lat_wgt_nm=rgr->lat_wgt_nm;
  lon_bnd_nm_out=rgr->lon_bnd_nm;
  lon_nm_out=rgr->lon_nm;
  if(flg_grd_out_1D){
    bnd_nm_out=rgr->vrt_nm;
    lat_bnd_nm_out=rgr->lat_vrt_nm;
    lon_bnd_nm_out=rgr->lon_vrt_nm;
  } /* !flg_grd_out_1D */
  if(flg_grd_out_2D){
    bnd_nm_out=rgr->bnd_nm;
    lat_bnd_nm_out=rgr->lat_bnd_nm;
    lon_bnd_nm_out=rgr->lon_bnd_nm;
  } /* !flg_grd_out_2D */

  /* Persistent metadata */
  aed_sct aed_mtd_crd;
  char *att_val_crd=NULL;
  char *att_nm_crd=NULL;
  att_nm_crd=strdup("coordinates");
  aed_mtd_crd.att_nm=att_nm_crd;
  if(flg_grd_out_1D) aed_mtd_crd.mode=aed_overwrite; else aed_mtd_crd.mode=aed_delete;
  aed_mtd_crd.type=NC_CHAR;
  aed_mtd_crd.sz=strlen(lat_nm_out)+strlen(lon_nm_out)+1L;
  att_val_crd=(char *)nco_malloc(aed_mtd_crd.sz*nco_typ_lng(aed_mtd_crd.type)+1L);
  (void)sprintf(att_val_crd,"%s %s",lat_nm_out,lon_nm_out);
  aed_mtd_crd.val.cp=att_val_crd;

  /* Define new horizontal dimensions before all else */
  if(flg_grd_out_1D){
    rcd=nco_def_dim(out_id,col_nm_out,ncol_nbr_out,&dmn_id_ncol);
  } /* !flg_grd_out_1D */
  if(flg_grd_out_2D){
    rcd=nco_def_dim(out_id,lat_nm_out,lat_nbr_out,&dmn_id_lat);
    rcd=nco_def_dim(out_id,lon_nm_out,lon_nbr_out,&dmn_id_lon);
  } /* !flg_grd_out_2D */
  rcd=nco_inq_dimid_flg(out_id,bnd_tm_nm_out,&dmn_id_bnd_tm);
  /* If dimension has not been defined, define it */
  if(rcd != NC_NOERR) rcd=nco_def_dim(out_id,bnd_tm_nm_out,bnd_tm_nbr_out,&dmn_id_bnd_tm);
  rcd=nco_inq_dimid_flg(out_id,bnd_nm_out,&dmn_id_bnd);
  /* If dimension has not been defined, define it */
  if(rcd != NC_NOERR) rcd=nco_def_dim(out_id,bnd_nm_out,bnd_nbr_out,&dmn_id_bnd);

  char dmn_nm[NC_MAX_NAME]; /* [sng] Dimension name */
  char *var_nm; /* [sng] Variable name */
  int *dmn_id_in; /* [id] Dimension IDs */
  int *dmn_id_out; /* [id] Dimension IDs */
  int var_id_in; /* [id] Variable ID */
  int var_id_out; /* [id] Variable ID */
  nc_type var_typ; /* [enm] Variable type */
  nco_bool PCK_ATT_CPY=True; /* [flg] Copy attributes "scale_factor", "add_offset" */

  /* Define new coordinates and variables in regridded file */
  if(flg_grd_out_1D){
    (void)nco_def_var(out_id,lat_nm_out,crd_typ_out,dmn_nbr_1D,&dmn_id_ncol,&lat_out_id);
    var_crt_nbr++;
    (void)nco_def_var(out_id,lon_nm_out,crd_typ_out,dmn_nbr_1D,&dmn_id_ncol,&lon_out_id);
    var_crt_nbr++;
    dmn_ids_out[0]=dmn_id_ncol;
    dmn_ids_out[1]=dmn_id_bnd;
    (void)nco_def_var(out_id,lat_bnd_nm_out,crd_typ_out,dmn_nbr_2D,dmn_ids_out,&lat_bnd_id);
    var_crt_nbr++;
    dmn_ids_out[0]=dmn_id_ncol;
    dmn_ids_out[1]=dmn_id_bnd;
    (void)nco_def_var(out_id,lon_bnd_nm_out,crd_typ_out,dmn_nbr_2D,dmn_ids_out,&lon_bnd_id);
    var_crt_nbr++;
    (void)nco_def_var(out_id,area_nm_out,crd_typ_out,dmn_nbr_1D,&dmn_id_ncol,&area_out_id);
    var_crt_nbr++;
  } /* !flg_grd_out_1D */
  if(flg_grd_out_2D){
    (void)nco_def_var(out_id,lat_nm_out,crd_typ_out,dmn_nbr_1D,&dmn_id_lat,&lat_out_id);
    var_crt_nbr++;
    (void)nco_def_var(out_id,lon_nm_out,crd_typ_out,dmn_nbr_1D,&dmn_id_lon,&lon_out_id);
    var_crt_nbr++;
    dmn_ids_out[0]=dmn_id_lat;
    dmn_ids_out[1]=dmn_id_bnd;
    (void)nco_def_var(out_id,lat_bnd_nm_out,crd_typ_out,dmn_nbr_2D,dmn_ids_out,&lat_bnd_id);
    var_crt_nbr++;
    dmn_ids_out[0]=dmn_id_lon;
    dmn_ids_out[1]=dmn_id_bnd;
    (void)nco_def_var(out_id,lon_bnd_nm_out,crd_typ_out,dmn_nbr_2D,dmn_ids_out,&lon_bnd_id);
    var_crt_nbr++;
    (void)nco_def_var(out_id,lat_wgt_nm,crd_typ_out,dmn_nbr_1D,&dmn_id_lat,&lat_wgt_id);
    var_crt_nbr++;
    dmn_ids_out[0]=dmn_id_lat;
    dmn_ids_out[1]=dmn_id_lon;
    (void)nco_def_var(out_id,area_nm_out,crd_typ_out,dmn_nbr_2D,dmn_ids_out,&area_out_id);
    var_crt_nbr++;
  } /* !flg_grd_out_2D */

  /* Define regridded variables in output file */
  for(idx_tbl=0;idx_tbl<trv_nbr;idx_tbl++){
    trv=trv_tbl->lst[idx_tbl];
    if(trv.nco_typ == nco_obj_typ_var && trv.flg_xtr){
      var_nm=trv.nm;
      var_typ=trv.var_typ;
      dmn_nbr_in=trv.nbr_dmn;
      dmn_nbr_out=trv.nbr_dmn;
      rcd=nco_inq_varid(in_id,var_nm,&var_id_in);
      rcd=nco_inq_varid_flg(out_id,var_nm,&var_id_out);
      /* If variable has not been defined, define it */
      if(rcd != NC_NOERR){
	dmn_id_in=(int *)nco_malloc((dmn_nbr_in+1)*sizeof(int)); /* Allocate an extra slot in case additional dimension in output */
	dmn_id_out=(int *)nco_malloc((dmn_nbr_in+1)*sizeof(int));
	dmn_srt=(long *)nco_malloc((dmn_nbr_in+1)*sizeof(long));
	dmn_cnt=(long *)nco_malloc((dmn_nbr_in+1)*sizeof(long));
	if(trv.flg_rgr){
	  /* Regrid */
	  rcd=nco_inq_vardimid(in_id,var_id_in,dmn_id_in);
	  for(dmn_idx=0;dmn_idx<dmn_nbr_in;dmn_idx++){
	    rcd=nco_inq_dimname(in_id,dmn_id_in[dmn_idx],dmn_nm);
	    if(flg_grd_out_1D){
	      if(nco_rgr_grd_typ == nco_rgr_grd_2D_to_1D && (!strcmp(dmn_nm,lat_nm) || !strcmp(dmn_nm,lon_nm))){
		/* Replace orthogonal horizontal dimensions by unstructured horizontal dimension already defined */
		if(!strcmp(dmn_nm,lat_nm)){
		  dmn_id_out[dmn_idx]=dmn_id_ncol;
		  dmn_cnt[dmn_idx]=ncol_nbr_out;
		} /* endif lat */
		if(!strcmp(dmn_nm,lon_nm)){
		  dmn_id_out[dmn_idx]=-999;
		  dmn_cnt[dmn_idx]=-999;
		  dmn_nbr_out--;
		} /* endif lon */
	      }else{
		/* Always copy existing horizontal dimensions for 1D->1D */
		rcd=nco_inq_dimid_flg(out_id,dmn_nm,dmn_id_out+dmn_idx);
		/* If dimension has not been defined, define it */
		if(rcd != NC_NOERR){
		  rcd=nco_inq_dimlen(in_id,dmn_id_in[dmn_idx],dmn_cnt+dmn_idx);
		  rcd=nco_def_dim(out_id,dmn_nm,dmn_cnt[dmn_idx],dmn_id_out+dmn_idx);
		} /* !rcd */
	      } /* !lat && !lon */
	    } /* !2D_to_1D */
	    if(flg_grd_out_2D){
	      if(nco_rgr_grd_typ == nco_rgr_grd_1D_to_2D && !strcmp(dmn_nm,col_nm)){
		/* Replace unstructured horizontal dimension by orthogonal horizontal dimensions already defined */
		dmn_id_out[dmn_idx]=dmn_id_lat;
		dmn_id_out[dmn_idx+1]=dmn_id_lon;
		dmn_cnt[dmn_idx]=lat_nbr_out;
		dmn_cnt[dmn_idx+1]=lon_nbr_out;
		dmn_idx++;
		dmn_nbr_out++;
	      }else{
		/* Always copy existing horizontal dimensions for 2D->2D */
		rcd=nco_inq_dimid_flg(out_id,dmn_nm,dmn_id_out+dmn_idx);
		/* If dimension has not been defined, define it */
		if(rcd != NC_NOERR){
		  rcd=nco_inq_dimlen(in_id,dmn_id_in[dmn_idx],dmn_cnt+dmn_idx);
		  rcd=nco_def_dim(out_id,dmn_nm,dmn_cnt[dmn_idx],dmn_id_out+dmn_idx);
		} /* !rcd */
	      } /* !ncol */
	    } /* !1D_to_2D */
	  } /* end loop over dimensions */
	}else{ /* !flg_rgr */
	  /* Copy as-is */
	  rcd=nco_inq_vardimid(in_id,var_id_in,dmn_id_in);
	  for(dmn_idx=0;dmn_idx<dmn_nbr_in;dmn_idx++){
	    rcd=nco_inq_dimname(in_id,dmn_id_in[dmn_idx],dmn_nm);
	    rcd=nco_inq_dimid_flg(out_id,dmn_nm,dmn_id_out+dmn_idx);
	    /* If dimension has not been defined, define it */
	    if(rcd != NC_NOERR){
	      rcd=nco_inq_dimlen(in_id,dmn_id_in[dmn_idx],dmn_cnt+dmn_idx);
	      rcd=nco_def_dim(out_id,dmn_nm,dmn_cnt[dmn_idx],dmn_id_out+dmn_idx);
	    } /* !rcd */
	  } /* end loop over dimensions */
	} /* end !flg_rgr */
	rcd=nco_def_var(out_id,var_nm,var_typ,dmn_nbr_out,dmn_id_out,&var_id_out);
	(void)nco_att_cpy(in_id,out_id,var_id_in,var_id_out,PCK_ATT_CPY);
	if(trv.flg_rgr){
	  aed_mtd_crd.var_nm=var_nm;
	  aed_mtd_crd.id=var_id_out;
	  (void)nco_aed_prc(out_id,var_id_out,aed_mtd_crd);
	} /* !flg_rgr */
	if(dmn_id_in) dmn_id_in=(int *)nco_free(dmn_id_in);
	if(dmn_id_out) dmn_id_out=(int *)nco_free(dmn_id_out);
	if(dmn_srt) dmn_srt=(long *)nco_free(dmn_srt);
	if(dmn_cnt) dmn_cnt=(long *)nco_free(dmn_cnt);
      } /* !rcd */
    } /* !var */
  } /* end idx_tbl */

  /* Define new metadata in regridded file */
  att_nm=strdup("long_name");
  att_val=strdup("solid angle subtended by grid cell");
  aed_mtd.att_nm=att_nm;
  aed_mtd.var_nm=area_nm_out;
  aed_mtd.id=area_out_id;
  aed_mtd.sz=strlen(att_val);
  aed_mtd.type=NC_CHAR;
  aed_mtd.val.cp=att_val;
  aed_mtd.mode=aed_create;
  (void)nco_aed_prc(out_id,area_out_id,aed_mtd);
  if(att_nm) att_nm=(char *)nco_free(att_nm);
  if(att_val) att_val=(char *)nco_free(att_val);

  att_nm=strdup("standard_name");
  att_val=strdup("cell_area");
  aed_mtd.att_nm=att_nm;
  aed_mtd.var_nm=area_nm_out;
  aed_mtd.id=area_out_id;
  aed_mtd.sz=strlen(att_val);
  aed_mtd.type=NC_CHAR;
  aed_mtd.val.cp=att_val;
  aed_mtd.mode=aed_create;
  (void)nco_aed_prc(out_id,area_out_id,aed_mtd);
  if(att_nm) att_nm=(char *)nco_free(att_nm);
  if(att_val) att_val=(char *)nco_free(att_val);

  att_nm=strdup("units");
  att_val=strdup("steradian");
  aed_mtd.att_nm=att_nm;
  aed_mtd.var_nm=area_nm_out;
  aed_mtd.id=area_out_id;
  aed_mtd.sz=strlen(att_val);
  aed_mtd.type=NC_CHAR;
  aed_mtd.val.cp=att_val;
  aed_mtd.mode=aed_create;
  (void)nco_aed_prc(out_id,area_out_id,aed_mtd);
  if(att_nm) att_nm=(char *)nco_free(att_nm);
  if(att_val) att_val=(char *)nco_free(att_val);

  att_nm=strdup("cell_methods");
  att_val=strdup("lat, lon: sum");
  aed_mtd.att_nm=att_nm;
  aed_mtd.var_nm=area_nm_out;
  aed_mtd.id=area_out_id;
  aed_mtd.sz=strlen(att_val);
  aed_mtd.type=NC_CHAR;
  aed_mtd.val.cp=att_val;
  aed_mtd.mode=aed_create;
  (void)nco_aed_prc(out_id,area_out_id,aed_mtd);
  if(att_nm) att_nm=(char *)nco_free(att_nm);
  if(att_val) att_val=(char *)nco_free(att_val);

  att_nm=strdup("long_name");
  att_val=strdup("latitude");
  aed_mtd.att_nm=att_nm;
  aed_mtd.var_nm=lat_nm_out;
  aed_mtd.id=lat_out_id;
  aed_mtd.sz=strlen(att_val);
  aed_mtd.type=NC_CHAR;
  aed_mtd.val.cp=att_val;
  aed_mtd.mode=aed_create;
  (void)nco_aed_prc(out_id,lat_out_id,aed_mtd);
  if(att_nm) att_nm=(char *)nco_free(att_nm);
  if(att_val) att_val=(char *)nco_free(att_val);

  att_nm=strdup("standard_name");
  att_val=strdup("latitude");
  aed_mtd.att_nm=att_nm;
  aed_mtd.var_nm=lat_nm_out;
  aed_mtd.id=lat_out_id;
  aed_mtd.sz=strlen(att_val);
  aed_mtd.type=NC_CHAR;
  aed_mtd.val.cp=att_val;
  aed_mtd.mode=aed_create;
  (void)nco_aed_prc(out_id,lat_out_id,aed_mtd);
  if(att_nm) att_nm=(char *)nco_free(att_nm);
  if(att_val) att_val=(char *)nco_free(att_val);

  att_nm=strdup("units");
  att_val=strdup("degrees_north");
  aed_mtd.att_nm=att_nm;
  aed_mtd.var_nm=lat_nm_out;
  aed_mtd.id=lat_out_id;
  aed_mtd.sz=strlen(att_val);
  aed_mtd.type=NC_CHAR;
  aed_mtd.val.cp=att_val;
  aed_mtd.mode=aed_create;
  (void)nco_aed_prc(out_id,lat_out_id,aed_mtd);
  if(att_nm) att_nm=(char *)nco_free(att_nm);
  if(att_val) att_val=(char *)nco_free(att_val);

  att_nm=strdup("axis");
  att_val=strdup("Y");
  aed_mtd.att_nm=att_nm;
  aed_mtd.var_nm=lat_nm_out;
  aed_mtd.id=lat_out_id;
  aed_mtd.sz=strlen(att_val);
  aed_mtd.type=NC_CHAR;
  aed_mtd.val.cp=att_val;
  aed_mtd.mode=aed_create;
  (void)nco_aed_prc(out_id,lat_out_id,aed_mtd);
  if(att_nm) att_nm=(char *)nco_free(att_nm);
  if(att_val) att_val=(char *)nco_free(att_val);

  att_nm=strdup("bounds");
  att_val=lat_bnd_nm_out;
  aed_mtd.att_nm=att_nm;
  aed_mtd.var_nm=lat_nm_out;
  aed_mtd.id=lat_out_id;
  aed_mtd.sz=strlen(att_val);
  aed_mtd.type=NC_CHAR;
  aed_mtd.val.cp=att_val;
  aed_mtd.mode=aed_create;
  (void)nco_aed_prc(out_id,lat_out_id,aed_mtd);
  if(att_nm) att_nm=(char *)nco_free(att_nm);

  att_nm=strdup("long_name");
  if(flg_grd_out_2D) att_val=strdup("gridcell latitude interfaces"); else att_val=strdup("gridcell latitude vertices");
  aed_mtd.att_nm=att_nm;
  aed_mtd.var_nm=lat_bnd_nm_out;
  aed_mtd.id=lat_bnd_id;
  aed_mtd.sz=strlen(att_val);
  aed_mtd.type=NC_CHAR;
  aed_mtd.val.cp=att_val;
  aed_mtd.mode=aed_create;
  (void)nco_aed_prc(out_id,lat_bnd_id,aed_mtd);
  if(att_nm) att_nm=(char *)nco_free(att_nm);
  if(att_val) att_val=(char *)nco_free(att_val);

  att_nm=strdup("long_name");
  att_val=strdup("longitude");
  aed_mtd.att_nm=att_nm;
  aed_mtd.var_nm=lon_nm_out;
  aed_mtd.id=lon_out_id;
  aed_mtd.sz=strlen(att_val);
  aed_mtd.type=NC_CHAR;
  aed_mtd.val.cp=att_val;
  aed_mtd.mode=aed_create;
  (void)nco_aed_prc(out_id,lon_out_id,aed_mtd);
  if(att_nm) att_nm=(char *)nco_free(att_nm);
  if(att_val) att_val=(char *)nco_free(att_val);

  att_nm=strdup("standard_name");
  att_val=strdup("longitude");
  aed_mtd.att_nm=att_nm;
  aed_mtd.var_nm=lon_nm_out;
  aed_mtd.id=lon_out_id;
  aed_mtd.sz=strlen(att_val);
  aed_mtd.type=NC_CHAR;
  aed_mtd.val.cp=att_val;
  aed_mtd.mode=aed_create;
  (void)nco_aed_prc(out_id,lon_out_id,aed_mtd);
  if(att_nm) att_nm=(char *)nco_free(att_nm);
  if(att_val) att_val=(char *)nco_free(att_val);

  att_nm=strdup("units");
  att_val=strdup("degrees_east");
  aed_mtd.att_nm=att_nm;
  aed_mtd.var_nm=lon_nm_out;
  aed_mtd.id=lon_out_id;
  aed_mtd.sz=strlen(att_val);
  aed_mtd.type=NC_CHAR;
  aed_mtd.val.cp=att_val;
  aed_mtd.mode=aed_create;
  (void)nco_aed_prc(out_id,lon_out_id,aed_mtd);
  if(att_nm) att_nm=(char *)nco_free(att_nm);
  if(att_val) att_val=(char *)nco_free(att_val);

  att_nm=strdup("axis");
  att_val=strdup("X");
  aed_mtd.att_nm=att_nm;
  aed_mtd.var_nm=lon_nm_out;
  aed_mtd.id=lon_out_id;
  aed_mtd.sz=strlen(att_val);
  aed_mtd.type=NC_CHAR;
  aed_mtd.val.cp=att_val;
  aed_mtd.mode=aed_create;
  (void)nco_aed_prc(out_id,lon_out_id,aed_mtd);
  if(att_nm) att_nm=(char *)nco_free(att_nm);
  if(att_val) att_val=(char *)nco_free(att_val);

  att_nm=strdup("bounds");
  att_val=lon_bnd_nm_out;
  aed_mtd.att_nm=att_nm;
  aed_mtd.var_nm=lon_nm_out;
  aed_mtd.id=lon_out_id;
  aed_mtd.sz=strlen(att_val);
  aed_mtd.type=NC_CHAR;
  aed_mtd.val.cp=att_val;
  aed_mtd.mode=aed_create;
  (void)nco_aed_prc(out_id,lon_out_id,aed_mtd);
  if(att_nm) att_nm=(char *)nco_free(att_nm);

  att_nm=strdup("long_name");
  if(flg_grd_out_2D) att_val=strdup("gridcell longitude interfaces"); else att_val=strdup("gridcell longitude vertices");
  aed_mtd.att_nm=att_nm;
  aed_mtd.var_nm=lon_bnd_nm_out;
  aed_mtd.id=lon_bnd_id;
  aed_mtd.sz=strlen(att_val);
  aed_mtd.type=NC_CHAR;
  aed_mtd.val.cp=att_val;
  aed_mtd.mode=aed_create;
  (void)nco_aed_prc(out_id,lon_bnd_id,aed_mtd);
  if(att_nm) att_nm=(char *)nco_free(att_nm);
  if(att_val) att_val=(char *)nco_free(att_val);

  if(flg_grd_out_2D){
    att_nm=strdup("long_name");
    att_val=strdup("latitude quadrature weights (normalized to sum to 2.0)");
    aed_mtd.att_nm=att_nm;
    aed_mtd.var_nm=lat_wgt_nm;
    aed_mtd.id=lat_wgt_id;
    aed_mtd.sz=strlen(att_val);
    aed_mtd.type=NC_CHAR;
    aed_mtd.val.cp=att_val;
    aed_mtd.mode=aed_create;
    (void)nco_aed_prc(out_id,lat_wgt_id,aed_mtd);
    if(att_nm) att_nm=(char *)nco_free(att_nm);
    if(att_val) att_val=(char *)nco_free(att_val);
  } /* !flg_grd_out_2D */
  
  att_nm=strdup("mapping_file");
  att_val=strdup(fl_in);
  aed_mtd.att_nm=att_nm;
  aed_mtd.var_nm=NULL;
  aed_mtd.id=NC_GLOBAL;
  aed_mtd.sz=strlen(att_val);
  aed_mtd.type=NC_CHAR;
  aed_mtd.val.cp=att_val;
  aed_mtd.mode=aed_create;
  (void)nco_aed_prc(out_id,NC_GLOBAL,aed_mtd);
  if(att_nm) att_nm=(char *)nco_free(att_nm);
  if(att_val) att_val=(char *)nco_free(att_val);

  att_nm=strdup("source_file");
  att_val=strdup(rgr->fl_in);
  aed_mtd.att_nm=att_nm;
  aed_mtd.var_nm=NULL;
  aed_mtd.id=NC_GLOBAL;
  aed_mtd.sz=strlen(att_val);
  aed_mtd.type=NC_CHAR;
  aed_mtd.val.cp=att_val;
  aed_mtd.mode=aed_create;
  (void)nco_aed_prc(out_id,NC_GLOBAL,aed_mtd);
  if(att_nm) att_nm=(char *)nco_free(att_nm);
  if(att_val) att_val=(char *)nco_free(att_val);

  /* Annotate persistent metadata that should appear last in attribute list */
  if(flg_grd_out_1D){
    aed_mtd_crd.var_nm=area_nm_out;
    aed_mtd_crd.id=area_out_id;
    (void)nco_aed_prc(out_id,lat_out_id,aed_mtd_crd);
    
    aed_mtd_crd.var_nm=lat_nm_out;
    aed_mtd_crd.id=lat_out_id;
    (void)nco_aed_prc(out_id,lat_out_id,aed_mtd_crd);
    
    aed_mtd_crd.var_nm=lon_nm_out;
    aed_mtd_crd.id=lon_out_id;
    (void)nco_aed_prc(out_id,lon_out_id,aed_mtd_crd);
  } /* !flg_grd_out_1D */
  
  /* Persistent metadata */
  if(att_nm_crd) att_nm_crd=(char *)nco_free(att_nm_crd);
  if(att_val_crd) att_val_crd=(char *)nco_free(att_val_crd);

  /* Begin data mode */
  (void)nco_enddef(out_id);

  /* Write new coordinates and variables to regridded file */
  if(flg_grd_out_1D){
    dmn_srt_out[0]=0L;
    dmn_cnt_out[0]=ncol_nbr_out;
    (void)nco_put_vara(out_id,lat_out_id,dmn_srt_out,dmn_cnt_out,lat_ctr_out,crd_typ_out);
    dmn_srt_out[0]=0L;
    dmn_cnt_out[0]=ncol_nbr_out;
    (void)nco_put_vara(out_id,lon_out_id,dmn_srt_out,dmn_cnt_out,lon_ctr_out,crd_typ_out);
    dmn_srt_out[0]=dmn_srt_out[1]=0L;
    dmn_cnt_out[0]=ncol_nbr_out;
    dmn_cnt_out[1]=bnd_nbr_out;
    (void)nco_put_vara(out_id,lat_bnd_id,dmn_srt_out,dmn_cnt_out,lat_bnd_out,crd_typ_out);
    dmn_srt_out[0]=dmn_srt_out[1]=0L;
    dmn_cnt_out[0]=ncol_nbr_out;
    dmn_cnt_out[1]=bnd_nbr_out;
    (void)nco_put_vara(out_id,lon_bnd_id,dmn_srt_out,dmn_cnt_out,lon_bnd_out,crd_typ_out);
    dmn_srt_out[0]=0L;
    dmn_cnt_out[0]=ncol_nbr_out;
    (void)nco_put_vara(out_id,area_out_id,dmn_srt_out,dmn_cnt_out,area_out,crd_typ_out);
  } /* !flg_grd_out_1D */
  if(flg_grd_out_2D){
    dmn_srt_out[0]=0L;
    dmn_cnt_out[0]=lat_nbr_out;
    (void)nco_put_vara(out_id,lat_out_id,dmn_srt_out,dmn_cnt_out,lat_ctr_out,crd_typ_out);
    dmn_srt_out[0]=0L;
    dmn_cnt_out[0]=lon_nbr_out;
    (void)nco_put_vara(out_id,lon_out_id,dmn_srt_out,dmn_cnt_out,lon_ctr_out,crd_typ_out);
    dmn_srt_out[0]=0L;
    dmn_cnt_out[0]=lat_nbr_out;
    (void)nco_put_vara(out_id,lat_wgt_id,dmn_srt_out,dmn_cnt_out,lat_wgt_out,crd_typ_out);
    dmn_srt_out[0]=dmn_srt_out[1]=0L;
    dmn_cnt_out[0]=lat_nbr_out;
    dmn_cnt_out[1]=bnd_nbr_out;
    (void)nco_put_vara(out_id,lat_bnd_id,dmn_srt_out,dmn_cnt_out,lat_bnd_out,crd_typ_out);
    dmn_srt_out[0]=dmn_srt_out[1]=0L;
    dmn_cnt_out[0]=lon_nbr_out;
    dmn_cnt_out[1]=bnd_nbr_out;
    (void)nco_put_vara(out_id,lon_bnd_id,dmn_srt_out,dmn_cnt_out,lon_bnd_out,crd_typ_out);
    dmn_srt_out[0]=dmn_srt_out[1]=0L;
    dmn_cnt_out[0]=lat_nbr_out;
    dmn_cnt_out[1]=lon_nbr_out;
    (void)nco_put_vara(out_id,area_out_id,dmn_srt_out,dmn_cnt_out,area_out,crd_typ_out);
  } /* !flg_grd_out_2D */

  /* Regrid or copy variable values */
  const double wgt_vld_thr=rgr->wgt_vld_thr; /* [frc] Weight threshold for valid destination value */
  const nco_bool flg_rnr=rgr->flg_rnr; /* [flg] Renormalize destination values by valid area */
  const size_t grd_sz_in=rgr_map.src_grid_size; /* [nbr] Number of elements in single layer of input grid */
  const size_t grd_sz_out=rgr_map.dst_grid_size; /* [nbr] Number of elements in single layer of output grid */
  double *var_val_dbl_in;
  double *var_val_dbl_out;
  double *wgt_vld_out=NULL;
  double mss_val_dbl;
  double var_val_crr;
  int *tally=NULL; /* [nbr] Number of valid (non-missing) values */
  int idx_in; /* [idx] Input grid index */
  int idx_out; /* [idx] Output grid index */
  int lvl_idx; /* [idx] Level index */
  int lvl_nbr; /* [nbr] Number of levels */
  int thr_idx; /* [idx] Thread index */
  nco_bool has_mss_val; /* [flg] Has numeric missing value attribute */
  size_t dst_idx; 
  size_t var_sz_in; /* [nbr] Number of elements in variable (will be self-multiplied) */
  size_t var_sz_out; /* [nbr] Number of elements in variable (will be self-multiplied) */
  size_t val_in_fst; /* [nbr] Number of elements by which current N-D slab input values are offset from origin */
  size_t val_out_fst; /* [nbr] Number of elements by which current N-D slab output values are offset from origin */
  
  if(nco_dbg_lvl_get() >= nco_dbg_var) (void)fprintf(stdout,"Regridding progress: # means regridded, ~ means copied\n");

  /* Using naked stdin/stdout/stderr in parallel region generates warning
     Copy appropriate filehandle to variable scoped shared in parallel clause */
  FILE * const fp_stdout=stdout; /* [fl] stdout filehandle CEWI */

  /* OpenMP notes:
     default(): none
     firstprivate(): tally,wgt_vld_out (preserve NULL-initialization)
     private(): almost everything else
     shared(): flg_rnr,fnc_nm,wgt_vld_thr explicit shared for icc 13.1.3 (rhea), default shared for gcc 4.9.2 */
#ifdef __INTEL_COMPILER
# pragma omp parallel for default(none) firstprivate(tally,wgt_vld_out) private(dmn_cnt,dmn_id_in,dmn_id_out,dmn_idx,dmn_nbr_in,dmn_nbr_out,dmn_srt,dst_idx,has_mss_val,idx,idx_in,idx_out,idx_tbl,in_id,lnk_idx,lvl_idx,lvl_nbr,mss_val_dbl,rcd,thr_idx,trv,val_in_fst,val_out_fst,var_id_in,var_id_out,var_nm,var_sz_in,var_sz_out,var_typ,var_val_crr,var_val_dbl_in,var_val_dbl_out) shared(col_src_adr,flg_rnr,fnc_nm,lnk_nbr,out_id,row_dst_adr,wgt_raw,wgt_vld_thr)
#else /* !__INTEL_COMPILER */
# pragma omp parallel for default(none) firstprivate(tally,wgt_vld_out) private(dmn_cnt,dmn_id_in,dmn_id_out,dmn_idx,dmn_nbr_in,dmn_nbr_out,dmn_srt,dst_idx,has_mss_val,idx,idx_in,idx_out,idx_tbl,in_id,lnk_idx,lvl_idx,lvl_nbr,mss_val_dbl,rcd,thr_idx,trv,val_in_fst,val_out_fst,var_id_in,var_id_out,var_nm,var_sz_in,var_sz_out,var_typ,var_val_crr,var_val_dbl_in,var_val_dbl_out) shared(col_src_adr,lnk_nbr,out_id,row_dst_adr,wgt_raw)
#endif /* !__INTEL_COMPILER */
  for(idx_tbl=0;idx_tbl<trv_nbr;idx_tbl++){
    trv=trv_tbl->lst[idx_tbl];
    thr_idx=omp_get_thread_num();
    in_id=trv_tbl->in_id_arr[thr_idx];
#ifdef _OPENMP
    if(nco_dbg_lvl_get() >= nco_dbg_var && !thr_idx && !idx_tbl) (void)fprintf(fp_stdout,"%s: %s reports regrid loop uses %d thread%s\n",nco_prg_nm_get(),fnc_nm,omp_get_num_threads(),(omp_get_num_threads() > 1) ? "s" : "");
    if(nco_dbg_lvl_get() >= nco_dbg_io) (void)fprintf(fp_stdout,"%s: thread = %d, idx_tbl = %d, nm = %s\n",nco_prg_nm_get(),thr_idx,idx_tbl,trv.nm);
#endif /* !_OPENMP */
    if(trv.nco_typ == nco_obj_typ_var && trv.flg_xtr){
      if(nco_dbg_lvl_get() >= nco_dbg_var) (void)fprintf(fp_stdout,"%s%s ",trv.flg_rgr ? "#" : "~",trv.nm);
      if(trv.flg_rgr){
	/* Regrid variable */
	var_nm=trv.nm;
	//var_typ=trv.var_typ;
	var_typ=NC_DOUBLE; /* NB: Perform regridding in double precision */
	var_sz_in=1L;
	var_sz_out=1L;
	rcd=nco_inq_varid(in_id,var_nm,&var_id_in);
	rcd=nco_inq_varid(out_id,var_nm,&var_id_out);
	rcd=nco_inq_varndims(out_id,var_id_out,&dmn_nbr_out);
	rcd=nco_inq_varndims(in_id,var_id_in,&dmn_nbr_in);
	dmn_id_in=(int *)nco_malloc(dmn_nbr_in*sizeof(int));
	dmn_id_out=(int *)nco_malloc(dmn_nbr_out*sizeof(int));
	dmn_srt=(long *)nco_malloc(dmn_nbr_out*sizeof(long)); /* Allocate an extra slot for new dimensions */
	dmn_cnt=(long *)nco_malloc(dmn_nbr_out*sizeof(long));
	rcd=nco_inq_vardimid(out_id,var_id_out,dmn_id_out);
	rcd=nco_inq_vardimid(in_id,var_id_in,dmn_id_in);
	for(dmn_idx=0;dmn_idx<dmn_nbr_in;dmn_idx++){
	  rcd=nco_inq_dimlen(in_id,dmn_id_in[dmn_idx],dmn_cnt+dmn_idx);
	  var_sz_in*=dmn_cnt[dmn_idx];
	  dmn_srt[dmn_idx]=0L;
	} /* end loop over dimensions */
	var_val_dbl_in=(double *)nco_malloc_dbg(var_sz_in*nco_typ_lng(var_typ),"Unable to malloc() input value buffer",fnc_nm);
	rcd=nco_get_vara(in_id,var_id_in,dmn_srt,dmn_cnt,var_val_dbl_in,var_typ);

	for(dmn_idx=0;dmn_idx<dmn_nbr_out;dmn_idx++){
	  rcd=nco_inq_dimlen(out_id,dmn_id_out[dmn_idx],dmn_cnt+dmn_idx);
	  var_sz_out*=dmn_cnt[dmn_idx];
	  dmn_srt[dmn_idx]=0L;
	} /* end loop over dimensions */
	var_val_dbl_out=(double *)nco_malloc_dbg(var_sz_out*nco_typ_lng(var_typ),"Unable to malloc() input value buffer",fnc_nm);
	
	lvl_nbr=1;
	for(dmn_idx=0;dmn_idx<dmn_nbr_out-2;dmn_idx++) lvl_nbr*=dmn_cnt[dmn_idx];

	/* Initialize output */
	for(dst_idx=0;dst_idx<var_sz_out;dst_idx++) var_val_dbl_out[dst_idx]=0.0;
	/* Missing value setup */
	has_mss_val=nco_mss_val_get_dbl(in_id,var_id_in,&mss_val_dbl);
	if(has_mss_val) tally=(int *)nco_calloc(var_sz_out,nco_typ_lng(NC_INT));
	if(has_mss_val && flg_rnr) wgt_vld_out=(double *)nco_malloc_dbg(var_sz_out*nco_typ_lng(var_typ),"Unable to malloc() input weight buffer",fnc_nm);
	if(has_mss_val && flg_rnr) 
	  for(dst_idx=0;dst_idx<var_sz_out;dst_idx++) wgt_vld_out[dst_idx]=0.0;

	/* Apply weights */
	if(!has_mss_val){
	  if(lvl_nbr == 1){
	    for(lnk_idx=0;lnk_idx<lnk_nbr;lnk_idx++)
	      var_val_dbl_out[row_dst_adr[lnk_idx]]+=var_val_dbl_in[col_src_adr[lnk_idx]]*wgt_raw[lnk_idx];
	  }else{
	    val_in_fst=0L;
	    val_out_fst=0L;
	    for(lvl_idx=0;lvl_idx<lvl_nbr;lvl_idx++){
	      //if(nco_dbg_lvl_get() >= nco_dbg_crr) (void)fprintf(fp_stdout,"%s lvl_idx = %d val_in_fst = %li, val_out_fst = %li\n",trv.nm,lvl_idx,val_in_fst,val_out_fst);
	      for(lnk_idx=0;lnk_idx<lnk_nbr;lnk_idx++)
		var_val_dbl_out[row_dst_adr[lnk_idx]+val_out_fst]+=var_val_dbl_in[col_src_adr[lnk_idx]+val_in_fst]*wgt_raw[lnk_idx];
	      val_in_fst+=grd_sz_in;
	      val_out_fst+=grd_sz_out;
	    } /* end loop over lvl */
	  } /* lvl_nbr > 1 */
	}else{ /* has_mss_val */
	  if(lvl_nbr == 1){
	    for(lnk_idx=0;lnk_idx<lnk_nbr;lnk_idx++){
	      idx_in=col_src_adr[lnk_idx];
	      idx_out=row_dst_adr[lnk_idx];
	      if((var_val_crr=var_val_dbl_in[idx_in]) != mss_val_dbl){
		var_val_dbl_out[idx_out]+=var_val_crr*wgt_raw[lnk_idx];
		if(flg_rnr) wgt_vld_out[idx_out]+=wgt_raw[lnk_idx];
		tally[idx_out]++;
	      } /* endif */
	    } /* end loop over link */
	  }else{ /* lvl_nbr > 1 */
	    val_in_fst=0L;
	    val_out_fst=0L;
	    for(lvl_idx=0;lvl_idx<lvl_nbr;lvl_idx++){
	      for(lnk_idx=0;lnk_idx<lnk_nbr;lnk_idx++){
		idx_in=col_src_adr[lnk_idx]+val_in_fst;
		idx_out=row_dst_adr[lnk_idx]+val_out_fst;
		if((var_val_crr=var_val_dbl_in[idx_in]) != mss_val_dbl){
		  var_val_dbl_out[idx_out]+=var_val_crr*wgt_raw[lnk_idx];
		  if(flg_rnr) wgt_vld_out[idx_out]+=wgt_raw[lnk_idx];
		  tally[idx_out]++;
		} /* endif */
	      } /* end loop over link */
	      val_in_fst+=grd_sz_in;
	      val_out_fst+=grd_sz_out;
	    } /* end loop over lvl */
	  } /* lvl_nbr > 1 */
	  /* NCL and ESMF treatment of weights and missing values described at
	     https://www.ncl.ucar.edu/Applications/ESMF.shtml#WeightsAndMasking
	     http://earthsystemmodeling.org/esmf_releases/non_public/ESMF_6_1_1/ESMF_refdoc/node5.html#SECTION05012600000000000000
	     NCO implements one of two procedures: "conservative" or "renormalized"
	     The "conservative" algorithm uses all valid data from the input grid on the output grid
	     Destination cells receive the weighted valid values of the source cells
	     This is conservative because the global integrals of the source and destination fields are equal
	     The "renormalized" algorithm divides the destination value by the sum of the valid weights
	     This returns "reasonable" values, i.e., the mean of the valid input values
	     However, renormalization is equivalent to extrapolating valid data to missing regions
	     Hence the input and output integrals are unequal and the regridding is not conservative */
	  for(dst_idx=0;dst_idx<var_sz_out;dst_idx++)
	    if(!tally[dst_idx]) var_val_dbl_out[dst_idx]=mss_val_dbl;
	  if(flg_rnr){
	    if(wgt_vld_thr == 0.0){
	      for(dst_idx=0;dst_idx<var_sz_out;dst_idx++)
		if(tally[dst_idx]) var_val_dbl_out[dst_idx]/=wgt_vld_out[dst_idx];
	    }else{
	      for(dst_idx=0;dst_idx<var_sz_out;dst_idx++)
		if(wgt_vld_out[dst_idx] >= wgt_vld_thr){var_val_dbl_out[dst_idx]/=wgt_vld_out[dst_idx];}else{var_val_dbl_out[dst_idx]=mss_val_dbl;}
	    } /* !wgt_vld_thr */
	  } /* !flg_rnr */
	} /* !has_mss_val */
	
#pragma omp critical
	{ /* begin OpenMP critical */
	  rcd=nco_put_vara(out_id,var_id_out,dmn_srt,dmn_cnt,var_val_dbl_out,var_typ);
	} /* end OpenMP critical */
	
	if(dmn_id_in) dmn_id_out=(int *)nco_free(dmn_id_in);
	if(dmn_id_out) dmn_id_out=(int *)nco_free(dmn_id_out);
	if(dmn_srt) dmn_srt=(long *)nco_free(dmn_srt);
	if(dmn_cnt) dmn_cnt=(long *)nco_free(dmn_cnt);
	if(tally) tally=(int *)nco_free(tally);
	if(var_val_dbl_out) var_val_dbl_out=(double *)nco_free(var_val_dbl_out);
	if(var_val_dbl_in) var_val_dbl_in=(double *)nco_free(var_val_dbl_in);
	if(wgt_vld_out) wgt_vld_out=(double *)nco_free(wgt_vld_out);
      }else{
	/* Use standard NCO copy routine for variables that are not regridded */
#pragma omp critical
	{ /* begin OpenMP critical */
	  (void)nco_cpy_var_val(in_id,out_id,(FILE *)NULL,(md5_sct *)NULL,trv.nm,trv_tbl);
	} /* end OpenMP critical */
     } /* !flg_rgr */
    } /* !xtr */
  } /* end (OpenMP parallel for) loop over idx_tbl */
  if(nco_dbg_lvl_get() >= nco_dbg_var) (void)fprintf(stdout,"\n");
  if(nco_dbg_lvl_get() >= nco_dbg_fl) (void)fprintf(stdout,"%s: INFO %s completion report. Variables regridded = %d, copied unmodified = %d, omitted = %d, created = %d\n",nco_prg_nm_get(),fnc_nm,var_rgr_nbr,var_cpy_nbr,var_xcl_nbr,var_crt_nbr);
  
  /* Free memory allocated for grid reading/writing */
  if(area_out) area_out=(double *)nco_free(area_out);
  if(col_src_adr) col_src_adr=(int *)nco_free(col_src_adr);
  if(dmn_sz_out_int) dmn_sz_out_int=(int *)nco_free(dmn_sz_out_int);
  if(dmn_sz_in_int) dmn_sz_in_int=(int *)nco_free(dmn_sz_in_int);
  if(lat_bnd_out) lat_bnd_out=(double *)nco_free(lat_bnd_out);
  if(lat_crn_out) lat_crn_out=(double *)nco_free(lat_crn_out);
  if(lat_ctr_out) lat_ctr_out=(double *)nco_free(lat_ctr_out);
  if(lat_ntf_out) lat_ntf_out=(double *)nco_free(lat_ntf_out);
  if(lat_wgt_out) lat_wgt_out=(double *)nco_free(lat_wgt_out);
  if(lon_bnd_out) lon_bnd_out=(double *)nco_free(lon_bnd_out);
  if(lon_crn_out) lon_crn_out=(double *)nco_free(lon_crn_out);
  if(lon_ctr_out) lon_ctr_out=(double *)nco_free(lon_ctr_out);
  if(lon_ntf_out) lon_ntf_out=(double *)nco_free(lon_ntf_out);
  if(row_dst_adr) row_dst_adr=(int *)nco_free(row_dst_adr);
  if(wgt_raw) wgt_raw=(double *)nco_free(wgt_raw);
  
  return rcd;
} /* nco_rgr_map() */

void
nco_bsl_zro /* Return Bessel function zeros */
(const int bsl_zro_nbr, /* O [nbr] Order of Bessel function */
 double * const bsl_zro) /* O [frc] Bessel zero */
{
  /* Purpose: Return Bessel function zeros
     Source: CCM code /fs/cgd/csm/models/atm/ccm3.5.8/src/ccmlsm_share/bsslzr.F
     Return bsl_zro_nbr zeros (or if bsl_zro_nbr > 50, approximate zeros), of the Bessel function j0
     First 50 zeros are given exactly, and remaining zeros are computed by extrapolation, and therefore are not exact
     Original version:  CCM1
     Standardized:      J. Rosinski, June 1992
     Reviewed:          J. Hack, D. Williamson, August 1992
     Reviewed:          J. Hack, D. Williamson, April 1996
     Modified 19970123 by Jim Rosinski to use double precision arithmetic
     ~2000: Converted to Fortran9X by C. Zender, changed all real*16 statements to double precision (real*8)
     20150530: Converted to C99 by C. Zender */
  const char fnc_nm[]="nco_bsl_zro()"; /* [sng] Function name */
  const double pi=M_PI; // [frc] 3
  const double bsl_zro_tbl[]={ // Zeros of Bessel functions of order 1 to 50
    -1.e36, 2.4048255577,   5.5200781103, 
    8.6537279129,  11.7915344391,  14.9309177086,  18.0710639679, 
    21.2116366299,  24.3524715308,  27.4934791320,  30.6346064684, 
    33.7758202136,  36.9170983537,  40.0584257646,  43.1997917132, 
    46.3411883717,  49.4826098974,  52.6240518411,  55.7655107550, 
    58.9069839261,  62.0484691902,  65.1899648002,  68.3314693299, 
    71.4729816036,  74.6145006437,  77.7560256304,  80.8975558711, 
    84.0390907769,  87.1806298436,  90.3221726372,  93.4637187819, 
    96.6052679510,  99.7468198587, 102.8883742542, 106.0299309165, 
    109.1714896498, 112.3130502805, 115.4546126537, 118.5961766309, 
    121.7377420880, 124.8793089132, 128.0208770059, 131.1624462752, 
    134.3040166383, 137.4455880203, 140.5871603528, 143.7287335737, 
    146.8703076258, 150.0118824570, 153.1534580192, 156.2950342685};
  const int bsl_zro_tbl_nbr_max=sizeof(bsl_zro_tbl)/sizeof(double); /* [nbr] */
  int bsl_idx; /* [idx] Counting index */
    
  /* Main Code */
  if(nco_dbg_lvl_get() >= nco_dbg_sbr) (void)fprintf(stdout,"%s: DEBUG Entering %s\n",nco_prg_nm_get(),fnc_nm);
    
  assert(bsl_zro_tbl_nbr_max == 51); /* 51 is original size of 50 plus extra value for Fortran offset */

  for(bsl_idx=1;bsl_idx<=bsl_zro_nbr;bsl_idx++)
    if(bsl_idx <= bsl_zro_tbl_nbr_max) bsl_zro[bsl_idx]=bsl_zro_tbl[bsl_idx];

  if(bsl_zro_nbr > bsl_zro_tbl_nbr_max)
    for(bsl_idx=bsl_zro_tbl_nbr_max+1;bsl_idx<=bsl_zro_nbr;bsl_idx++)
      bsl_zro[bsl_idx]=bsl_zro[bsl_idx-1]+pi;
    
  if(nco_dbg_lvl_get() == nco_dbg_old){
    (void)fprintf(stdout,"%s: DEBUG %s reports bsl_zro_nbr = %d\n",nco_prg_nm_get(),fnc_nm,bsl_zro_nbr);
    (void)fprintf(stdout,"idx\tbsl_zro\n");
    for(bsl_idx=1;bsl_idx<=bsl_zro_nbr;bsl_idx++)
      (void)fprintf(stdout,"%d\t%g\n",bsl_idx,bsl_zro[bsl_idx]);
  } /* endif dbg */

  return;
} /* end nco_bsl_zro() */

void
nco_lat_wgt_gss /* [fnc] Compute and return sine of Gaussian latitudes and their weights */
(const int lat_nbr, /* I [nbr] Latitude number */
 double * const lat_sin, /* O [frc] Sine of latitudes */
 double * const wgt_Gss) /* O [frc] Gaussian weights */
{  
  /* Purpose: Compute and return sine of Gaussian latitudes and their weights
     Source: CCM /fs/cgd/csm/models/atm/ccm3.5.8/src/ccmlsm_share/gauaw.F
     Calculate sine of latitudes lat_sin(lat_nbr) and weights wgt_Gss(lat_nbr) for Gaussian quadrature
     Algorithm described in Davis and Rabinowitz, Journal of Research of the NBS, V 56, Jan 1956
     Zeros of Bessel function j0, obtained from nco_bsl_zro(), are first guess for abscissae
     Original version: CCM1
     Standardized: L. Bath, Jun 1992
                   L. Buja, Feb 1996
     Reviewed:     D. Williamson, J. Hack, Aug 1992
                   D. Williamson, J. Hack, Feb 1996
     19970123 Modified by Jim Rosinski to use real*16 arithmetic in order to 
     achieve (nearly) identical weights and latitudes on all machines.
     ~2000: Converted to Fortran9X by C. Zender, changed all real*16 statements to double precision (real*8)
     20150530: Converted to C99 by C. Zender */
  
  const char fnc_nm[]="nco_lat_wgt_gss()"; /* [sng] Function name */
  const double eps_rlt=1.0e-15; // Convergence criterion (NB: Threshold was 1.0d-27 in real*16, 1.0e-15 is for real*8)
  const double pi=M_PI; // [frc] 3
  const int itr_nbr_max=20; // [nbr] Maximum number of iterations
  double c; // Constant combination
  double lat_idx_dbl; // Latitude index, double precision
  double lat_nnr_idx_dbl; // Inner latitude index, double precision
  double lat_nbr_dbl; // [nbr] Number of latitudes, double precision
  double pk; // Polynomial
  double pkm1; // Polynomial
  double pkm2; // Polynomial
  double pkmrk; // Polynomial
  double sp; // Current iteration latitude increment
  double xz; // Abscissa estimate
  int itr_cnt; // Iteration counter
  int lat_idx; // [idx] Counting index (latitude)
  int lat_sym_idx; // [idx] Counting index (symmetric latitude)
  int lat_nnr_idx; // [idx] Counting index (inner latitude loop)
  int lat_nbr_rcp2; // lat_nbr/2 (number of latitudes in hemisphere)
  double *lat_sin_p1; // Sine of Gaussian latitudes double precision
  double *wgt_Gss_p1; // Gaussian weights double precision

  /* Main Code */
  if(nco_dbg_lvl_get() >= nco_dbg_sbr) (void)fprintf(stdout,"%s: DEBUG Entering %s\n",nco_prg_nm_get(),fnc_nm);
    
  /* Create arrays with Fortran indexing to keep numerical algorithm identical */
  lat_sin_p1=(double *)nco_malloc((lat_nbr+1)*sizeof(double)); // Sine of Gaussian latitudes double precision
  wgt_Gss_p1=(double *)nco_malloc((lat_nbr+1)*sizeof(double)); // Gaussian weights double precision
    
  /* Use Newton iteration to find abscissas */
  c=0.25*(1.0-4.0/(pi*pi));
  lat_nbr_dbl=lat_nbr;
  lat_nbr_rcp2=lat_nbr/2; // Integer arithmetic
  (void)nco_bsl_zro(lat_nbr_rcp2,lat_sin_p1);
  for(lat_idx=1;lat_idx<=lat_nbr_rcp2;lat_idx++){
    xz=cos(lat_sin_p1[lat_idx]/sqrt((lat_nbr_dbl+0.5)*(lat_nbr_dbl+0.5)+c));
    /* First approximation to xz */
    itr_cnt=0;
    /* goto label_73 */
  label_73:
    pkm2=1.0;
    pkm1=xz;
    itr_cnt=itr_cnt+1;
    if(itr_cnt > itr_nbr_max){
      (void)fprintf(stdout,"%s: ERROR %s reports no convergence in %d iterations for lat_idx = %d\n",nco_prg_nm_get(),fnc_nm,itr_nbr_max,lat_idx);
      nco_exit(EXIT_FAILURE);
    } /* endif */
    /* Compute Legendre polynomial */
    for(lat_nnr_idx=2;lat_nnr_idx<=lat_nbr;lat_nnr_idx++){
      lat_nnr_idx_dbl=lat_nnr_idx;
      pk=((2.0*lat_nnr_idx_dbl-1.0)*xz*pkm1-(lat_nnr_idx_dbl-1.0)*pkm2)/lat_nnr_idx_dbl;
      pkm2=pkm1;
      pkm1=pk;
    } /* end inner loop over lat_nnr */
    pkm1=pkm2;
    pkmrk=(lat_nbr_dbl*(pkm1-xz*pk))/(1.0-xz*xz);
    sp=pk/pkmrk;
    xz=xz-sp;
    if(fabs(sp) > eps_rlt) goto label_73;
    lat_sin_p1[lat_idx]=xz;
    wgt_Gss_p1[lat_idx]=(2.0*(1.0-xz*xz))/((lat_nbr_dbl*pkm1)*(lat_nbr_dbl*pkm1));
  } /* end outer loop over lat */
  if(lat_nbr != lat_nbr_rcp2*2){
    /* When lat_nbr is odd, compute weight at Equator */
    lat_sin_p1[lat_nbr_rcp2+1]=0.0;
    pk=2.0/(lat_nbr_dbl*lat_nbr_dbl);
    for(lat_idx=2;lat_idx<=lat_nbr;lat_idx+=2){
      lat_idx_dbl=lat_idx;
      pk=pk*lat_idx_dbl*lat_idx_dbl/((lat_idx_dbl-1.0)*(lat_idx_dbl-1.0));
    } /* end loop over lat */
    wgt_Gss_p1[lat_nbr_rcp2+1]=pk;
  } /* endif lat_nbr is odd */
    
  /* Complete sets of abscissas and weights, using symmetry properties */
  for(lat_idx=1;lat_idx<=lat_nbr_rcp2;lat_idx++){
    lat_sym_idx=lat_nbr-lat_idx+1;
    lat_sin_p1[lat_sym_idx]=-lat_sin_p1[lat_idx];
    wgt_Gss_p1[lat_sym_idx]=wgt_Gss_p1[lat_idx];
  } /* end loop over lat */
    
  /* Shift by one to remove Fortran offset in p1 arrays */
  //memcpy(lat_sin,lat_sin_p1,lat_nbr*sizeof(double));
  //memcpy(wgt_Gss,wgt_Gss_p1,lat_nbr*sizeof(double));
  
  /* Reverse and shift arrays because original CCM code algorithm computed latitudes from north-to-south
     Shift by one to remove Fortran offset in p1 arrays */
  for(lat_idx=0;lat_idx<lat_nbr;lat_idx++){
    lat_sin[lat_idx]=lat_sin_p1[lat_nbr-lat_idx];
    wgt_Gss[lat_idx]=wgt_Gss_p1[lat_nbr-lat_idx];
  } /* end loop over lat */

  if(nco_dbg_lvl_get() == nco_dbg_old){
    (void)fprintf(stdout,"%s: DEBUG %s reports lat_nbr = %d\n",nco_prg_nm_get(),fnc_nm,lat_nbr);
    (void)fprintf(stdout,"idx\tasin\tngl_rad\tngl_dgr\tgw\n");
    for(lat_idx=0;lat_idx<lat_nbr;lat_idx++)
      (void)fprintf(stdout,"%d\t%g\t%g\t%g%g\n",lat_idx,lat_sin[lat_idx],asin(lat_sin[lat_idx]),180.0*asin(lat_sin[lat_idx])/pi,wgt_Gss[lat_idx]);
  } /* endif dbg */
  
  if(wgt_Gss_p1) wgt_Gss_p1=(double *)nco_free(wgt_Gss_p1);
  if(lat_sin_p1) lat_sin_p1=(double *)nco_free(lat_sin_p1);
  return;
} /* end nco_lat_wgt_gss() */
  
int /* O [enm] Return code */
nco_rgr_tps /* [fnc] Regrid using Tempest library */
(rgr_sct * const rgr) /* I/O [sct] Regridding structure */
{
  /* Purpose: Regrid fields using Tempest remapping "library" (more precisely, executables)

     Test Tempest library: no way to activate yet
     export DATA_TEMPEST='/data/zender/rgr';ncks -O --rgr=Y ${DATA}/rgr/essgcm14_clm.nc ~/foo.nc */

  const char fnc_nm[]="nco_rgr_tps()";
  
  const int fmt_chr_nbr=6;
  const char *cmd_rgr_fmt;
  char *cmd_rgr;
  char fl_grd_dst[]="/tmp/foo_outRLLMesh.g";
  char *fl_grd_dst_cdl;
  int rcd_sys;
  int lat_nbr_rqs=180;
  int lon_nbr_rqs=360;
  nco_rgr_cmd_typ nco_rgr_cmd; /* [enm] Tempest remap command enum */

  char *nvr_DATA_TEMPEST; /* [sng] Directory where Tempest grids, meshes, and weights are stored */
  nvr_DATA_TEMPEST=getenv("DATA_TEMPEST");
  rgr->drc_tps= (nvr_DATA_TEMPEST && strlen(nvr_DATA_TEMPEST) > 0L) ? (char *)strdup(nvr_DATA_TEMPEST) : (char *)strdup("/tmp");

  if(nco_dbg_lvl_get() >= nco_dbg_crr){
    (void)fprintf(stderr,"%s: INFO %s reports\n",nco_prg_nm_get(),fnc_nm);
    (void)fprintf(stderr,"drc_tps = %s, ",rgr->drc_tps ? rgr->drc_tps : "NULL");
    (void)fprintf(stderr,"\n");
  } /* endif dbg */

  /* Allow for whitespace characters in fl_grd_dst
     Assume CDL translation results in acceptable name for shell commands */
  fl_grd_dst_cdl=nm2sng_fl(fl_grd_dst);
  //drc_tps=strcat(drc_tps,"/");
  //drc_tps=strcat(drc_tps,fl_grd_dst);

  /* Construct and execute regridding command */
  nco_rgr_cmd=nco_rgr_GenerateRLLMesh;
  cmd_rgr_fmt=nco_tps_cmd_fmt_sng(nco_rgr_cmd);
  cmd_rgr=(char *)nco_malloc((strlen(cmd_rgr_fmt)+strlen(fl_grd_dst_cdl)-fmt_chr_nbr+1UL)*sizeof(char));
  if(nco_dbg_lvl_get() >= nco_dbg_fl) (void)fprintf(stderr,"%s: %s reports generating %d by %d RLL mesh in %s...\n",nco_prg_nm_get(),fnc_nm,lat_nbr_rqs,lon_nbr_rqs,fl_grd_dst);
  (void)sprintf(cmd_rgr,cmd_rgr_fmt,lat_nbr_rqs,lon_nbr_rqs,fl_grd_dst_cdl);
  rcd_sys=system(cmd_rgr);
  if(rcd_sys == -1){
    (void)fprintf(stdout,"%s: ERROR %s unable to complete Tempest regridding command \"%s\"\n",nco_prg_nm_get(),fnc_nm,cmd_rgr);
    nco_exit(EXIT_FAILURE);
  } /* end if */
  if(nco_dbg_lvl_get() >= nco_dbg_std) (void)fprintf(stderr,"done\n");

  /* Clean-up memory */
  if(fl_grd_dst_cdl) fl_grd_dst_cdl=(char *)nco_free(fl_grd_dst_cdl);
  if(cmd_rgr) cmd_rgr=(char *)nco_free(cmd_rgr);
  
  return NCO_NOERR;
} /* end nco_rgr_tps() */

const char * /* O [sng] String describing two-dimensional grid-type */
nco_grd_2D_sng /* [fnc] Convert two-dimensional grid-type enum to string */
(const nco_grd_2D_typ_enm nco_grd_2D_typ) /* I [enm] Two-dimensional grid-type enum */
{
  /* Purpose: Convert two-dimensional grid-type enum to string */
  switch(nco_grd_2D_typ){
  case nco_grd_2D_gss: return "Gaussian latitude grid used by global spectral models: CCM 1-3, CAM 1-3, LSM, MATCH, UCICTM";
  case nco_grd_2D_ngl_eqi_pol: return "Equi-angle latitude grid with poles at centers of first and last gridpoints (i.e., lat_ctr[0]=-90), aka FV scalar grid: CAM FV, GEOS-CHEM, UCICTM, UKMO";
  case nco_grd_2D_ngl_eqi_fst: return "Equi-angle latitude grid with poles at edges of first and last gridpoints (i.e., lat_ctr[0]=-89.xxx), aka FV staggered velocity grid: CIESIN/SEDAC, IGBP-DIS, TOMS AAI";
  default: nco_dfl_case_generic_err(); break;
  } /* end switch */

  /* Some compilers: e.g., SGI cc, need return statement to end non-void functions */
  return (char *)NULL;
} /* end nco_grd_2D_sng() */

const char * /* O [sng] String describing grid conversion */
nco_rgr_grd_sng /* [fnc] Convert grid conversion enum to string */
(const nco_rgr_grd_typ_enm nco_rgr_grd_typ) /* I [enm] Grid conversion enum */
{
  /* Purpose: Convert grid conversion enum to string */
  switch(nco_rgr_grd_typ){
  case nco_rgr_grd_1D_to_1D: return "1D_to_1D";
  case nco_rgr_grd_1D_to_2D: return "1D_to_2D";
  case nco_rgr_grd_2D_to_1D: return "2D_to_1D";
  case nco_rgr_grd_2D_to_2D: return "2D_to_2D";
  default: nco_dfl_case_generic_err(); break;
  } /* end switch */

  /* Some compilers: e.g., SGI cc, need return statement to end non-void functions */
  return (char *)NULL;
} /* end nco_rgr_grd_sng() */

const char * /* O [sng] String describing regridding method */
nco_rgr_mth_sng /* [fnc] Convert regridding method enum to string */
(const nco_rgr_mth_typ_enm nco_rgr_mth_typ) /* I [enm] Regridding method enum */
{
  /* Purpose: Convert regridding method enum to string */
  switch(nco_rgr_mth_typ){
  case nco_rgr_mth_conservative: return "Conservative remapping";
  case nco_rgr_mth_bilinear: return "Bilinear remapping";
  case nco_rgr_mth_none: return "none";
  default: nco_dfl_case_generic_err(); break;
  } /* end switch */

  /* Some compilers: e.g., SGI cc, need return statement to end non-void functions */
  return (char *)NULL;
} /* end nco_rgr_mth_sng() */

const char * /* O [sng] String describing regridding normalization */
nco_rgr_nrm_sng /* [fnc] Convert regridding normalization enum to string */
(const nco_rgr_nrm_typ_enm nco_rgr_nrm_typ) /* I [enm] Regridding normalization enum */
{
  /* Purpose: Convert regridding normalization enum to string */
  switch(nco_rgr_nrm_typ){
  case nco_rgr_nrm_fracarea: return "fracarea";
  case nco_rgr_nrm_destarea: return "destarea";
  case nco_rgr_nrm_none: return "none";
  default: nco_dfl_case_generic_err(); break;
  } /* end switch */

  /* Some compilers: e.g., SGI cc, need return statement to end non-void functions */
  return (char *)NULL;
} /* end nco_rgr_nrm_sng() */

const char * /* O [sng] String containing regridding command and format */
nco_tps_cmd_fmt_sng /* [fnc] Convert Tempest remap command enum to command string */
(const nco_rgr_cmd_typ nco_rgr_cmd) /* I [enm] Tempest remap command enum */
{
  /* Purpose: Convert Tempest remap command enum to command string and format */
  switch(nco_rgr_cmd){
  case nco_rgr_ApplyOfflineMap:
    return "ApplyOfflineMap";
  case nco_rgr_CalculateDiffNorms:
    return "CalculateDiffNorms";
  case nco_rgr_GenerateCSMesh:
    return "GenerateCSMesh --res %d --file %s";
  case nco_rgr_GenerateGLLMetaData:
    return "GenerateGLLMetaData";
  case nco_rgr_GenerateICOMesh:
    return "GenerateICOMesh";
  case nco_rgr_GenerateLambertConfConicMesh:
    return "GenerateLambertConfConicMesh";
  case nco_rgr_GenerateOfflineMap:
    return "GenerateOfflineMap --in_mesh %s --out_mesh %s --ov_mesh %s --in_data %s --out_data %s";
  case nco_rgr_GenerateOverlapMesh:
    return "GenerateOverlapMesh --a %s --b %s --out %s";
  case nco_rgr_GenerateRLLMesh:
    return "GenerateRLLMesh --lat %d --lon %d --file %s";
  case nco_rgr_GenerateTestData:
    return "GenerateTestData --mesh %s --np %d --test %d --out %s";
  case nco_rgr_MeshToTxt:
    return "MeshToTxt";
  case nco_rgr_AAA_nil:
  case nco_rgr_ZZZ_last:
  default: nco_dfl_case_generic_err(); break;
  } /* end switch */
  
  /* Some compilers: e.g., SGI cc, need return statement to end non-void functions */
  return (char *)NULL;
} /* end nco_tps_cmd_fmt_sng() */

const char * /* O [sng] String containing regridding command name */
nco_tps_cmd_sng /* [fnc] Convert Tempest remap command enum to command name */
(const nco_rgr_cmd_typ nco_rgr_cmd) /* I [enm] Tempest remap command enum */
{
  /* Purpose: Convert Tempest remap command enum to command string */
  switch(nco_rgr_cmd){
  case nco_rgr_ApplyOfflineMap: return "ApplyOfflineMap";
  case nco_rgr_CalculateDiffNorms: return "CalculateDiffNorms";
  case nco_rgr_GenerateCSMesh: return "GenerateCSMesh";
  case nco_rgr_GenerateGLLMetaData: return "GenerateGLLMetaData";
  case nco_rgr_GenerateICOMesh: return "GenerateICOMesh";
  case nco_rgr_GenerateLambertConfConicMesh: return "GenerateLambertConfConicMesh";
  case nco_rgr_GenerateOfflineMap: return "GenerateOfflineMap";
  case nco_rgr_GenerateOverlapMesh: return "GenerateOverlapMesh";
  case nco_rgr_GenerateRLLMesh: return "GenerateRLLMesh";
  case nco_rgr_GenerateTestData: return "GenerateTestData";
  case nco_rgr_MeshToTxt: return "MeshToTxt";
  case nco_rgr_AAA_nil:
  case nco_rgr_ZZZ_last:
  default: nco_dfl_case_generic_err(); break;
  } /* end switch */

  /* Some compilers: e.g., SGI cc, need return statement to end non-void functions */
  return (char *)NULL;
} /* end nco_tps_cmd_sng() */

#ifdef ENABLE_ESMF
const char * /* O [sng] String version of ESMC_FileFormat_Flag */
nco_esmf_fl_fmt_sng /* [fnc] Convert ESMF file format to string */
(const int nco_esmf_fl_fmt) /* I [enm] ESMF file format enum ESMC_FileFormat_Flag */
{
  /* Purpose: Convert ESMF fileTypeFlag enum to string */
  switch(nco_esmf_fl_fmt){
  case ESMC_FILEFORMAT_UNDEFINED: return "ESMC_FILEFORMAT_UNDEFINED";
  case ESMC_FILEFORMAT_VTK: return "ESMC_FILEFORMAT_VTK";
  case ESMC_FILEFORMAT_SCRIP: return "ESMC_FILEFORMAT_SCRIP";
  case ESMC_FILEFORMAT_ESMFMESH: return "ESMC_FILEFORMAT_ESMFMESH";
  case ESMC_FILEFORMAT_ESMCGRID: return "ESMC_FILEFORMAT_ESMCGRID";
  case ESMC_FILEFORMAT_UGRID: return "ESMC_FILEFORMAT_UGRID";
  case ESMC_FILEFORMAT_GRIDSPEC: return "ESMC_FILEFORMAT_GRIDSPEC";
  default: nco_dfl_case_generic_err(); break;
  } /* end switch */

  /* Some compilers: e.g., SGI cc, need return statement to end non-void functions */
  return (char *)NULL;
} /* end nco_esmf_fl_fmt_sng() */

const char * /* O [sng] String version of ESMC_StaggerLoc enum */
nco_esmf_stg_lcn_sng /* [fnc] Convert ESMF stagger location enum to string */
(const int nco_esmf_stg_lcn) /* I [enm] ESMF stagger location enum ESMC_StaggerLoc */
{
  /* Purpose: Convert ESMF stagger location enum ESMC_StaggerLoc to string */
  switch(nco_esmf_stg_lcn){
  case ESMC_STAGGERLOC_INVALID: return "ESMC_STAGGERLOC_INVALID";
  case ESMC_STAGGERLOC_UNINIT: return "ESMC_STAGGERLOC_UNINIT";
  case ESMC_STAGGERLOC_CENTER: return "ESMC_STAGGERLOC_CENTER";
  case ESMC_STAGGERLOC_EDGE1: return "ESMC_STAGGERLOC_EDGE1";
  case ESMC_STAGGERLOC_EDGE2: return "ESMC_STAGGERLOC_EDGE2";
  case ESMC_STAGGERLOC_CORNER: return "ESMC_STAGGERLOC_CORNER";
  default: abort(); break;
  } /* end switch */

  /* Some compilers: e.g., SGI cc, need return statement to end non-void functions */
  return (char *)NULL;
} /* end nco_esmf_stg_lcn_sng() */

const char * /* O [sng] String version of ESMC_GridItem_Flag enum */
nco_esmf_grd_itm_sng /* [fnc] Convert ESMF grid item enum to string */
(const int nco_esmf_grd_itm) /* I [enm] ESMF grid item enum ESMC_GridItem_Flag */
{
  /* Purpose: Convert ESMF grid item enum ESMC_GridItem_Flag to string */
  switch(nco_esmf_grd_itm){
  case ESMC_GRIDITEM_INVALID: return "ESMC_GRIDITEM_INVALID";
  case ESMC_GRIDITEM_UNINIT: return "ESMC_GRIDITEM_UNINIT";
  case ESMC_GRIDITEM_MASK: return "ESMC_GRIDITEM_MASK";
  case ESMC_GRIDITEM_AREA: return "ESMC_GRIDITEM_AREA";
  default: abort(); break;
  } /* end switch */

  /* Some compilers: e.g., SGI cc, need return statement to end non-void functions */
  return (char *)NULL;
} /* end nco_esmf_grd_itm_sng() */

const char * /* O [sng] String version of ESMC_CoordSys_Flag enum */
nco_esmf_crd_sys_sng /* [fnc] Convert ESMF coordinate system enum to string */
(const int nco_esmf_crd_sys) /* I [enm] ESMF coordinate system enum ESMC_CoordSys_Flag */
{
  /* Purpose: Convert ESMF coordinate system enum ESMC_CoordSys_Flag to string */
  switch(nco_esmf_crd_sys){
  case ESMC_COORDSYS_INVALID: return "ESMC_COORDSYS_INVALID";
  case ESMC_COORDSYS_UNINIT: return "ESMC_COORDSYS_UNINIT";
  case ESMC_COORDSYS_CART: return "ESMC_COORDSYS_CART";
  case ESMC_COORDSYS_SPH_DEG: return "ESMC_COORDSYS_SPH_DEG";
  case ESMC_COORDSYS_SPH_RAD: return "ESMC_COORDSYS_SPH_RAD";
  default: abort(); break;
  } /* end switch */

  /* Some compilers: e.g., SGI cc, need return statement to end non-void functions */
  return (char *)NULL;
} /* end nco_esmf_crd_sys_sng() */

const char * /* O [sng] String version of ESMC_TypeKind_Flag enum */
nco_esmf_typ_knd_sng /* [fnc] Convert ESMF type kind enum to string */
(const int nco_esmf_typ_knd) /* I [enm] ESMF type kind enum ESMC_TypeKind_Flag */
{
  /* Purpose: Convert ESMF type kind enum ESMC_TypeKind_Flag to string */
  switch(nco_esmf_typ_knd){
  case ESMC_TYPEKIND_I1: return "ESMC_TYPEKIND_I1";
  case ESMC_TYPEKIND_I2: return "ESMC_TYPEKIND_I2";
  case ESMC_TYPEKIND_I4: return "ESMC_TYPEKIND_I4";
  case ESMC_TYPEKIND_I8: return "ESMC_TYPEKIND_I8";
  case ESMC_TYPEKIND_R4: return "ESMC_TYPEKIND_R4";
  case ESMC_TYPEKIND_R8: return "ESMC_TYPEKIND_R8";
  case ESMC_TYPEKIND_LOGICAL: return "ESMC_TYPEKIND_LOGICAL";
  case ESMC_TYPEKIND_CHARACTER: return "ESMC_TYPEKIND_CHARACTER";
  default: abort(); break;
  } /* end switch */

  /* Some compilers: e.g., SGI cc, need return statement to end non-void functions */
  return (char *)NULL;
} /* end nco_esmf_typ_knd_sng() */

const char * /* O [sng] String version of ESMC_LogMsgType_Flag enum */
nco_esmf_log_msg_sng /* [fnc] Convert ESMF log message type enum to string */
(const int nco_esmf_log_msg) /* I [enm] ESMF log message type enum ESMC_LogMsgType_Flag */
{
  /* Purpose: Convert ESMF coordinate system enum ESMC_LogMsgType_Flag to string */
  switch(nco_esmf_log_msg){
  case ESMC_LOGMSG_INFO: return "ESMC_LOGMSG_INFO";
  case ESMC_LOGMSG_WARN: return "ESMC_LOGMSG_WARN";
  case ESMC_LOGMSG_ERROR: return "ESMC_LOGMSG_ERROR";
  case ESMC_LOGMSG_TRACE: return "ESMC_LOGMSG_TRACE";
  default: abort(); break;
  } /* end switch */

  /* Some compilers: e.g., SGI cc, need return statement to end non-void functions */
  return (char *)NULL;
} /* end nco_esmf_log_msg_sng() */

const char * /* O [sng] String version of ESMC_RegridMethod_Flag enum */
nco_esmf_rgr_mth_sng /* [fnc] Convert ESMF regrid method type enum to string */
(const int nco_esmf_rgr_mth) /* I [enm] ESMF regrid method type enum ESMC_RegridMethod_Flag */
{
  /* Purpose: Convert ESMF regrid method enum ESMC_RegridMethod_Flag to string */
  switch(nco_esmf_rgr_mth){
  case ESMC_REGRIDMETHOD_BILINEAR: return "ESMC_REGRIDMETHOD_BILINEAR";
  case ESMC_REGRIDMETHOD_PATCH: return "ESMC_REGRIDMETHOD_PATCH";
  case ESMC_REGRIDMETHOD_CONSERVE: return "ESMC_REGRIDMETHOD_CONSERVE";
  case ESMC_REGRIDMETHOD_NEAREST_STOD: return "ESMC_REGRIDMETHOD_NEAREST_STOD";
  case ESMC_REGRIDMETHOD_NEAREST_DTOS: return "ESMC_REGRIDMETHOD_NEAREST_DTOS";
  default: abort(); break;
  } /* end switch */

  /* Some compilers: e.g., SGI cc, need return statement to end non-void functions */
  return (char *)NULL;
} /* end nco_esmf_rgr_mth_sng() */

const char * /* O [sng] String version of ESMC_PoleMethod_Flag enum */
nco_esmf_pll_mth_sng /* [fnc] Convert ESMF pole method type enum to string */
(const int nco_esmf_pll_mth) /* I [enm] ESMF pole method type enum ESMC_PoleMethod_Flag */
{
  /* Purpose: Convert ESMF pole method enum ESMC_PoleMethod_Flag to string */
  switch(nco_esmf_pll_mth){
  case ESMC_POLEMETHOD_NONE: return "ESMC_POLEMETHOD_NONE";
  case ESMC_POLEMETHOD_ALLAVG: return "ESMC_POLEMETHOD_ALLAVG";
  case ESMC_POLEMETHOD_NPNTAVG: return "ESMC_POLEMETHOD_NPNTAVG";
  case ESMC_POLEMETHOD_TEETH: return "ESMC_POLEMETHOD_TEETH";
  default: abort(); break;
  } /* end switch */

  /* Some compilers: e.g., SGI cc, need return statement to end non-void functions */
  return (char *)NULL;
} /* end nco_esmf_pll_mth_sng() */

const char * /* O [sng] String version of ESMC_UnmappedAction_Flag enum */
nco_esmf_unm_act_sng /* [fnc] Convert ESMF unmapped action type enum to string */
(const int nco_esmf_unm_act) /* I [enm] ESMF unmapped action type enum ESMC_UnmappedAction_Flag */
{
  /* Purpose: Convert ESMF unmapped action enum ESMC_UnmappedAction_Flag to string */
  switch(nco_esmf_unm_act){
  case ESMC_UNMAPPEDACTION_ERROR: return "ESMC_UNMAPPEDACTION_ERROR";
  case ESMC_UNMAPPEDACTION_IGNORE: return "ESMC_UNMAPPEDACTION_IGNORE";
  default: abort(); break;
  } /* end switch */

  /* Some compilers: e.g., SGI cc, need return statement to end non-void functions */
  return (char *)NULL;
} /* end nco_esmf_unm_act_sng() */

const char * /* O [sng] String version of ESMC_Region_Flag enum */
nco_esmf_rgn_flg_sng /* [fnc] Convert ESMF region flag enum to string */
(const int nco_esmf_rgn_flg) /* I [enm] ESMF region flag enum ESMC_Region_Flag */
{
  /* Purpose: Convert ESMF region flag enum ESMC_Region_Flag to string */
  switch(nco_esmf_rgn_flg){
  case ESMC_REGION_TOTAL: return "ESMC_REGION_TOTAL";
  case ESMC_REGION_SELECT: return "ESMC_REGION_SELECT";
  case ESMC_REGION_EMPTY: return "ESMC_REGION_EMPTY";
  default: abort(); break;
  } /* end switch */

  /* Some compilers: e.g., SGI cc, need return statement to end non-void functions */
  return (char *)NULL;
} /* end nco_esmf_rgn_flg_sng() */

int /* O [enm] Return code */
nco_rgr_esmf /* [fnc] Regrid using ESMF library */
(rgr_sct * const rgr) /* I/O [sct] Regridding structure */
{
  /* Purpose: Regrid fields
     ESMC is C-interface to ESMF documented at
     http://www.earthsystemmodeling.org/esmf_releases/last_built/ESMC_crefdoc/ESMC_crefdoc.html
     ESMF Developer's Guide
     http://www.earthsystemmodeling.org/documents/dev_guide
     ESMF_RegridWeightGen
     http://www.earthsystemcog.org/projects/regridweightgen
     http://www.earthsystemmodeling.org/python_releases/last_esmpy/esmpy_doc/html/index.html

     ESMF C-interface examples:
     ${DATA}/esmf/src/Infrastructure/Field/tests/ESMC_FieldRegridUTest.C
     ${DATA}/esmf/src/Infrastructure/Field/tests/ESMC_FieldRegridCsrvUTest.C */
  
  const char fnc_nm[]="nco_rgr_esmf()"; /* [sng] Function name */
  const char fl_nm_esmf_log[]="nco_rgr_log_foo.txt"; /* [sng] Log file for ESMF routines */
  
  enum ESMC_StaggerLoc stg_lcn_src=ESMC_STAGGERLOC_CENTER; /* [enm] Stagger location of source grid */
  enum ESMC_StaggerLoc stg_lcn_dst=ESMC_STAGGERLOC_CENTER; /* [enm] Stagger location of destination grid */

  int flg_openMPEnabledFlag; /* [flg] ESMF library was compiled with OpenMP enabled */
  int flg_pthreadsEnabledFlag; /* [flg] ESMF library was compiled with Pthreads enabled */
  int localPet; /* [id] ID of PET that issued call */
  int peCount; /* [nbr] Number of PEs referenced by ESMF_VM */
  int petCount; /* [nbr] Number of PETs referenced by ESMF_VM */
  int rcd_esmf; /* [enm] Return codes from ESMF functions */
  
  int dmn_nbr_grd=2;
  int rcd=ESMF_SUCCESS;
  
  int *dmn_id;
  
  int dmn_nbr;
  int idx;
  int var_in_id;
  
  long *dmn_cnt;
  long *dmn_srt;
  
  nc_type var_typ_in;
  nc_type crd_typ_out=NC_DOUBLE;
  nc_type var_typ_out=NC_DOUBLE;
  
  double *lon_in;
  double *lat_in;
  void *void_ptr_var;
  
  int in_id; /* I [id] Input netCDF file ID */
  int out_id; /* I [id] Output netCDF file ID */

  /* SCRIP rank-2 grids are almost always lat,lon (C) and lon,lat (Fortran)
     Indexing is confusing because ESMF always uses Fortran-ordering convention with mixture of 0- and 1-based indices
     netCDF always uses C-ordering convention with 0-based indices
     No data transposition is necessary because variable RAM is always in C-order (same as netCDF)
     However, Fortran and C (i.e., ESMF and netCDF) access that same RAM using different conventions
     2-D data are always C[lat,lon] or F(lon,lat) in practice
     2-D stored in netCDF or RAM as C[lon,lat] and F(lat,lon) are possible, though not seen in practice
     NCO regridding below assumes data are in C[lat,lon] order
     NCO hardcoded subscript conventions follow this pattern:
     crd_idx_[axs]_[ibs]_[dmn]_[grd] where
     axs = access convention = C or Fortran = _c_ or _f_ 
     ibs = index base = 0 or 1 = _0bs_ or _1bs_ for zero-based or one-based indices
     NB: axs is independent from ibs! 
     dmn = dimension = lon or lat
     grd = grid = source or destination */
  const int crd_idx_c_0bs_lat_dst=0; /* [dgr] 0-based index of latitude  in C       representation of rank-2 destination grids */
  const int crd_idx_c_0bs_lon_dst=1; /* [dgr] 0-based index of longitude in C       representation of rank-2 destination grids */
  const int crd_idx_f_0bs_lat_dst=1; /* [dgr] 0-based index of latitude  in Fortran representation of rank-2 destination grids */
  const int crd_idx_f_0bs_lat_src=1; /* [dgr] 0-based index of latitude  in Fortran representation of rank-2 source grids */
  const int crd_idx_f_0bs_lon_dst=0; /* [dgr] 0-based index of longitude in Fortran representation of rank-2 destination grids */
  const int crd_idx_f_0bs_lon_src=0; /* [dgr] 0-based index of longitude in Fortran representation of rank-2 source grids */
  const int crd_idx_f_1bs_lat_dst=2; /* [dgr] 1-based index of latitude  in Fortran representation of rank-2 destination grids */
  const int crd_idx_f_1bs_lat_src=2; /* [dgr] 1-based index of latitude  in Fortran representation of rank-2 source grids */
  const int crd_idx_f_1bs_lon_dst=1; /* [dgr] 1-based index of longitude in Fortran representation of rank-2 destination grids */
  const int crd_idx_f_1bs_lon_src=1; /* [dgr] 1-based index of longitude in Fortran representation of rank-2 source grids */

  /* Initialize */
  in_id=rgr->in_id;
  out_id=rgr->out_id;
  
  /* Obtain input longitude type and length */
  char lon_nm_in[]="lon";
  (void)nco_inq_varid(in_id,lon_nm_in,&var_in_id);
  (void)nco_inq_var(in_id,var_in_id,(char *)NULL,&var_typ_in,&dmn_nbr,(int *)NULL,(int *)NULL);
  if(var_typ_in != NC_DOUBLE){
    assert(var_typ_in == NC_FLOAT);
    if(nco_dbg_lvl_get() >= nco_dbg_std) (void)fprintf(stderr,"%s: INFO WARNING Cheapskate regridding input file %s stores coordinate as NC_FLOAT\n",nco_prg_nm_get(),rgr->fl_in);
  } /* !var_typ_in */

  /* Allocate space to hold dimension metadata */
  dmn_cnt=(long *)nco_malloc(dmn_nbr*sizeof(long));
  dmn_id=(int *)nco_malloc(dmn_nbr*sizeof(int));
  dmn_srt=(long *)nco_malloc(dmn_nbr*sizeof(long));
  (void)nco_inq_vardimid(in_id,var_in_id,dmn_id);
  /* Get input dimension sizes */
  long dmn_sz=1L; /* [nbr] Number of elements in dimension (will be self-multiplied) */
  for(idx=0;idx<dmn_nbr;idx++){
    (void)nco_inq_dimlen(in_id,dmn_id[idx],dmn_cnt+idx);
    dmn_srt[idx]=0L;
    dmn_sz*=dmn_cnt[idx];
  } /* end loop over dim */
  
  int *max_idx; /* [nbr] Zero-based array containing dimension sizes in order? */
  max_idx=(int *)nco_malloc(dmn_nbr_grd*sizeof(int));
  max_idx[crd_idx_f_0bs_lon_src]=dmn_sz; /* [nbr] Number of elements in dimensions */
  
  /* Allocate space for and obtain longitude */
  lon_in=(double *)nco_malloc(dmn_sz*nco_typ_lng(crd_typ_out));
  rcd=nco_get_vara(in_id,var_in_id,dmn_srt,dmn_cnt,lon_in,crd_typ_out);

  /* Obtain input latitude type and length */
  char lat_nm_in[]="lat";
  (void)nco_inq_varid(in_id,lat_nm_in,&var_in_id);
  (void)nco_inq_var(in_id,var_in_id,(char *)NULL,&var_typ_in,&dmn_nbr,(int *)NULL,(int *)NULL);
  if(var_typ_in != NC_DOUBLE){
    assert(var_typ_in == NC_FLOAT);
    if(nco_dbg_lvl_get() >= nco_dbg_std) (void)fprintf(stderr,"%s: INFO WARNING Cheapskate regridding input file %s stores coordinate as NC_FLOAT\n",nco_prg_nm_get(),rgr->fl_in);
  } /* !var_typ_in */
  (void)nco_inq_vardimid(in_id,var_in_id,dmn_id);
  dmn_sz=1L;
  /* Get dimension sizes from input file */
  for(idx=0;idx<dmn_nbr;idx++){
    (void)nco_inq_dimlen(in_id,dmn_id[idx],dmn_cnt+idx);
    dmn_srt[idx]=0L;
    dmn_sz*=dmn_cnt[idx];
  } /* end loop over dim */
  max_idx[crd_idx_f_0bs_lat_src]=dmn_sz; /* [nbr] Number of elements in dimensions */

  /* Allocate space for and obtain latitude */
  lat_in=(double *)nco_malloc(dmn_sz*nco_typ_lng(crd_typ_out));
  rcd=nco_get_vara(in_id,var_in_id,dmn_srt,dmn_cnt,lat_in,crd_typ_out);

  /* Initialize before any other ESMC API calls!
     ESMC_ArgLast is ALWAYS at the end to indicate the end of opt args */
  ESMC_Initialize(&rcd_esmf,
		  ESMC_InitArgDefaultCalKind(ESMC_CALKIND_GREGORIAN),
		  ESMC_InitArgLogFilename(fl_nm_esmf_log),
		  ESMC_InitArgLogKindFlag(ESMC_LOGKIND_MULTI),
		  ESMC_ArgLast);
  enum ESMC_LogMsgType_Flag log_msg_typ=ESMC_LOGMSG_TRACE;
  if(rcd_esmf != ESMF_SUCCESS){
    ESMC_LogWrite("ESMC_Initialize() failed",log_msg_typ);
    goto rgr_cln;
  } /* endif */
  
  /* Set log to flush after every message */
  rcd_esmf=ESMC_LogSet(ESMF_TRUE);
  if(rcd_esmf != ESMF_SUCCESS) goto rgr_cln;
  /* Obtain VM */
  ESMC_VM vm;
  vm=ESMC_VMGetGlobal(&rcd_esmf);
  if(rcd_esmf != ESMF_SUCCESS) goto rgr_cln;

  /* Set-up local PET info */
  /* Source: ${DATA}/esmf/src/Infrastructure/VM/interface/ESMC_VM.C */
  rcd_esmf=ESMC_VMGet(vm,&localPet,&petCount,&peCount,(MPI_Comm *)NULL,&flg_pthreadsEnabledFlag,&flg_openMPEnabledFlag);
  if(rcd_esmf != ESMF_SUCCESS) goto rgr_cln;

  /* Create destination grid from SCRIP file set as rgr_grd_dst */
  int flg_isSphere=1; /* [flg] Set to 1 for a spherical grid, or 0 for regional. Defaults to 1. */
  int flg_addCornerStagger=0; /* [flg] Add corner stagger to grid. Defaults to 0. */
  int flg_addUserArea=0; /* [flg] Read cell area from Grid file (instead of calculate it). Defaults to 0. */
  int flg_addMask=0; /* [flg] Generate mask using missing value attribute in var_nm (iff GRIDSPEC) */
  /* 20150424: ESMF library bug at ESMCI_Grid.C line 365 means var_nm must non-NULL so set to blank name */
  char var_nm[]=""; /* [sng] Iff addMask == 1 use this variable's missing value attribute */
  char **crd_nm=NULL; /* [sng] Iff GRIDSPEC use these lon/lat variable coordinates */
  enum ESMC_FileFormat_Flag grd_fl_typ=ESMC_FILEFORMAT_SCRIP;
  ESMC_Grid grd_dst;
  grd_dst=ESMC_GridCreateFromFile(rgr->fl_grd_dst,grd_fl_typ,&flg_isSphere,&flg_addCornerStagger,&flg_addUserArea,&flg_addMask,var_nm,crd_nm,&rcd_esmf);
  if(rcd_esmf != ESMF_SUCCESS) goto rgr_cln;

  int *msk_dst; /* [enm] Destination grid mask */
  enum ESMC_GridItem_Flag grd_itm=ESMC_GRIDITEM_MASK;
  msk_dst=ESMC_GridGetItem(grd_dst,grd_itm,stg_lcn_dst,&rcd_esmf);
  if(rcd_esmf != ESMF_SUCCESS) goto rgr_cln;

  int *bnd_lwr_dst=(int *)nco_malloc(dmn_nbr_grd*sizeof(int));
  int *bnd_upr_dst=(int *)nco_malloc(dmn_nbr_grd*sizeof(int));
  double *lon_dst; /* [dgr] Destination grid longitude */
  double *lat_dst; /* [dgr] Destination grid latitude */
  /* 20150427: Written documentation is somewhat inadequate or misleading to normal C-programmers
     Some ESMC_Grid***() functions, like this one, return allocated void pointers that must be cast to desired numeric type
     Source: ${DATA}/esmf/src/Infrastructure/Grid/interface/ESMC_Grid.C */
  lon_dst=ESMC_GridGetCoord(grd_dst,crd_idx_f_1bs_lon_dst,stg_lcn_dst,bnd_lwr_dst,bnd_upr_dst,&rcd_esmf);
  if(rcd_esmf != ESMF_SUCCESS) goto rgr_cln;
  lat_dst=ESMC_GridGetCoord(grd_dst,crd_idx_f_1bs_lat_dst,stg_lcn_dst,bnd_lwr_dst,bnd_upr_dst,&rcd_esmf);
  if(rcd_esmf != ESMF_SUCCESS) goto rgr_cln;

  double *lon_out; /* [dgr] Longitude dimension for output file */
  double *lat_out; /* [dgr] Latitude  dimension for output file */
  lon_out=(double *)nco_malloc(bnd_upr_dst[crd_idx_f_0bs_lon_dst]*sizeof(double));
  lat_out=(double *)nco_malloc(bnd_upr_dst[crd_idx_f_0bs_lat_dst]*sizeof(double));
  for(idx=0;idx<bnd_upr_dst[crd_idx_f_0bs_lon_dst];idx++) lon_out[idx]=lon_dst[idx];
  for(idx=0;idx<bnd_upr_dst[crd_idx_f_0bs_lat_dst];idx++) lat_out[idx]=lat_dst[idx];
  const long var_sz_dst=bnd_upr_dst[crd_idx_f_0bs_lon_dst]*bnd_upr_dst[crd_idx_f_0bs_lat_dst]; /* [nbr] Number of elements in variable */
  for(idx=0;idx<var_sz_dst;idx++) msk_dst[idx]=0;

  /* Create source grid with same sizes as those in input data file */
  ESMC_InterfaceInt max_idx_src;
  /* Source: ${DATA}/esmf/src/Infrastructure/Util/interface/ESMC_Interface.C
     NB: ESMF is fragile in that dynamic memory provided as input to grids cannot be immediately freed
     In other words, ESMF copies the values of pointers but not the contents of pointers to provided arrays
     To be concrete, the max_idx array provided below cannot be freed until after the ESMC_Finalize() is called */
  max_idx_src=ESMC_InterfaceIntCreate(max_idx,dmn_nbr_grd,&rcd_esmf);
  if(rcd_esmf != ESMF_SUCCESS) goto rgr_cln;

  enum ESMC_CoordSys_Flag crd_sys=ESMC_COORDSYS_SPH_DEG; /* NB: dyw sez ESMF supports ESMC_COORDSYS_SPH_DEG only */
  enum ESMC_TypeKind_Flag typ_knd=ESMC_TYPEKIND_R8; /* NB: NCO default is to use double precision for coordinates */
  ESMC_Grid grd_src;
  /* Source: ${DATA}/esmf/src/Infrastructure/Grid/interface/ESMC_Grid.C */
  grd_src=ESMC_GridCreateNoPeriDim(max_idx_src,&crd_sys,&typ_knd,&rcd_esmf);
  if(rcd_esmf != ESMF_SUCCESS) goto rgr_cln;
  /* fxm: why destroy this now? */
  ESMC_InterfaceIntDestroy(&max_idx_src);
  /* Define stagger locations on source grid. Necessary for remapping later? */
  rcd_esmf=ESMC_GridAddCoord(grd_src,stg_lcn_src);
  if(rcd_esmf != ESMF_SUCCESS) goto rgr_cln;
  
  /* Allocate */
  int *bnd_lwr_src=(int *)nco_malloc(dmn_nbr_grd*sizeof(int));
  int *bnd_upr_src=(int *)nco_malloc(dmn_nbr_grd*sizeof(int));
  double *lon_src; /* [dgr] Source grid longitude */
  double *lat_src; /* [dgr] Source grid latitude  */
  lon_src=(double *)ESMC_GridGetCoord(grd_src,crd_idx_f_1bs_lon_src,stg_lcn_src,bnd_lwr_src,bnd_upr_src,&rcd_esmf);
  if(rcd_esmf != ESMF_SUCCESS) goto rgr_cln;
  lat_src=(double *)ESMC_GridGetCoord(grd_src,crd_idx_f_1bs_lat_src,stg_lcn_src,NULL,NULL,&rcd_esmf);
  if(rcd_esmf != ESMF_SUCCESS) goto rgr_cln;

  if(nco_dbg_lvl_get() >= nco_dbg_crr){
    (void)fprintf(stderr,"%s: INFO %s reports\n",nco_prg_nm_get(),fnc_nm);
    (void)fprintf(stderr,"ESMC_VMGet(): localPet = %d, petCount = %d, peCount = %d, pthreadsEnabledFlag = %s, openMPEnabledFlag = %s\n",localPet,petCount,peCount,flg_pthreadsEnabledFlag ? "Enabled" : "Disabled",flg_openMPEnabledFlag ? "Enabled" : "Disabled");
    (void)fprintf(stderr,"ESMC_GridCreateFromFile(): filename = %s, fileTypeFlag = %s, isSphere = %s, addCornerStagger = %s, addUserArea = %s, addMask = %s, var_nm = %s, crd_nm = %s\n",rgr->fl_grd_dst,nco_esmf_fl_fmt_sng(grd_fl_typ),flg_isSphere ? "Enabled" : "Disabled",flg_addCornerStagger ? "Enabled" : "Disabled",flg_addUserArea ? "Enabled" : "Disabled",flg_addMask ? "Enabled" : "Disabled",var_nm,crd_nm ? "non-NULL" : "NULL");
    (void)fprintf(stderr,"ESMC_GridGetCoord(): bnd_lwr_dst[0..1] = %d, %d; bnd_upr_dst[0..1] = %d, %d;\n",bnd_lwr_dst[0],bnd_lwr_dst[1],bnd_upr_dst[0],bnd_upr_dst[1]);
    (void)fprintf(stderr,"ESMC_InterfaceIntCreate(): No printable information\n");
    (void)fprintf(stderr,"ESMC_GridCreateNoPeriDim(): crd_sys = %s, typ_knd = %s\n",nco_esmf_crd_sys_sng(crd_sys),nco_esmf_typ_knd_sng(typ_knd));
    (void)fprintf(stderr,"ESMC_GridAddCoord(): stg_lcn_src = %s,  stg_lcn_dst = %s\n",nco_esmf_stg_lcn_sng(stg_lcn_src),nco_esmf_stg_lcn_sng(stg_lcn_dst));
    (void)fprintf(stderr,"ESMC_GridGetCoord(): bnd_lwr_src[0..1] = %d, %d; bnd_upr_src[0..1] = %d, %d;\n",bnd_lwr_src[0],bnd_lwr_src[1],bnd_upr_src[0],bnd_upr_src[1]);
    //    (void)fprintf(stderr,"ESMC_InterfaceIntCreate(): max_idx_src[0..1] = %d, %d\n",max_idx_src[0],max_idx_src[1]);
  } /* endif dbg */

  /* Type-conversion and cell-center coordinates */
  for(idx=0;idx<bnd_upr_src[crd_idx_f_0bs_lat_src];idx++) lat_src[idx]=lat_in[idx];
  for(idx=0;idx<bnd_upr_src[crd_idx_f_0bs_lon_src];idx++) lon_src[idx]=lon_in[idx];
  idx=0;
  for(int idx_lat=0;idx_lat<bnd_upr_src[crd_idx_f_0bs_lat_src];idx_lat++){
    for(int idx_lon=0;idx_lon<bnd_upr_src[crd_idx_f_0bs_lon_src];idx_lon++){
      lon_src[idx]=lon_in[idx_lon];
      lat_src[idx]=lat_in[idx_lat];
      idx++;
    } /* endfor */
  } /* endfor */
  
  /* Create source field from source grid */
  ESMC_Field fld_src;
  ESMC_InterfaceInt *gridToFieldMap=NULL; /* [idx] Map of all grid dimensions to field dimensions */
  ESMC_InterfaceInt *ungriddedLBound=NULL; /* [idx] Lower bounds of ungridded dimensions */
  ESMC_InterfaceInt *ungriddedUBound=NULL; /* [idx] Upper bounds of ungridded dimensions */
  fld_src=ESMC_FieldCreateGridTypeKind(grd_src,typ_knd,stg_lcn_src,gridToFieldMap,ungriddedLBound,ungriddedUBound,"fld_src",&rcd_esmf);
  if(rcd_esmf != ESMF_SUCCESS) goto rgr_cln;
  /* Create destination field from destination grid */
  ESMC_Field fld_dst;
  fld_dst=ESMC_FieldCreateGridTypeKind(grd_dst,typ_knd,stg_lcn_dst,gridToFieldMap,ungriddedLBound,ungriddedUBound,"fld_dst",&rcd_esmf);
  if(rcd_esmf != ESMF_SUCCESS) goto rgr_cln;

  /* Get field pointers */
  double *fld_src_ptr;
  int localDe=0; /* [idx] Local DE for which information is requested [0..localDeCount-1] */
  fld_src_ptr=(double *)ESMC_FieldGetPtr(fld_src,localDe,&rcd_esmf);
  if(rcd_esmf != ESMF_SUCCESS) goto rgr_cln;
  double *fld_dst_ptr;
  fld_dst_ptr=(double *)ESMC_FieldGetPtr(fld_dst,localDe,&rcd_esmf);
  if(rcd_esmf != ESMF_SUCCESS) goto rgr_cln;

  /* Get variables from input file */
  (void)nco_inq_varid(in_id,rgr->var_nm,&var_in_id);
  (void)nco_inq_var(in_id,var_in_id,(char *)NULL,&var_typ_in,&dmn_nbr,(int *)NULL,(int *)NULL);
  /* Get dimension IDs from input file */
  (void)nco_inq_vardimid(in_id,var_in_id,dmn_id);
  long var_sz_src=1L; /* [nbr] Number of elements in variable (will be self-multiplied) */
  /* Get input dimension sizes */
  for(idx=0;idx<dmn_nbr;idx++){
    (void)nco_inq_dimlen(in_id,dmn_id[idx],dmn_cnt+idx);
    dmn_srt[idx]=0L;
    var_sz_src*=dmn_cnt[idx];
  } /* end loop over dim */

  /* Allocate space for and obtain input latitude */
  void_ptr_var=(void *)nco_malloc_dbg(var_sz_src*nco_typ_lng(var_typ_in),"Unable to malloc() value buffer when copying hyperslab from input to output file",fnc_nm);
  rcd=nco_get_vara(in_id,var_in_id,dmn_srt,dmn_cnt,void_ptr_var,var_typ_in);
  float *var_fld=(float *)void_ptr_var;

  /* Type-conversion and ensure every cell has data */
  idx=0;
  for(int idx_lat=bnd_lwr_src[crd_idx_f_0bs_lat_src];idx_lat<=bnd_upr_src[crd_idx_f_0bs_lat_src];idx_lat++){
    for(int idx_lon=bnd_lwr_src[crd_idx_f_0bs_lon_src];idx_lon<=bnd_upr_src[crd_idx_f_0bs_lon_src];idx_lon++){
      fld_src_ptr[idx]=(double)var_fld[idx];
      idx++;
    } /* idx_lon */
  } /* idx_lat */

  /* Initialize dst data ptr */
  idx=0;
  for(int idx_lat=bnd_lwr_dst[crd_idx_f_0bs_lat_src];idx_lat<=bnd_upr_dst[crd_idx_f_0bs_lat_src];idx_lat++){
    for(int idx_lon=bnd_lwr_dst[crd_idx_f_0bs_lon_src];idx_lon<=bnd_upr_dst[crd_idx_f_0bs_lon_src];idx_lon++){
      fld_dst_ptr[idx]=0.0;
      idx++;
    } /* idx_lon */
  } /* idx_lat */

  ESMC_LogWrite("nco_rgr_esmf() invoking ESMC to start regridstore actions",log_msg_typ);
  /* int *msk_val=(int *)nco_malloc(sizeof(int));
     msk_val[0]=1;
     ESMC_InterfaceInt i_msk_val=ESMC_InterfaceIntCreate(msk_val,1,&rcd_esmf);
     rcd_esmf = ESMC_FieldRegridStore(fld_src,fld_dst,&i_msk_val,&i_msk_val,&rte_hnd,NULL,NULL,NULL,&unmap_act,NULL,NULL);
     rcd_esmf=ESMC_FieldRegridStore(fld_src,fld_dst,NULL,NULL,&rte_hnd,NULL,NULL,NULL,&unmap_act,NULL,NULL); */

  ESMC_Field *cll_frc_dst=NULL; /* [frc] Fraction of each cell participating in regridding, destination grid */
  ESMC_Field *cll_frc_src=NULL; /* [frc] Fraction of each cell participating in regridding, source grid */
  ESMC_InterfaceInt *msk_src_rgr=NULL; /* [idx] Points to mask while regridding, source grid */
  /* fxm: unsure whether/why need both msk_dst (above) and msk_dst_rgr (below) */
  ESMC_InterfaceInt *msk_dst_rgr=NULL; /* [idx] Points to mask while regridding, destination grid */
  ESMC_RouteHandle rte_hnd;
  enum ESMC_RegridMethod_Flag rgr_mth=ESMC_REGRIDMETHOD_BILINEAR; /* [flg] Regrid method (default bilinear) */
  enum ESMC_PoleMethod_Flag pll_mth=ESMC_POLEMETHOD_ALLAVG; /* [flg] Regrid method (default ESMC_POLEMETHOD_ALLAVG) */
  enum ESMC_UnmappedAction_Flag unm_act=ESMC_UNMAPPEDACTION_ERROR; /* [flg] Unmapped action (default ESMC_UNMAPPEDACTION_ERROR) */
  int pll_nbr=int_CEWI; /* [nbr] Number of points to average (iff ESMC_POLEMETHOD_NPNTAVG) */
  /* Source: ${DATA}/esmf/src/Infrastructure/Field/interface/ESMC_Field.C */
  rcd_esmf=ESMC_FieldRegridStore(fld_src,fld_dst,msk_src_rgr,msk_dst_rgr,&rte_hnd,&rgr_mth,&pll_mth,&pll_nbr,&unm_act,cll_frc_src,cll_frc_dst);
  if(rcd_esmf != ESMF_SUCCESS) goto rgr_cln;

  enum ESMC_Region_Flag rgn_flg=ESMC_REGION_TOTAL; /* [flg] Whether/how to zero input fields before regridding (default ESMC_REGION_TOTAL) */
  rcd_esmf=ESMC_FieldRegrid(fld_src,fld_dst,rte_hnd,&rgn_flg);
  if(rcd_esmf != ESMF_SUCCESS) goto rgr_cln;

  /* Call once on each PET */
  rcd_esmf=ESMC_Finalize();
  if(rcd_esmf != ESMF_SUCCESS) goto rgr_cln;

  /* Write regridded data to netCDF file */
  int var_out_id; /* [id] Variable ID */
  int lon_out_id; /* [id] Variable ID for longitude */
  int lat_out_id; /* [id] Variable ID for latitude  */
  int dmn_id_lat; /* [id] Dimension ID */
  int dmn_id_lon; /* [id] Dimension ID */
  char *lat_nm_out=lat_nm_in;
  char *lon_nm_out=lon_nm_in;
  (void)nco_def_dim(out_id,lat_nm_out,bnd_upr_dst[crd_idx_f_0bs_lat_dst],&dmn_id_lat);
  (void)nco_def_dim(out_id,lon_nm_out,bnd_upr_dst[crd_idx_f_0bs_lon_dst],&dmn_id_lon);
  int dmn_ids_out[2]; /* [id] Dimension IDs array for output variable */
  long dmn_srt_out[2];
  long dmn_cnt_out[2];
  dmn_ids_out[crd_idx_c_0bs_lat_dst]=dmn_id_lat;
  dmn_ids_out[crd_idx_c_0bs_lon_dst]=dmn_id_lon;
  (void)nco_def_var(out_id,lon_nm_out,crd_typ_out,(int)1,&dmn_id_lon,&lon_out_id);
  (void)nco_def_var(out_id,lat_nm_out,crd_typ_out,(int)1,&dmn_id_lat,&lat_out_id);
  (void)nco_def_var(out_id,rgr->var_nm,var_typ_out,(int)2,dmn_ids_out,&var_out_id);
  (void)nco_enddef(out_id);
  dmn_srt_out[crd_idx_c_0bs_lat_dst]=0L;
  dmn_srt_out[crd_idx_c_0bs_lon_dst]=0L;
  dmn_cnt_out[crd_idx_c_0bs_lat_dst]=bnd_upr_dst[crd_idx_f_0bs_lat_dst];
  dmn_cnt_out[crd_idx_c_0bs_lon_dst]=bnd_upr_dst[crd_idx_f_0bs_lon_dst];
  (void)nco_put_vara(out_id,lat_out_id,dmn_srt_out,dmn_cnt_out,lat_out,crd_typ_out);
  (void)nco_put_vara(out_id,lon_out_id,dmn_srt_out,dmn_cnt_out,lon_out,crd_typ_out);
  (void)nco_put_vara(out_id,var_out_id,dmn_srt_out,dmn_cnt_out,fld_dst_ptr,var_typ_out);

  /* Close output file and move it from temporary to permanent location */
  (void)nco_fl_out_cls(rgr->fl_out,rgr->fl_out_tmp,out_id);

rgr_cln:
  if(rcd_esmf != ESMF_SUCCESS){
    /* ESMF return codes are enumerated in ESMC_ReturnCodes.h and ESMC_LogErr.h
       However, there seems to be no translator function for these codes */
    (void)fprintf(stderr,"%s: ERROR %s received ESMF return code %d\nSee ESMC_ReturnCodes.h or ESMC_LogErr.h for more information, e.g.,\n/bin/more /usr/local/include/ESMC_ReturnCodes.h | grep %d\n",nco_prg_nm_get(),fnc_nm,rcd_esmf,rcd_esmf);
  } /* rcd_esmf */
  if(max_idx) max_idx=(int *)nco_free(max_idx);
  if(bnd_lwr_src) bnd_lwr_src=(int *)nco_free(bnd_lwr_src);
  if(bnd_upr_src) bnd_upr_src=(int *)nco_free(bnd_upr_src);
  if(bnd_lwr_dst) bnd_lwr_dst=(int *)nco_free(bnd_lwr_dst);
  if(bnd_upr_dst) bnd_upr_dst=(int *)nco_free(bnd_upr_dst);
  if(void_ptr_var) void_ptr_var=(void *)nco_free(void_ptr_var);
  if(lat_out) lat_out=(double *)nco_free(lat_out);
  if(lon_out) lon_out=(double *)nco_free(lon_out);
  if(dmn_cnt) dmn_cnt=(long *)nco_free(dmn_cnt);
  if(dmn_id) dmn_id=(int *)nco_free(dmn_id);
  if(dmn_srt) dmn_srt=(long *)nco_free(dmn_srt);

  rcd_esmf=ESMC_FieldRegridRelease(&rte_hnd);
  rcd_esmf=ESMC_FieldDestroy(&fld_src);
  rcd_esmf=ESMC_FieldDestroy(&fld_dst);
  rcd_esmf=ESMC_GridDestroy(&grd_src);
  rcd_esmf=ESMC_GridDestroy(&grd_dst);
  ESMC_Finalize();

  return rcd;
} /* nco_rgr_esmf */
#endif /* !ENABLE_ESMF */

