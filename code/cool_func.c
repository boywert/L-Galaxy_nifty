#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "allvars.h"
#include "proto.h"


#define TABSIZE 91


static char *name[] = {
  "stripped_mzero.cie",
  "stripped_m-30.cie",
  "stripped_m-20.cie",
  "stripped_m-15.cie",
  "stripped_m-10.cie",
  "stripped_m-05.cie",
  "stripped_m-00.cie",
  "stripped_m+05.cie"
};


/* metallicies with repect to solar. Will be converted to
   absolut metallicities by adding  log10(Z_sun), Zsun=0.02 */

static double metallicities[8] = {
  -5.0,                         /* actually primordial -> -infinity */
  -3.0,
  -2.0,
  -1.5,
  -1.0,
  -0.5,
  +0.0,
  +0.5
};


static double CoolRate[8][TABSIZE];

/**@file cool_cunf.c reads the gas cooling functions*/
void read_cooling_functions(void)
{
  FILE *fd;
  char buf[200];
  int i, n;
  float sd_logT, sd_ne, sd_nh, sd_nt, sd_logLnet,
    sd_logLnorm, sd_logU, sd_logTau, sd_logP12, sd_logRho24, sd_ci, sd_mubar;

  for(i = 0; i < 8; i++)
    metallicities[i] += log10(0.02);    /* add solar metallicity */

  for(i = 0; i < 8; i++)
    {
      sprintf(buf, "%s/%s",CoolFunctionsDir,name[i]);

      if(!(fd = fopen(buf, "r")))
		{
		  char sbuf[1000];
		  sprintf(sbuf, "file `%s' not found.\n", buf);
		  terminate(sbuf);
		}

      for(n = 0; n <= 90; n++)
        {
          fscanf(fd, " %f %f %f %f %f %f %f %f %f %f %f %f ",
                 &sd_logT, &sd_ne, &sd_nh, &sd_nt,
                 &sd_logLnet, &sd_logLnorm, &sd_logU,
                 &sd_logTau, &sd_logP12, &sd_logRho24, &sd_ci, &sd_mubar);

          CoolRate[i][n] = sd_logLnorm;
        }

      fclose(fd);
    }

#ifdef PARALLEL
  if(ThisTask == 0)
    printf("cooling functions read.\n\n");
#else
  printf("cooling functions read.\n\n");
#endif

}




/* pass: log10(temperatue/Kelvin), log10(metallicity) */

double get_metaldependent_cooling_rate(double logTemp, double logZ)
{
  int i;
  double get_rate(int tab, double logTemp);
  double rate1, rate2, rate;


  if(logZ < metallicities[0])
    logZ = metallicities[0];

  if(logZ > metallicities[7])
    logZ = metallicities[7];

  i = 0;
  while(logZ > metallicities[i + 1])
    {
      i++;
    }
  /* look up at i and i+1 */


  rate1 = get_rate(i, logTemp);
  rate2 = get_rate(i + 1, logTemp);

  rate = rate1 + (rate2 - rate1) / (metallicities[i + 1] - metallicities[i]) * (logZ - metallicities[i]);

  return pow(10, rate);
}


double get_rate(int tab, double logTemp)
{
  int index;
  double rate1, rate2, rate, logTindex;


  if(logTemp < 4.0)
    logTemp = 4.0;

  index = (logTemp - 4.0) / 0.05;
  if(index >= 90)
    index = 89;

  logTindex = 4.0 + 0.05 * index;

  rate1 = CoolRate[tab][index];
  rate2 = CoolRate[tab][index + 1];

  rate = rate1 + (rate2 - rate1) / (0.05) * (logTemp - logTindex);

  return rate;
}

void test(void)
{
  double z;

  read_cooling_functions();

  for(z = -8.0; z < 2.0; z += 0.5)
    printf("z=%g\t rate= %g\n", z, log10(get_metaldependent_cooling_rate(1.0, z)));
}
