/* Generated by Together */
#ifndef ORDEREDTASK_H
#define ORDEREDTASK_H

#include "Tasks/AbstractTask.h"
#include "Tasks/TaskStats.hpp"
#include "BaseTask/OrderedTaskPoint.hpp"
#include "BaseTask/StartPoint.hpp"
#include "BaseTask/TaskLeg.h"
#include <vector>
#include "Util/Serialisable.hpp"
#include "GlideSolvers/MacCready.hpp"

class OrderedTask:
  public AbstractTask,
  public Serialisable
{
public:
  OrderedTask();
  ~OrderedTask();

  std::vector<TaskLeg*> legs;

  TaskPoint* getActiveTaskPoint();
  virtual void setActiveTaskPoint(unsigned);

  void insert(OrderedTaskPoint*, unsigned position);
  void remove(unsigned position);

  virtual bool update_sample(const AIRCRAFT_STATE &, const AIRCRAFT_STATE&);

  double get_distance_nominal() const {
    return distance_nominal;
  };
  double get_distance_min() const {
    return distance_min;
  };
  double get_distance_max() const {
    return distance_max;
  };
  double get_distance_scored() const {
    return distance_scored;
  };
  void report(const AIRCRAFT_STATE &state);

  const TaskStats& get_stats() const {
    return stats;
  }
  unsigned task_size() const {
    return tps.size();
  }

  // these used by task dijkstra
  const SearchPointVector& get_tp_search_points(unsigned tp) const {
    return tps[tp]->get_search_points();
  }
  void set_tp_search_min(unsigned tp, const SearchPoint &sol) {
    tps[tp]->set_search_min(sol);
  }
  void set_tp_search_max(unsigned tp, const SearchPoint &sol) {
    tps[tp]->set_search_max(sol);
  }
private:
  TaskStats stats;
  std::vector<OrderedTaskPoint*> tps;

  double distance_nominal;
  double distance_min;
  double distance_max;
  double distance_scored;

  virtual void scan_distance(const GEOPOINT &location, bool full);
  void update_geometry();

  /**
   * @supplierCardinality 1 
   */
  StartPoint *ts;

  TaskProjection task_projection;

  void glide_solution_remaining(const AIRCRAFT_STATE &, 
                                const double mc);
  void glide_solution_travelled(const AIRCRAFT_STATE &, 
                                const double mc);
  void glide_solution_planned(const AIRCRAFT_STATE &, 
                              const double mc);

  double calc_mc_best(const AIRCRAFT_STATE &, 
                      const double mc);
  double calc_cruise_efficiency(const AIRCRAFT_STATE &aircraft, 
                                const double mc);

};

#endif //ORDEREDTASK_H
