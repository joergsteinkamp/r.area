
/****************************************************************************
 *
 * MODULE:       r.area
 * AUTHOR(S):    Joerg Steinkamp - joergsteinkamp (at) yahoo.de
 *               based on the r.example by Markus Neteler
 * PURPOSE:      This programm calculates the fractional or total
 *               area per raster map/gridcells.
 * COPYRIGHT:    (C) Joerg Steinkamp
 *
 *               This program is free software under the GNU General Public
 *   	    	 License (>=v2). Read the file COPYING that comes with GRASS
 *   	    	 for details.
 *
 *****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/glocale.h>

/* 
 * global function declaration 
 */
extern CELL f_c(CELL);
extern FCELL f_f(FCELL);
extern DCELL f_d(DCELL);

/*
 * function definitions 
 */

/*
 * main function
 */
int main(int argc, char *argv[])
{
  struct Cell_head cellhd;	/* it stores region information,
				   and header information of rasters */
  char *inname, *maskname;  /* input raster and mask name */
  char *result;		          /* output raster name */
  char *mapset;		          /* mapset name */
  void *inrast, *inmask;    /* input buffer */
  unsigned char *outrast;	  /* output buffer */
  int nrows, ncols;
  int row, col;
  int infd, maskfd, outfd;	/* file descriptor */
  int verbose;
  int outputdesired;        /* 1 if output is desired else 0*/  
  int maskexist;            /* 1 if mask defined else 0 */
  int beg_cell_area;        /* calculation of cellarea possible */ 
  double cellarea;          /* area of cell im m^2*/
  double def_out_val;       /* null() of 0 as default value for output map */
  double tsum;              /* total sum over gridcells */
  double fscale;            /* scaling factor for total sum*/
  RASTER_MAP_TYPE in_type, mask_type, out_type;	/* type of the map (CELL/DCELL/...) */
  struct History history;	  /* holds meta-data (title, comments,..) */

  struct GModule *module;	/* GRASS module for parsing arguments */

  struct Option *input, *mask, *field, *scale, *output;	/* options */
  struct Flag *flagq, *flagf, *flagn, *flaga;	/* flags */

  /* initialize GIS environment */
  G_gisinit(argv[0]);		/* reads grass env, stores program name to G_program_name() */

  /* initialize module */
  module = G_define_module();
  module->keywords = _("raster, area, gridcell");
  module->description = _("Area calculation of raster gridcells");

  /* Define the different options as defined in gis.h */
  input = G_define_standard_option(G_OPT_R_INPUT);

  /* output raster, only if given */
  //output   = G_define_standard_option(G_OPT_R_OUTPUT);
  output = G_define_option() ;
  output->key = "output";
  output->type = TYPE_STRING;
  output->key_desc = "name";
  output->required = NO;
  output->gisprompt = "new,cell,raster";
  output->description = _("Name for output raster map");
  outputdesired = 0;
  if (output->answer)
    outputdesired = 1;

  out_type = DCELL_TYPE;
  def_out_val = 0.0;

  /* mask */
  mask = G_define_option() ;
  mask->key        = "mask";
  mask->type       = TYPE_STRING;
  mask->required   = NO;
  mask->description= "Name of an existing raster map, to be used as mask" ;

  /* which value of the previous raster mask should be used */
  field = G_define_option() ;
  field->key        = "field";
  field->type       = TYPE_INTEGER;
  field->required   = NO;
  //field->answer     = "1";
  //field->options    = "0-99999";
  field->description= "field of input mask to use as mask (required if mask is defined)" ;
  if(mask->answer && !field->answer)
    G_fatal_error(_("option field must be given when raster mask is given"));

  /* multiplicator for result */
  scale = G_define_option() ;
  scale->key        = "scale";
  scale->type       = TYPE_DOUBLE;
  scale->required   = NO;
  scale->answer     = "1";
  //scale->options    = "0-9999999999999999";
  scale->description= "scaling factor for output" ;

  /* Define the different flags */
  flagq = G_define_flag();
  flagq->key = 'q';
  flagq->description = _("Quiet");

  flagf = G_define_flag();
  flagf->key = 'f';
  flagf->description = _("use FCELL instead of DCELL for output");
  if (flagf->answer)
    out_type = FCELL_TYPE;

  flagn = G_define_flag();
  flagn->key = 'n';
  flagn->description = _("use null() instead of 0.0 as default output (only usefull with mask)");
  if (flagn->answer)
    def_out_val = strtod("NAN(char-sequence)", NULL);

  flaga = G_define_flag();
  flaga->key = 'a';
  flaga->description = _("multiply cell value by area in m^2");
  cellarea = 1.0;

  /* options and flags parser */
  if (G_parser(argc, argv))
    exit(EXIT_FAILURE);

  /* stores options and flags to variables */
  inname   = input->answer;
  maskname = mask->answer;
  result   = output->answer;
  verbose  = (!flagq->answer);

  /* returns NULL if the map was not found in any mapset, 
   * mapset name otherwise */
  maskexist = 0;
  if (maskname) {
    mapset = G_find_cell2(maskname, "");
    if (mapset != NULL)
      maskexist = 1;
  }

  mapset = G_find_cell2(inname, "");
  if (mapset == NULL)
    G_fatal_error(_("Raster map <%s> not found"), inname);

  if (outputdesired && G_legal_filename(result) < 0)
    G_fatal_error(_("<%s> is an illegal file name"), result);

  /* determine the inputmap type (CELL/FCELL/DCELL) */
  in_type = G_raster_map_type(inname, mapset);

  /* G_open_cell_old - returns file destriptor (>0) */
  if ((infd = G_open_cell_old(inname, mapset)) < 0)
    G_fatal_error(_("Unable to open raster map <%s>"), inname);

  /* controlling, if we can open input raster */
  if (G_get_cellhd(inname, mapset, &cellhd) < 0)
    G_fatal_error(_("Unable to read file header of <%s>"), inname);

  if (maskexist) {
    mask_type = G_raster_map_type(maskname, mapset);
    if (mask_type != CELL_TYPE)
      G_fatal_error(_("Raster map <%s> must be of type CELL"), maskname);
    if ((maskfd = G_open_cell_old(maskname, mapset)) < 0)
      G_fatal_error(_("Unable to open raster map <%s>"), maskname);
    if (G_get_cellhd(maskname, mapset, &cellhd) < 0)
      G_fatal_error(_("Unable to read file header of <%s>"), maskname);
  }

  G_debug(3, "number of rows %d", cellhd.rows);

  /* Allocate input buffer */
  inrast = G_allocate_raster_buf(in_type);

  if (maskexist)
    inmask = G_allocate_raster_buf(mask_type);

  /* Allocate output buffer, use input map data_type */
  nrows = G_window_rows();
  ncols = G_window_cols();
  outrast = G_allocate_raster_buf(out_type);

  /* controlling, if we can write the raster */
  if (outputdesired && (outfd = G_open_raster_new(result, out_type)) < 0)
    G_fatal_error(_("Unable to create raster map <%s>"), result);

  beg_cell_area = G_begin_cell_area_calculations();

  if (beg_cell_area == 0)
    G_fatal_error(_("Cell size can not be measured"));

  if (beg_cell_area == 1)
    G_fatal_error(_("Cell size is the same for all cells"));

  if (beg_cell_area != 2)
    G_fatal_error(_("Cell size error"));

  tsum = 0.0;

  /* for each row */
  for (row = 0; row < nrows; row++) {
    CELL c;
    FCELL f;
    DCELL d;
    CELL m;

    if (verbose)
      G_percent(row, nrows, 2);

    if (flaga->answer)
      cellarea = G_area_of_cell_at_row(row);

    /* read input map */
    if (G_get_raster_row(infd, inrast, row, in_type) < 0)
      G_fatal_error(_("Unable to read raster map <%s> row %d"), inname,
		    row);
    if (maskexist)
      if (G_get_raster_row(maskfd, inmask, row, CELL_TYPE) < 0)
	G_fatal_error(_("Unable to read raster map <%s> row %d"), maskname,
		      row);

    /* process the data */
    for (col = 0; col < ncols; col++) {
      /* use different function for each data type */
      if (flagf->answer) {
	((FCELL *) outrast)[col] = def_out_val;
      } else {
	((DCELL *) outrast)[col] = def_out_val;
      }
      if (maskexist) {
	m = ((CELL *) inmask)[col];
	if (m != atoi(field->answer))
	  continue;
      }

      switch (in_type) {
      case CELL_TYPE:
	c = ((CELL *) inrast)[col];
	tsum += c*cellarea;
	if (flagf->answer) {
	  ((FCELL *) outrast)[col] = c*cellarea;
	} else {
	  ((DCELL *) outrast)[col] = c*cellarea;
	}
	break;
      case FCELL_TYPE:
	f = ((FCELL *) inrast)[col];
	if (isnan(f)) break;
	tsum += f*cellarea;
	if (flagf->answer) {
	  ((FCELL *) outrast)[col] = f*cellarea;
	} else {
	  ((DCELL *) outrast)[col] = f*cellarea;
	}
	break;
      case DCELL_TYPE:
	d = ((DCELL *) inrast)[col];
	if (isnan(d)) break;
	tsum += d*cellarea;
	if (flagf->answer) {
	  ((FCELL *) outrast)[col] = d*cellarea;
	} else {
	  ((DCELL *) outrast)[col] = d*cellarea;
	}
	break;
      }
    }

    /* write raster row to output raster map */
    if (outputdesired && G_put_raster_row(outfd, outrast, out_type) < 0)
      G_fatal_error(_("Failed writing raster map <%s>"), result);
  }

  fscale = atof(scale->answer);
  printf("%f\n", tsum * fscale);

  /* memory cleanup */
  G_free(inrast);
  G_free(inmask);
  if(outputdesired)
    G_free(outrast);

  /* closing raster maps */
  G_close_cell(infd);
  G_close_cell(maskfd);
  if(outputdesired) {
    G_close_cell(outfd);

  /* add command line incantation to history file */
    G_short_history(result, "raster", &history);
    G_command_history(&history);
    G_write_history(result, &history);
  }

  exit(EXIT_SUCCESS);
}
