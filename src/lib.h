#include "cantera/base/AnyMap.h"
#include "cantera/base/Solution.h"
#include "cantera/base/logger.h"
#include "cantera/base/stringUtils.h"

#include <memory>
#include <sstream>
#include <string>
#include <vector>

Cantera::AnyMap mechanism_map(const Cantera::AnyMap &phases,
                              const std::vector<Cantera::AnyMap> &species,
                              const std::vector<Cantera::AnyMap> &reactions) {
  Cantera::AnyMap rootNode;

  // Cantera::AnyMap modifiedPhase = phases;
  // const auto originalName = phases.at("name").asString();

  // const auto newName = originalName ;
  // modifiedPhase["name"] = newName;

  Cantera::AnyMap units;
  units["length"] = "cm";
  units["time"] = "s";
  units["quantity"] = "mol";
  units["activation-energy"] = "cal/mol";
  rootNode["units"] = units;

  rootNode["phases"] = std::vector<Cantera::AnyMap>{phases};
  rootNode["species"] = species;
  rootNode["reactions"] = reactions;

  return rootNode;
}

// std::shared_ptr<Cantera::Solution>
// modified_solution(const Cantera::AnyMap &phases,
//                   const std::vector<Cantera::AnyMap> &species,
//                   const std::vector<Cantera::AnyMap> &reactions,
//                   const std::string &transport = "default",
//                   const std::vector<std::string> &adjacent = {}) {

//   Cantera::AnyMap rootNode;

//   // Create a modified phase with a new name
//   Cantera::AnyMap modifiedPhase = phases;
//   const auto originalName = phases.at("name").asString();

//   const auto newName = originalName;
//   modifiedPhase["name"] = newName;


//   rootNode["phases"] = std::vector<Cantera::AnyMap>{modifiedPhase};
//   rootNode["species"] = species;
//   rootNode["reactions"] = reactions;

//   const Cantera::AnyMap &phaseNode =
//       rootNode.at("phases").getMapWhere("name", newName);

//   std::vector<std::shared_ptr<Cantera::Solution>> adjPhases;

//   // Create explicitly-specified adjacent bulk phases
//   for (const auto &adjName : adjacent) {
//     const auto &adjNode = rootNode.at("phases").getMapWhere("name", adjName);
//     adjPhases.push_back(Cantera::newSolution(adjNode, rootNode));
//   }

//   // Construct a new Solution using in-memory AnyMap definitions
//   return Cantera::newSolution(phaseNode, rootNode, transport, adjPhases);
// }

double flamespeed(std::shared_ptr<Cantera::Solution> sol, double temperature,
                  double pressure, double uin, double phi, bool refine_grid,
                  int loglevel,
                  std::multimap<std::string, std::pair<std::string, double>> &reactions_weighted) {

  reactions_weighted.clear();
  try {

    auto gas = sol->thermo();
    auto kinetics = sol->kinetics();

    // Inicializar Rmax com o tamanho correto
    std::vector<double> Rmax(kinetics->nReactions(), 0.0);

    size_t nsp = gas->nSpecies();
    std::vector<double> x(nsp, 0.0);

    gas->setEquivalenceRatio(phi, "CH4", "O2:0.21,N2:0.79");
    gas->setState_TP(temperature, pressure);
    gas->getMoleFractions(x.data());

    double rho_in = gas->density();

    std::vector<double> yin(nsp);
    gas->getMassFractions(&yin[0]);

    gas->equilibrate("HP");
    std::vector<double> yout(nsp);
    gas->getMassFractions(&yout[0]);
    double rho_out = gas->density();
    double Tad = gas->temperature();
    // print("phi = {}, Tad = {}\n", phi, Tad);

    //=============  build each domain ========================

    //-------- step 1: create the flow -------------

    auto flow = Cantera::newDomain<Cantera::Flow1D>("gas-flow", sol, "flow");
    flow->setFreeFlow();

    // create an initial grid
    int nz = 6;
    double lz = 0.1;
    std::vector<double> z(nz);
    double dz = lz / ((double)(nz - 1));
    for (int iz = 0; iz < nz; iz++) {
      z[iz] = ((double)iz) * dz;
    }

    flow->setupGrid(nz, &z[0]);

    //------- step 2: create the inlet  -----------------------

    auto inlet = Cantera::newDomain<Cantera::Inlet1D>("inlet", sol);

    inlet->setMoleFractions(x.data());
    double mdot = uin * rho_in;
    inlet->setMdot(mdot);
    inlet->setTemperature(temperature);

    //------- step 3: create the outlet  ---------------------

    auto outlet = Cantera::newDomain<Cantera::Outlet1D>("outlet", sol);
    //=================== create the container and insert the domains =====

    std::vector<std::shared_ptr<Cantera::Domain1D>> domains{inlet, flow,
                                                            outlet};
    Cantera::Sim1D flame(domains);

    //----------- Supply initial guess----------------------

    std::vector<double> locs{0.0, 0.3, 0.7, 1.0};
    std::vector<double> value;

    double uout = inlet->mdot() / rho_out;
    value = {uin, uin, uout, uout};
    flame.setInitialGuess("velocity", locs, value);
    value = {temperature, temperature, Tad, Tad};
    flame.setInitialGuess("T", locs, value);

    for (size_t i = 0; i < nsp; i++) {
      value = {yin[i], yin[i], yout[i], yout[i]};
      flame.setInitialGuess(gas->speciesName(i), locs, value);
    }

    inlet->setMoleFractions(x.data());
    inlet->setMdot(mdot);
    inlet->setTemperature(temperature);

    flame.show();

    int flowdomain = 1;
    double ratio = 10.0;
    double slope = 0.08;
    double curve = 0.1;

    flame.setRefineCriteria(flowdomain, ratio, slope, curve);

    // Save initial guess to container file

    // Solution is saved in HDF5 or YAML file format
    // std::string fileName;
    // if (Cantera::usesHDF5()) {
    //   // Cantera is compiled with native HDF5 support
    //   fileName = "flamespeed.h5";
    // } else {
    //   fileName = "flamespeed.yaml";
    // }
    // flame.save(fileName, "initial-guess", "Initial guess", true);

    // Solve freely propagating flame

    // Linearly interpolate to find location where this temperature would exist. The temperature at this location will then be fixed for remainder of calculation.
    flame.setFixedTemperature(0.5 * (temperature + Tad));
    flow->solveEnergyEqn();

    flame.solve(loglevel, refine_grid);
    // double flameSpeed_mix =
    //     flame.value(flowdomain, flow->componentIndex("velocity"), 0);
    // // print("Flame speed with mixture-averaged transport: {} m/s\n",
    // // flameSpeed_mix);
    // flame.save(fileName, "mix", "Solution with mixture-averaged transport",
    //            true);

    // now switch to multicomponent transport
    flow->setTransportModel("multicomponent");
    flame.solve(loglevel, refine_grid);
    double flameSpeed_multi =
        flame.value(flowdomain, flow->componentIndex("velocity"), 0);
    // print("Flame speed with multicomponent transport: {} m/s\n",
    //       flameSpeed_multi);
    // flame.save(fileName, "multi", "Solution with multicomponent transport",
    //            true);

    // now enable Soret diffusion
    flow->enableSoret(true);
    flame.solve(loglevel, refine_grid);
    double flameSpeed_full =
        flame.value(flowdomain, flow->componentIndex("velocity"), 0);
    // print("Flame speed with multicomponent transport + Soret: {} m/s\n",
    //       flameSpeed_full);
    // flame.save(fileName, "soret",
    //            "Solution with mixture-averaged transport and Soret", true);

    std::vector<double> zvec, Tvec, COvec, CO2vec, Uvec;

    // print("\n{:9s}\t{:8s}\t{:5s}\t{:7s}\n", "z (m)", "T (K)", "U (m/s)",
    //       "Y(CO)");
    for (size_t n = 0; n < flow->nPoints(); n++) {
      Tvec.push_back(flame.value(flowdomain, flow->componentIndex("T"), n));
      COvec.push_back(flame.value(flowdomain, flow->componentIndex("CO"), n));
      CO2vec.push_back(flame.value(flowdomain, flow->componentIndex("CO2"), n));
      Uvec.push_back(
          flame.value(flowdomain, flow->componentIndex("velocity"), n));
      zvec.push_back(flow->z(n));
      // print("{:9.6f}\t{:8.3f}\t{:5.3f}\t{:7.5f}\n", flow->z(n), Tvec[n],
      //       Uvec[n], COvec[n]);
    }

    // print("\nAdiabatic flame temperature from equilibrium is: {}\n", Tad);
    // print("Flame speed for phi={} is {} m/s.\n", phi, Uvec[0]);

    // std::ofstream outfile("flamespeed.csv", std::ios::trunc);
    // outfile << "  Grid,   Temperature,   Uvec,   CO,    CO2\n";

    for (size_t n = 0; n < flow->nPoints(); n++) {
      // print(outfile, " {:11.3e}, {:11.3e}, {:11.3e}, {:11.3e}, {:11.3e}\n",
      //       flow->z(n), Tvec[n], Uvec[n], COvec[n], CO2vec[n]);

      // Definir estado termodinâmico no ponto n
      std::vector<double> Y_point(nsp);
      for (size_t k = 0; k < nsp; k++) {
        Y_point[k] = flame.value(flowdomain,
                                 flow->componentIndex(gas->speciesName(k)), n);
      }
      gas->setState_TPY(Tvec[n], pressure, &Y_point[0]);

      // Usar sol->kinetics() para acessar as taxas de reação
      auto kinetics = sol->kinetics();
      std::vector<double> rnet(kinetics->nReactions());
      kinetics->getNetRatesOfProgress(&rnet[0]);

      // Normalizar as taxas
      double max_rate = 0.0;
      for (size_t i = 0; i < rnet.size(); i++) {
        rnet[i] = std::abs(rnet[i]);
        max_rate = std::max(max_rate, rnet[i]);
      }

      if (max_rate > 0.0) {
        for (size_t i = 0; i < rnet.size(); i++) {
          rnet[i] /= max_rate;
          Rmax[i] = std::max(Rmax[i], rnet[i]);
        }
      }
    }
    for (size_t i = 0; i < Rmax.size(); i++) {

      auto reaction_equation = kinetics->reaction(i)->equation();

      auto reaction_pair = std::make_pair(kinetics->reaction(i)->input.toYamlString(), Rmax[i]);

      // reactions_weighted[reaction_equation] = reaction_pair;
      reactions_weighted.insert(std::make_pair(reaction_equation, reaction_pair));
      // }

      std::cout << "Reaction " << i << " (" << kinetics->reaction(i)->equation()
                << ")"
                << " max normalized rate = " << Rmax[i] << "\n";
    }

    return Uvec[0];
  } catch (Cantera::CanteraError &err) {
    std::cerr << err.what() << std::endl;
    std::cerr << "program terminating." << std::endl;
    return -1.0;
  }
  return 0.0;
}