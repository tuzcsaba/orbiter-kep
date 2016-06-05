#ifndef PAGMO_PLOT_MATHGL_H
#define PAGMO_PLOT_MATHGL_H

#include "OrbiterKEPConfig.h"

#ifdef BUILD_PLOT

#include <string>

#include <pagmo/population.h>
#include <gnuplot-iostream.h>

void population_plot_pareto_fronts(const pagmo::population &pop, double maxDeltaV);
#endif

#endif
