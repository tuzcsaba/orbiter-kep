#include "pareto.h"

void population_plot_pareto_fronts(const pagmo::population &pop) {
  
  int p_dim = pop.problem().get_f_dimension();
  std::vector<std::vector<pagmo::population::size_type> > p_list = pop.compute_pareto_fronts();

  Gnuplot gp;

  std::vector<boost::tuple<double, double> > d;
  double min_x = DBL_MAX, max_x = 0;
  double min_y = DBL_MAX, max_y = 0;
  for (int i = 0; i < p_list.size(); ++i) {
    std::vector<pagmo::population::size_type> f = p_list[i];
    
    for (int j = 0; j < p_list[i].size(); ++j) {
      double x = pop.get_individual(f[j]).cur_f[0];
      double y = pop.get_individual(f[j]).cur_f[1];
      d.push_back(boost::make_tuple(x, y));

      if (x < min_x) min_x = x;
      if (x > max_x) max_x = x;

      if (y < min_y) min_y = y;
      if (y > max_y) max_y = y;
    }
  }

  gp << "set xrange [" << min_x << ":" << max_x << "]\nset yrange [" << min_y << ":" << max_y << "]\n"; 
  gp << "plot '-' with points ps 1 title 'Pareto Fronts'\n";
  gp.send1d(d);

}

