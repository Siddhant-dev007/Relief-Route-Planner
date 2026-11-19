#ifndef DATASET_IO_H
#define DATASET_IO_H

#include <stddef.h>

#include "relief_model.h"
#include "relief_planner.h"

/*
 * Scenario file format (CSV with sections):
 *
 *   # comment
 *   NAME,Normal Relief Operations
 *   NODES,8
 *   DEPOT,0
 *   [SHELTERS]   id,name,population,urgency,node
 *   [PACKAGES]   id,type,weight,severity,shelterId
 *   [VEHICLES]   id,capacity,available
 *   [ROADS]      from,to,distance
 *
 * A line inside a section whose first field is not a number is
 * treated as a column header and skipped.
 */

// Loads, validates and finalises a scenario. On failure, reason
// holds the file line number and the problem.
Status loadScenarioCSV(const char path[], Scenario *scenario,
                       char reason[], size_t size);

// Exports write into the given path; return STATUS_OK or FILE_ERROR.
Status exportPlanCSV(const Scenario *scenario, const Plan *plan,
                     const char path[]);
Status exportRoutesCSV(const Scenario *scenario, const Plan *plan,
                       const char path[]);
Status exportComparisonCSV(const PlanComparison *comparison,
                           const char path[]);

#endif
