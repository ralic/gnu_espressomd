#ifndef LJ_H
#define LJ_H

/** \file lj.h
 *  Routines to calculate the lennard jones energy or/and and force 
 *  for a particle pair.
 *  \ref forces.c
 *
 *  <b>Responsible:</b>
 *  <a href="mailto:limbach@mpip-mainz.mpg.de">Hanjo</a>
*/

MDINLINE void add_lj_pair_force(Particle *p1, Particle *p2, IA_parameters *ia_params,
				double d[3], double dist)
{
  int j;
  double r_off, frac2, frac6, fac=0.0;

  if(dist < ia_params->LJ_cut+ia_params->LJ_offset) {
    r_off = dist - ia_params->LJ_offset;
    /* normal case: resulting force/energy smaller than capping. */
    if(r_off > ia_params->LJ_capradius) {
      frac2 = SQR(ia_params->LJ_sig/r_off);
      frac6 = frac2*frac2*frac2;
      fac   = 48.0 * ia_params->LJ_eps * frac6*(frac6 - 0.5)*frac2 * (r_off/dist);
      for(j=0;j<3;j++) {
	p1->f[j] += fac * d[j];
	p2->f[j] -= fac * d[j];
      }
      if(fac*dist*0.5*time_step*time_step > 3e-6) fprintf(stderr,"%d: LJ-Warning: Pair (%d-%d) force=%f dist=%f\n",
				  this_node,p1->r.identity,p2->r.identity,fac*dist,dist);
    }
    /* capped part of lj potential. */
    else if(dist > 0.0) {
      frac2 = SQR(ia_params->LJ_sig/ia_params->LJ_capradius);
      frac6 = frac2*frac2*frac2;
      fac   = 48.0 * ia_params->LJ_eps * frac6*(frac6 - 0.5)*frac2 *(ia_params->LJ_capradius/dist);
      for(j=0;j<3;j++) {
	/* vector d is rescaled to length LJ_capradius */
	p1->f[j] += fac * d[j];
	p2->f[j] -= fac * d[j];
      }
    }
    /* this should not happen! */
    else {
      fprintf(stderr, "%d: Lennard-Jones warning: Particles id1=%d id2=%d exactly on top of each other\n",
	      this_node,p1->r.identity,p2->r.identity);

      frac2 = SQR(ia_params->LJ_sig/ia_params->LJ_capradius);
      frac6 = frac2*frac2*frac2;
      fac   = 48.0 * ia_params->LJ_eps * frac6*(frac6 - 0.5)*frac2;

      p1->f[0] += fac * ia_params->LJ_capradius;
      p2->f[0] -= fac * ia_params->LJ_capradius;

    }
    LJ_TRACE(fprintf(stderr,"%d: LJ: Pair (%d-%d) dist=%.3f: force+-: (%.3e,%.3e,%.3e)\n",
		     this_node,p1->r.identity,p2->r.identity,dist,fac*d[0],fac*d[1],fac*d[2]));
  }
}

/** calculate lj_capradius from lj_force_cap */
MDINLINE void calc_lj_cap_radii(double force_cap)
{
  int i,j,cnt=0;
  IA_parameters *params;
  double force=0.0, rad=0.0, step, frac2, frac6;

  for(i=0; i<n_particle_types; i++) {
    for(j=0; j<n_particle_types; j++) {
      params = get_ia_param(i,j);
      if(force_cap > 0.0) {
	/* I think we have to solve this numerically... and very crude as well */
	cnt=0;
	rad = params->LJ_sig;
	step = -0.1 * params->LJ_sig;
	force=0.0;
	
	while(step != 0) {
	  frac2 = SQR(params->LJ_sig/rad);
	  frac6 = frac2*frac2*frac2;
	  force = 48.0 * params->LJ_eps * frac6*(frac6 - 0.5)*frac2*rad;
	  if((step < 0 && force_cap < force) || (step > 0 && force_cap > force)) {
	    step = - (step/2.0); 
	  }
	  if(fabs(force-force_cap) < 1.0e-6) step=0;
	  rad += step; cnt++;
	} 
      	params->LJ_capradius = rad;
      }
      else {
	params->LJ_capradius = 0.0; 
      }
      FORCE_TRACE(fprintf(stderr,"%d: Ptypes %d-%d have cap_radius %f and cap_force %f (iterations: %d)\n",
			  this_node,i,j,rad,force,cnt));
    }
  }

}

#endif
