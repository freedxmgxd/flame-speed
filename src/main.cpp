
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#include "cantera/base/stringUtils.h"
#include "cantera/kinetics/Reaction.h"
#include "cantera/oneD/DomainFactory.h"
#include "cantera/onedim.h"
#include "cantera/thermo/Species.h"
#include "cantera/transport/TransportData.h"
#include "lib.h"

struct output {
  double ratio_fuel_ox;
  double speed_flame_reduced;
  double temperature_ad_reduced;
  double temperature_max_reduced;
  double z_t_max_reduced;
  double speed_flame_full;
  double temperature_ad_full;
  double temperature_max_full;
  double z_t_max_full;
};

int main(int argc, char** argv) {
  int loglevel         = 0;
  bool refine_grid     = true;
  auto tolerance_speed = 0.01;  // m/s

  auto phi             = 1.0;
  auto fuel            = "CH4";
  auto oxidizer        = "O2:1, N2:3.76";

  auto sol_complete    = Cantera::newSolution("gri30.yaml", "gri30", "mixture-averaged");

  auto gas             = sol_complete->thermo();
  gas->setEquivalenceRatio(phi, fuel, oxidizer);

  auto mixture_fraction_stoichiometric = gas->mixtureFraction(fuel, oxidizer);

  double temperature                   = 300.0;                  // K
  double pressure                      = 1.0 * Cantera::OneBar;  // Bar
  double uin                           = 0.3;                    // m/sec
  std::multimap<std::string, std::pair<std::string, double>> Reactions;

  auto flow_complete = flamespeed(sol_complete,
                                  temperature,
                                  pressure,
                                  uin,
                                  mixture_fraction_stoichiometric,
                                  fuel,
                                  oxidizer,
                                  refine_grid,
                                  loglevel,
                                  Reactions);

  std::cout << "Flame speed (complete mechanism): " << flow_complete.flamespeed << " m/s"
            << std::endl;
  std::cout << "Adiabatic flame temperature (complete mechanism): " << flow_complete.Tad << " K"
            << std::endl;

  std::string output_dir = "/home/Shinmen/Workspace Cloud/flame-speed/output";
  if (!std::filesystem::exists(output_dir)) {
    std::filesystem::create_directory(output_dir);
  }

  // Verify if file doesn't exist
  if (!std::filesystem::exists(output_dir + "/modified_mechanism.yaml")) {
    auto rootNode = mechanism_reduction(sol_complete,
                                        tolerance_speed,
                                        0.001,  // 0.1% tolerance for reaction rates
                                        flamespeed,
                                        temperature,
                                        pressure,
                                        uin,
                                        mixture_fraction_stoichiometric,
                                        fuel,
                                        oxidizer,
                                        refine_grid,
                                        loglevel);

    std::ofstream out(output_dir + "/modified_mechanism.yaml");
    out << rootNode.toYamlString();
  }

  std::vector<output> results;

  for (auto mixture_fraction = 0.00; mixture_fraction <= 0.20; mixture_fraction += 0.005) {
    auto sol_complete = Cantera::newSolution("gri30.yaml", "gri30", "mixture-averaged");

    std::cout << "Calculating for mixture fraction: " << mixture_fraction << std::endl;

    thermo_state flow_complete = {0.0, 0.0, 0.0, 0.0};
    try {
      flow_complete = flamespeed(sol_complete,
                                 temperature,
                                 pressure,
                                 uin,
                                 mixture_fraction,
                                 fuel,
                                 oxidizer,
                                 refine_grid,
                                 loglevel);
    } catch (Cantera::CanteraError& err) {
      std::cout << err.what() << std::endl;
      flow_complete = {0.0, 0.0, 0.0, 0.0};
    }

    auto sol_reduced =
        Cantera::newSolution(output_dir + "/modified_mechanism.yaml", "gri30", "mixture-averaged");

    thermo_state flow_reduced = {0.0, 0.0, 0.0, 0.0};
    try {
      flow_reduced = flamespeed(sol_reduced,
                                temperature,
                                pressure,
                                uin,
                                mixture_fraction,
                                fuel,
                                oxidizer,
                                refine_grid,
                                loglevel);
    } catch (Cantera::CanteraError& err) {
      std::cout << err.what() << std::endl;
      flow_reduced = {0.0, 0.0, 0.0, 0.0};
    }

    results.push_back({mixture_fraction,
                       flow_reduced.flamespeed,
                       flow_reduced.Tad,
                       flow_reduced.Tmax,
                       flow_reduced.zmax,
                       flow_complete.flamespeed,
                       flow_complete.Tad,
                       flow_complete.Tmax,
                       flow_complete.zmax});
  }

  // stechometric mixture fraction
  flow_complete = {0.0, 0.0, 0.0, 0.0};
  try {
    flow_complete = flamespeed(sol_complete,
                               temperature,
                               pressure,
                               uin,
                               mixture_fraction_stoichiometric,
                               fuel,
                               oxidizer,
                               refine_grid,
                               loglevel);
  } catch (Cantera::CanteraError& err) {
    std::cout << err.what() << std::endl;
    flow_complete = {0.0, 0.0, 0.0, 0.0};
  }

  auto sol_reduced =
      Cantera::newSolution(output_dir + "/modified_mechanism.yaml", "gri30", "mixture-averaged");

  thermo_state flow_reduced = {0.0, 0.0, 0.0, 0.0};
  try {
    flow_reduced = flamespeed(sol_reduced,
                              temperature,
                              pressure,
                              uin,
                              mixture_fraction_stoichiometric,
                              fuel,
                              oxidizer,
                              refine_grid,
                              loglevel);
  } catch (Cantera::CanteraError& err) {
    std::cout << err.what() << std::endl;
    flow_reduced = {0.0, 0.0, 0.0, 0.0};
  }

  results.push_back({mixture_fraction_stoichiometric,
                     flow_reduced.flamespeed,
                     flow_reduced.Tad,
                     flow_reduced.Tmax,
                     flow_reduced.zmax,
                     flow_complete.flamespeed,
                     flow_complete.Tad,
                     flow_complete.Tmax,
                     flow_complete.zmax});

  //  sort results by mixture fraction
  std::sort(results.begin(), results.end(), [](const output& a, const output& b) {
    return a.ratio_fuel_ox < b.ratio_fuel_ox;
  });

  auto num_reactions_reduced =
      Cantera::newSolution(output_dir + "/modified_mechanism.yaml", "gri30", "mixture-averaged")
          ->kinetics()
          ->nReactions();

  std::ofstream out_data(output_dir + "/flame_speed_data.csv");
  out_data << "Mixture fraction, Equivalence ratio (reduced mechanism " << num_reactions_reduced
           << "), "
              "Flame Speed (reduced mechanism "
           << num_reactions_reduced
           << ") [m/s], "
              "Adiabatic flame temperature (reduced mechanism "
           << num_reactions_reduced
           << ") [K], "
              "Maximum temperature (reduced mechanism "
           << num_reactions_reduced
           << ") [K], "
              "Z_max (reduced mechanism "
           << num_reactions_reduced
           << ") [m], "
              "Ignition delay time (reduced mechanism "
           << num_reactions_reduced
           << ") [s], "
              "Flame Speed (complete mechanism) [m/s], "
              "Adiabatic flame temperature (complete mechanism) [K], "
              "Maximum temperature (complete mechanism) [K], "
              "Z_max (complete mechanism) [m], "
              "Ignition delay time (complete mechanism) [s]\n";
  for (auto r : results) {
    auto new_phi                     = r.ratio_fuel_ox / (mixture_fraction_stoichiometric);

    auto ignition_delay_time_reduced = r.z_t_max_reduced / (r.speed_flame_reduced);
    auto ignition_delay_time_full    = r.z_t_max_full / (r.speed_flame_full);

    out_data << r.ratio_fuel_ox << "," << new_phi << "," << r.speed_flame_reduced << ","
             << r.temperature_ad_reduced << "," << r.temperature_max_reduced << ","
             << r.z_t_max_reduced << "," << ignition_delay_time_reduced << "," << r.speed_flame_full
             << "," << r.temperature_ad_full << "," << r.temperature_max_full << ","
             << r.z_t_max_full << "," << ignition_delay_time_full << "\n";
  }

  std::cout << "Data written to flame_speed_data.csv" << std::endl;

  return 0;
}
