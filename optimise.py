from PyGMO import *
from PyKEP import *
import traceback
import sys
import logging

class optimizer:

  def __init__(self, problem, nTrial = 1):
    self.problem = problem
    self.topology = topology.ring()
    self.nIsl = 4
    self.population = 40

    self.nTrial = nTrial

    self.mf = 100
    self.mr = 2
    self.gen = 1000

  def run_once(self):

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
      for u in range(1):
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
          for i in range(cpu_count):
              archi.push_back(island(algos[0], self.problem, self.population, s_policy = selection, r_policy = replacement))

        prev = None
        t = 0
        for k in range(self.nTrial):
          for i in range(self.gen / self.mf):
            archi.evolve(1)
            archi.join()

            isl = min(archi, key=lambda x: x.population.champion.f[0])
   
            val = isl.population.champion.f[0]

            if best_f == None or isl.population.champion.f[0] < best_f:
              best_f = isl.population.champion.f[0]
              best_x = isl.population.champion.x

            if prev != None and abs(val - prev) <= 1e-01:
              break

            prev = val
            print(best_f)

        # prob.exclude(best_x)


      print("Done!! Best solution found is: " +
      str(best_f / 1000) + " km / sec")
    except Exception,e:
      logging.error(e, exc_info = True)
    finally:
      return best_x
