use orbiter_kep

db.solutions.aggregate({
  $group: {
    _id: { 
      hash: "$param_hash", 
      problem: "$problem",
      planets: "$planets", 
      t0: "$t0",
      tof: "$tof",
      eject_altitude: "$eject_altitude",
      target_altitude: "$target_altitude",
    }, 
   best_burn: {
      $min: "$delta-v"
    }, 
    worst_burn: {
      $max: "$delta-v"
    }, 
    avg_burn: {
      $avg: "$delta-v"
    }
  }
}).pretty()
