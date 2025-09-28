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

#include "cantera/base/stringUtils.h"
#include "cantera/kinetics/Reaction.h"
#include "cantera/oneD/DomainFactory.h"
#include "cantera/onedim.h"
#include "cantera/thermo/Species.h"
#include "cantera/transport/TransportData.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <utility>

#include "lib.h"
#include <vector>

int main(int argc, char **argv) {

  Cantera::suppress_deprecation_warnings();
  Cantera::suppress_thermo_warnings(true);

  // Cantera::Application::Instance()->suppress_warnings();
  // Cantera::Application::Instance()->suppress_deprecation_warnings();

  double phi = 1.0;
  int loglevel = 0;
  bool refine_grid = true;
  auto tolerance_speed = 0.01; // m/s

  std::multimap<std::string, std::pair<std::string, double>> Reactions;

  auto sol_complete =
      Cantera::newSolution("gri30.yaml", "gri30", "mixture-averaged");

  double temperature = 300.0;              // K
  double pressure = 1.0 * Cantera::OneAtm; // atm
  double uin = 0.3;                        // m/sec
  auto flame_speed_baseline =
      flamespeed(sol_complete, temperature, pressure, uin, phi, refine_grid,
                 loglevel, Reactions);

  std::cout << "\nVelocidade da chama: " << flame_speed_baseline << " m/s\n";

  // std::sort(Reactions.begin(), Reactions.end(),
  //           [](const auto &a, const auto &b) { return a.first > b.first; });
  // for (const auto &item : Reactions) {
  //   std::cout << "Rate: " << item.first
  //             << "  Reaction: " << item.second->equation() << "\n";
  // }

  // how to get phase definition from existing Solution object
  // TODO: maybe pick direct from gri30.yaml instead? need test
  auto phaseNode = sol_complete->thermo()->input();

  std::vector<Cantera::AnyMap> species;
  for (size_t i = 0; i < sol_complete->thermo()->nSpecies(); i++) {
    auto sp = sol_complete->thermo()->species(i);
    Cantera::AnyMap sp_data = sp->parameters();

    std::string gambiarra_para_forçar_o_any_map_no_formato_certo =
        sp_data.toYamlString();

    Cantera::AnyMap sp_data_map = Cantera::AnyMap::fromYamlString(
        gambiarra_para_forçar_o_any_map_no_formato_certo);
    species.push_back(sp_data_map);
  }

  // std::vector<Cantera::AnyMap> reactionDefs;
  // for (size_t i = 0; i < sol_complete->kinetics()->nReactions(); i++) {
  //   auto rxn = sol_complete->kinetics()->reaction(i);
  //   Cantera::AnyMap rxn_data = rxn->input;
  //   reactionDefs.push_back(rxn_data);
  // }

  auto flame_speed_new = flame_speed_baseline;

  Cantera::AnyMap rootNode;

  while (std::abs(flame_speed_new - flame_speed_baseline) < tolerance_speed) {

    std::cout << "\nReaction with less weight:\n";
    auto min_reaction = std::min_element(
        Reactions.begin(), Reactions.end(), [](const auto &a, const auto &b) {
          return a.second.second < b.second.second;
        });

    // Remove the reaction from the list of reaction definitions
    if (min_reaction != Reactions.end()) {
      std::cout << "Rate: " << min_reaction->second.second
                << "  Reaction: " << min_reaction->second.first << "\n";

      if (Reactions.count(min_reaction->first) == 2) {
        auto equation = min_reaction->first;

        Reactions.erase(min_reaction);

        // remove só uma das duas
        auto min_duplicate = Reactions.find(equation);

        // std::cout << "Modifying duplicate reaction:\n";
        // std::cout << min_duplicate->second.first << "\n";

        Cantera::AnyMap rxn_data =
            Cantera::AnyMap::fromYamlString(min_duplicate->second.first);
        // rxn_data["duplicate"] = false;
        rxn_data.erase("duplicate");

        // std::cout << rxn_data.toYamlString() << "\n";

        min_duplicate->second.first = rxn_data.toYamlString();
      } else {
        Reactions.erase(min_reaction);
      }
    }

    std::vector<Cantera::AnyMap> reactionDefs;
    for (auto rxn : Reactions) {
      Cantera::AnyMap rxn_data =
          Cantera::AnyMap::fromYamlString(rxn.second.first);

      reactionDefs.push_back(rxn_data);
    }

    // auto new_sol =
    //     modified_solution(phaseNode, species, reactionDefs, "mixture-averaged");
    rootNode = mechanism_map(phaseNode, species, reactionDefs);

    // std::ofstream out(
    //     "/home/Shinmen/Workspace Cloud/flame-speed/modified_mechanism.yaml");
    // out << rootNode.toYamlString();

    std::string santa_gambiarra =
        rootNode.toYamlString(); // para forçar o formato certo
    // const auto originalName = phaseNode.at("name").asString();

    // Cantera::AnyMap modifiedPhase = phaseNode;
    // const auto newName = originalName;
    // modifiedPhase["name"] = newName;
    // write to file
    std::ofstream out(
        "/home/Shinmen/Workspace Cloud/flame-speed/modified_mechanism.yaml");
    out << rootNode.toYamlString();

    const Cantera::AnyMap &phaseNode_new =
        rootNode.at("phases").getMapWhere("name", "gri30");

    auto sol_new =
        Cantera::newSolution(phaseNode_new, rootNode, "mixture-averaged");

    flame_speed_new = flamespeed(sol_new, temperature, pressure, uin, phi,
                                 refine_grid, loglevel, Reactions);

    std::cout << "\nVelocidade da chama (novo mecanismo): " << flame_speed_new
              << " m/s\n";
    std::cout << "Reactions remaining: " << sol_new->kinetics()->nReactions()
              << "\n";
  }

  // write to file
  std::ofstream out(
      "/home/Shinmen/Workspace Cloud/flame-speed/modified_mechanism.yaml");
  out << rootNode.toYamlString();

  return 0;
}
