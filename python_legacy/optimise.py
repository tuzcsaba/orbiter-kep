from PyGMO import *
from PyKEP import *
import traceback
import sys
import logging
import matplotlib.pylab as plt

class optimizer:

  def __init__(self, prob, nTrial = 1, gen = 1000, mf = 100, mr = 1):
    self.problem = prob
    self.topology = topology.fully_connected()
    self.nIsl = 8
    self.population = 40

    self.nTrial = nTrial

    self.mf = mf
    self.mr = mr
    self.gen = gen

  def run_once(self, single_obj_result = None, print_fronts = False):

    de_variants = [11, 13, 15, 17]

    # algos = [algorithm.jde(gen = self.mf, variant = v, memory = True, variant_adptv = 2) for v in de_variants]
    if self.problem.f_dimension == 1:
      algos = [algorithm.jde(gen = self.mf, variant = 1, ftol=1e-01, memory = False)]
    else:
      algos = [algorithm.nsga_II(gen = self.mf)]

    try:
      best_x = None
      best_f = None

      cpu_count = self.nIsl
      
      selection = migration.best_s_policy(2, migration.rate_type.absolute)
      replacement = migration.fair_r_policy(2, migration.rate_type.absolute)
      
      for u in range(self.nTrial):
        archi = archipelago(topology=self.topology)
        if len(algos) > 1:
          i = 0
          while i < cpu_count:
            for algo in algos:
              archi.push_back(island(algo, self.problem, self.population, s_policy = selection, r_policy = replacement))
              i += 1
              if i >= cpu_count:
                break;
        else:
          i = 0
          if self.problem.f_dimension == 2:
            if single_obj_result != None:
              pop = population(self.problem, self.population - 1)
              pop.push_back(single_obj_result)
              isl = island(algos[0], pop, s_policy = selection, r_policy = replacement)
              archi.push_back(isl)
              i += 1
          while i < cpu_count:
              archi.push_back(island(algos[0], self.problem, self.population, s_policy = selection, r_policy = replacement))
              i += 1

        prev = None
        t = 0
        for i in range(self.gen / self.mf):
          archi.evolve(1)
          archi.join()

          if self.problem.f_dimension == 1:
            isl = min(archi, key=lambda x: x.population.champion.f[0])
   
            val = isl.population.champion.f[0]

            if best_f == None or isl.population.champion.f[0] < best_f:
              best_f = isl.population.champion.f[0]
              best_x = isl.population.champion.x
              print("Improved: %f m/s" % best_f )

            if prev != None and abs(val - prev) <= 1e-01:
              break
            

            prev = val


      if self.problem.f_dimension == 1:
        print("Done!! Best solution found is: " +
        str(best_f / 1000) + " km / sec")
    except Exception,e:
      logging.error(e, exc_info = True)
    finally:
      if print_fronts:
        master_pop = population(self.problem)
        for isl in archi:
          for ind in isl.population:
            master_pop.push_back(ind.cur_x)
        pop.plot_pareto_fronts()

        archi[0].population.plot_pareto_fronts(rgb=(1,0,0))
        plt.ylabel('total travelling time in days')
        plt.xlabel('total Delta-V in m/s')
        plt.show()

      return best_x
