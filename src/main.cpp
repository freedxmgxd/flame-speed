/*
 * Freely-propagating flame
 * ========================
 *
 * C++ demo program to compute flame speeds using GRI-Mech.
 *
 * Usage:
 *
 *     flamespeed [equivalence_ratio] [refine_grid] [loglevel]
 *
 * Requires: cantera >= 3.1
 *
 * .. tags:: C++, combustion, 1D flow, premixed flame, flame speed, saving
 * output
 */

// This file is part of Cantera. See License.txt in the top-level directory or
// at https://cantera.org/license.txt for license and copyright information.

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
#include "imgui.h"
#include "implot.h"
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
  auto oxidizer        = "O2:0.21,N2:0.79";

  auto sol_complete    = Cantera::newSolution("gri30.yaml", "gri30", "mixture-averaged");

  auto gas             = sol_complete->thermo();
  gas->setEquivalenceRatio(phi, fuel, oxidizer);

  auto mixture_fraction_stoichiometric = gas->mixtureFraction(fuel, oxidizer);

  double temperature                   = 300.0;                  // K
  double pressure                      = 1.0 * Cantera::OneAtm;  // atm
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
  // Verify if file doesn't exist
  if (!std::filesystem::exists("/home/Shinmen/Workspace "
                               "Cloud/flame-speed/modified_mechanism.yaml")) {
    auto rootNode = mechanism_reduction(sol_complete,
                                        tolerance_speed,
                                        flamespeed,
                                        temperature,
                                        pressure,
                                        uin,
                                        mixture_fraction_stoichiometric,
                                        fuel,
                                        oxidizer,
                                        refine_grid,
                                        loglevel);

    std::ofstream out("/home/Shinmen/Workspace Cloud/flame-speed/modified_mechanism.yaml");
    out << rootNode.toYamlString();
  }

  std::vector<output> results;

  for (auto mixture_fraction = 0; mixture_fraction <= 100; mixture_fraction++) {
    auto sol_complete          = Cantera::newSolution("gri30.yaml", "gri30", "mixture-averaged");

    thermo_state flow_complete = {-1.0, -1.0, -1.0, -1.0};
    try {
      flow_complete = flamespeed(sol_complete,
                                 temperature,
                                 pressure,
                                 uin,
                                 mixture_fraction / 100.0,
                                 fuel,
                                 oxidizer,
                                 refine_grid,
                                 loglevel);
    } catch (Cantera::CanteraError& err) {
      std::cout << err.what() << std::endl;
      flow_complete = {-1.0, -1.0, -1.0, -1.0};
    }

    auto sol_reduced =
        Cantera::newSolution("/home/Shinmen/Workspace Cloud/flame-speed/modified_mechanism.yaml",
                             "gri30",
                             "mixture-averaged");

    thermo_state flow_reduced = {-1.0, -1.0, -1.0, -1.0};
    try {
      flow_reduced = flamespeed(sol_reduced,
                                temperature,
                                pressure,
                                uin,
                                mixture_fraction / 100.0,
                                fuel,
                                oxidizer,
                                refine_grid,
                                loglevel);
    } catch (Cantera::CanteraError& err) {
      std::cout << err.what() << std::endl;
      flow_reduced = {-1.0, -1.0, -1.0, -1.0};
    }

    results.push_back({mixture_fraction / 100.0,
                       flow_reduced.flamespeed,
                       flow_reduced.Tad,
                       flow_reduced.Tmax,
                       flow_reduced.zmax,
                       flow_complete.flamespeed,
                       flow_complete.Tad,
                       flow_complete.Tmax,
                       flow_complete.zmax});
  }

  std::ofstream out_data("/home/Shinmen/Workspace Cloud/flame-speed/flame_speed_data.csv");
  out_data << "Mixture fraction, Equivalence ratio (reduced mechanism), "
              "Flame Speed (reduced mechanism) [m/s], "
              "Adiabatic flame temperature (reduced mechanism) [K], "
              "Maximum temperature (reduced mechanism) [K], "
              "Z_max (reduced mechanism) [m], "
              "Flame Speed (complete mechanism) [m/s], "
              "Adiabatic flame temperature (complete mechanism) [K], "
              "Maximum temperature (complete mechanism) [K], "
              "Z_max (complete mechanism) [m]\n";
  for (auto r : results) {
    auto new_phi = r.ratio_fuel_ox / (mixture_fraction_stoichiometric);
    out_data << r.ratio_fuel_ox << "," << new_phi << "," << r.speed_flame_reduced << ","
             << r.temperature_ad_reduced << "," << r.temperature_max_reduced << ","
             << r.z_t_max_reduced << "," << r.speed_flame_full << "," << r.temperature_ad_full
             << "," << r.temperature_max_full << "," << r.z_t_max_full << "\n";
  }

  std::cout << "Data written to flame_speed_data.csv" << std::endl;

  return 0;
}
