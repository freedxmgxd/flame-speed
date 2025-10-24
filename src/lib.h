#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "cantera/base/AnyMap.h"
#include "cantera/base/Solution.h"
#include "cantera/base/logger.h"
#include "cantera/base/stringUtils.h"

struct thermo_state {
  // TODO: Melhorar o nome
  double flamespeed;
  double Tad;
  double Tmax;
  double zmax;

  operator double() const { return flamespeed; }

  thermo_state() : flamespeed(-1.0), Tad(-1.0), Tmax(-1.0), zmax(-1.0) {}
  thermo_state(double fs, double tad, double tmax, double zmax)
      : flamespeed(fs), Tad(tad), Tmax(tmax), zmax(zmax) {}
  
  ~thermo_state() = default;
};

Cantera::AnyMap mechanism_map(const Cantera::AnyMap& phases,
                              const std::vector<Cantera::AnyMap>& species,
                              const std::vector<Cantera::AnyMap>& reactions) {
  Cantera::AnyMap rootNode;

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

static std::multimap<std::string, std::pair<std::string, double>> empty_reactions;

thermo_state flamespeed(std::shared_ptr<Cantera::Solution> sol,
                        double temperature,
                        double pressure,
                        double uin,
                        double mixture_ratio,
                        const std::string& fuelComp,
                        const std::string& oxComp,
                        bool refine_grid,
                        int loglevel,
                        std::multimap<std::string, std::pair<std::string, double>>&
                            reactions_weighted = empty_reactions) {
  // TODO: criar situação para calcular a velocidade sem precisar retornar/modificar o
  // reactions_weighted, talzes usar std::optional e usar if para ver se tem valor ou não
  // TODO: modificar para função ao inves de ser flamespeed, calcular todos os outputs desejados,
  // Temperatura máxima, adiabática, atraso de tempo de ignição, etc
  // TODO: mudar o nome da função para algo mais adequado
  reactions_weighted.clear();

  thermo_state state;

  try {
    auto gas = sol->thermo();
    auto kinetics = sol->kinetics();

    // Inicializar Rmax com o tamanho correto
    std::vector<double> Rmax(kinetics->nReactions(), 0.0);

    size_t nsp = gas->nSpecies();
    std::vector<double> x(nsp, 0.0);

    gas->setMixtureFraction(mixture_ratio, fuelComp, oxComp);
    // gas->setEquivalenceRatio(1.0, "CH4", "O2:0.21,N2:0.79");
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
    state.Tad = Tad;

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

    std::vector<std::shared_ptr<Cantera::Domain1D>> domains{inlet, flow, outlet};
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

    // Linearly interpolate to find location where this temperature would exist. The temperature at
    // this location will then be fixed for remainder of calculation.
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
    // double flameSpeed_multi =
    //     flame.value(flowdomain, flow->componentIndex("velocity"), 0);
    // print("Flame speed with multicomponent transport: {} m/s\n",
    //       flameSpeed_multi);
    // flame.save(fileName, "multi", "Solution with multicomponent transport",
    //            true);

    // now enable Soret diffusion
    flow->enableSoret(true);
    flame.solve(loglevel, refine_grid);
    // double flameSpeed_full =
    //     flame.value(flowdomain, flow->componentIndex("velocity"), 0);
    // print("Flame speed with multicomponent transport + Soret: {} m/s\n",
    //       flameSpeed_full);
    // flame.save(fileName, "soret",
    //            "Solution with mixture-averaged transport and Soret", true);

    std::vector<double> zvec, Tvec, COvec, CO2vec, Uvec;

    double T_max = 0.0;
    double z_max = 0.0;
    // print("\n{:9s}\t{:8s}\t{:5s}\t{:7s}\n",
    // "z (m)", "T (K)", "U (m/s)", "Y(CO)");
    for (size_t n = 0; n < flow->nPoints(); n++) {
      Tvec.push_back(flame.value(flowdomain, flow->componentIndex("T"), n));
      COvec.push_back(flame.value(flowdomain, flow->componentIndex("CO"), n));
      CO2vec.push_back(flame.value(flowdomain, flow->componentIndex("CO2"), n));
      Uvec.push_back(flame.value(flowdomain, flow->componentIndex("velocity"), n));
      zvec.push_back(flow->z(n));

      if (Tvec[n] > T_max) {
        T_max = Tvec[n];  // TODO: talvez tirar isso daqui e buscar forma melhor de fazer isso.
        z_max = zvec[n];  // TODO: calcular a frente de chama e pegar o máximo dela
        // TODO: talvez calcular o atraso de tempo de ignição também mas não sei como fazer isso
        // ainda
      }
      // print("{:9.6f}\t{:8.3f}\t{:5.3f}\t{:7.5f}\n",
      // flow->z(n), Tvec[n], Uvec[n], COvec[n]);
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
        Y_point[k] = flame.value(flowdomain, flow->componentIndex(gas->speciesName(k)), n);
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

      // std::cout << "Reaction " << i << " (" << kinetics->reaction(i)->equation()
      //           << ")"
      //           << " max normalized rate = " << Rmax[i] << "\n";
    }

    state.flamespeed = Uvec[0];
    state.Tmax = T_max;
    state.zmax = z_max;

    return state;
  } catch (Cantera::CanteraError& err) {
    std::cerr << err.what() << std::endl;
    return state;
  }
  return state;
}

template <typename Function, typename... Args>
Cantera::AnyMap mechanism_reduction(std::shared_ptr<Cantera::Solution> sol_complete,
                                    double tolerance_value,
                                    Function function_reference,
                                    Args... args) {
  std::multimap<std::string, std::pair<std::string, double>> Reactions;

  auto value_baseline =
      function_reference(sol_complete,
                         args...,
                         Reactions);  // TODO: Esse Reactions aqui pode quebrar implementações
                                      // futuras, por isso precisa de uma solução melhor

  std::cout << "\nVelocidade da chama: " << value_baseline << " m/s\n";

  // how to get phase definition from existing Solution object
  // TODO: maybe pick direct from gri30.yaml instead? need test
  auto phaseNode = sol_complete->thermo()->input();

  std::vector<Cantera::AnyMap> species;
  for (size_t i = 0; i < sol_complete->thermo()->nSpecies(); i++) {
    auto sp = sol_complete->thermo()->species(i);
    Cantera::AnyMap sp_data = sp->parameters();

    std::string gambiarra_para_forçar_o_any_map_no_formato_certo = sp_data.toYamlString();

    Cantera::AnyMap sp_data_map =
        Cantera::AnyMap::fromYamlString(gambiarra_para_forçar_o_any_map_no_formato_certo);
    species.push_back(sp_data_map);
  }

  auto value_new = value_baseline;

  Cantera::AnyMap rootNode;

  while ((std::abs(value_new - value_baseline) < tolerance_value) and (Reactions.size() > 0)) {
    std::cout << "\nReaction with less weight:\n";
    auto min_reaction =
        std::min_element(Reactions.begin(), Reactions.end(), [](const auto& a, const auto& b) {
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

        Cantera::AnyMap rxn_data = Cantera::AnyMap::fromYamlString(min_duplicate->second.first);
        rxn_data.erase("duplicate");

        min_duplicate->second.first = rxn_data.toYamlString();
      } else {
        Reactions.erase(min_reaction);
      }
    }  // TODO: Another criteria is to set a minimum weight value,

    std::vector<Cantera::AnyMap> reactionDefs;
    for (auto rxn : Reactions) {
      Cantera::AnyMap rxn_data = Cantera::AnyMap::fromYamlString(rxn.second.first);

      reactionDefs.push_back(rxn_data);
    }

    rootNode = mechanism_map(phaseNode, species, reactionDefs);

    std::string santa_gambiarra = rootNode.toYamlString();  // para forçar o formato certo

    // std::ofstream out(
    //     "/home/Shinmen/Workspace Cloud/flame-speed/modified_mechanism.yaml");
    // out << rootNode.toYamlString();

    const Cantera::AnyMap& phaseNode_new = rootNode.at("phases").getMapWhere("name", "gri30");

    auto sol_new = Cantera::newSolution(phaseNode_new, rootNode, "mixture-averaged");

    // sol_new, temperature, pressure, uin, phi,                                   refine_grid,
    // loglevel, Reactions
    value_new = function_reference(sol_new, args..., Reactions);

    std::cout << "\nVelocidade da chama (novo mecanismo): " << value_new << " m/s\n";
    std::cout << "Reactions remaining: " << Reactions.size() << "\n";
  }
  return rootNode;
}