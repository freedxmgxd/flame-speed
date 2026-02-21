#include <cmath>
#include <cstdlib>  // Necessário para std::getenv
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

// Função auxiliar para substituir variáveis em um arquivo
void replaceInFile(const std::string& filePath,
                   const std::map<std::string, std::string>& replacements) {
  std::ifstream inputFile(filePath);
  if (!inputFile.is_open()) {
    std::cerr << "Erro: Não foi possível abrir o arquivo: " << filePath << std::endl;
    return;
  }

  std::string content((std::istreambuf_iterator<char>(inputFile)),
                      std::istreambuf_iterator<char>());
  inputFile.close();

  for (const auto& [placeholder, value] : replacements) {
    size_t pos = 0;
    while ((pos = content.find(placeholder, pos)) != std::string::npos) {
      content.replace(pos, placeholder.length(), value);
      pos += value.length();
    }
  }

  std::ofstream outputFile(filePath);
  if (!outputFile.is_open()) {
    std::cerr << "Erro: Não foi possível escrever no arquivo: " << filePath << std::endl;
    return;
  }

  outputFile << content;
  outputFile.close();
}

int main(int argc, char** argv) {
  auto height_channel      = 2.0;    // mm (Altura do canal)
  auto length_inlet        = 37.0;   // mm (Comprimento entrada)
  auto length_expansion    = 110.0;  // mm (Comprimento expansão)
  auto width_inlet         = 25.0;   // mm (Largura entrada)
  auto width_expansion     = 63.2;   // mm (Largura expansão/saída)
  auto height_channel_half = height_channel * 0.5;

  auto channel_thickness   = 2.0;  // mm (Espessura do canal

  auto length_extern       = 50.0;  // mm (Comprimento sobressalente)
  auto width_extern        = 6.0;   // mm (Largura sobressalente adicional)
  auto height_extern       = 6.0;   // mm (Altura sobressalente adicional)

  // Defina o tamanho máximo e mínimo de célula em cada direção (mm)
  auto max_cell_size_x = 0.5; // mm
  auto max_cell_size_y = 1.0; // mm
  auto max_cell_size_z = 1.0; // mm

  // Mínimos ajustados para evitar células muito finas
  auto min_cell_size_x  = 0.5;  // mm
  auto min_cell_size_y  = 0.5;  // mm (camada limite, mais seguro)
  auto min_cell_size_z  = 0.5;  // mm

  auto velocity_inlet   = 0.5;  // m/s (Velocidade de entrada)'

  auto length_channel   = ((length_inlet + length_expansion) * 0.001);

  auto velocity_average = velocity_inlet / 2.0;
  auto time_cold_flow   = length_channel / velocity_average;  // Tempo total de simulação (s)

  time_cold_flow        = 0.01;
  // Calcular posicao da ignicao (final da expansao)
  double position_ignition_flame_val = length_channel;  // + (length_extern * 0.001) / 2.0;

  auto time_simulation               = time_cold_flow + 2.0;  // Tempo total de simulação (s)

  // Função para calcular o número de células e razão de expansão dado L, target_max e target_min de
  // forma analítica
  auto calculateGradingAndCells =
      [](double L, double dx_max, double dx_min) -> std::pair<int, double> {
    if (std::abs(dx_max - dx_min) < 1e-6 || L <= std::max(dx_max, dx_min)) {
      int N = std::max(4, static_cast<int>(std::round(L / std::max(dx_max, dx_min))));
      return {N, 1.0};
    }

    // Cálculo analítico exato usando a progressão geométrica:
    // Soma: L = dx_min * (k^N - 1) / (k - 1)
    // Último termo: dx_max = dx_min * k^(N-1)
    // Resolvendo o sistema obtemos k exatamente:
    double k            = (L - dx_min) / (L - dx_max);
    double target_ratio = dx_max / dx_min;

    // Calcula N e arredonda para o inteiro mais próximo
    int N = std::max(4, static_cast<int>(std::round(1.0 + std::log(target_ratio) / std::log(k))));

    return {N, target_ratio};
  };

  // Uniform in X uses max_cell_size
  auto [n_cells_x_inlet, _rx1] =
      calculateGradingAndCells(length_inlet, max_cell_size_x, max_cell_size_x);
  auto [n_cells_x_expansion, _rx2] =
      calculateGradingAndCells(length_expansion, max_cell_size_x, max_cell_size_x);
  auto [n_cells_x_extern, _rx3] =
      calculateGradingAndCells(length_extern, max_cell_size_x, max_cell_size_x);

  // O domínio já representa apenas metade da largura (0 a W/2) e altura (-h/2 a 0) do canal
  // Portanto, cada bloco vai da simetria (máximo) até a parede (mínimo)
  auto [n_cells_y, grading_ratio_y] =
      calculateGradingAndCells(width_expansion * 0.5, max_cell_size_y, min_cell_size_y);

  auto [n_cells_z, grading_ratio_z] =
      calculateGradingAndCells(height_channel_half, max_cell_size_z, min_cell_size_z);

  auto [n_cells_z_extern, _rze] =
      calculateGradingAndCells(height_extern, max_cell_size_z, max_cell_size_z);

  auto [n_cells_y_thickness, grading_ratio_y_thickness] =
      calculateGradingAndCells(channel_thickness, max_cell_size_y, min_cell_size_y);
  auto [n_cells_y_extern, _rye] =
      calculateGradingAndCells(width_extern, max_cell_size_y, max_cell_size_y);

  std::string vertices[40] = {
      "(0 0 " + std::to_string(-height_channel_half) + ")",  // Vertex 0
      "(0 0 0)",                                             // Vertex 1
      "(0 " + std::to_string(width_inlet / 2) + " " + std::to_string(-height_channel_half) +
          ")",                                               // Vertex 2
      "(0 " + std::to_string(width_inlet / 2) + " 0" + ")",  // Vertex 3
      "(" + std::to_string(length_inlet) + " 0 " + std::to_string(-height_channel_half) +
          ")",                                            // Vertex 4
      "(" + std::to_string(length_inlet) + " 0 0" + ")",  // Vertex 5
      "(" + std::to_string(length_inlet) + " " + std::to_string(width_inlet / 2) + " " +
          std::to_string(-height_channel_half) + ")",  // Vertex 6
      "(" + std::to_string(length_inlet) + " " + std::to_string(width_inlet / 2) + " " + "0" +
          ")",  // Vertex 7
      "(" + std::to_string(length_inlet + length_expansion) + " 0 " +
          std::to_string(-height_channel_half) + ")",                             // Vertex 8
      "(" + std::to_string(length_inlet + length_expansion) + " 0 " + "0" + ")",  // Vertex 9
      "(" + std::to_string(length_inlet + length_expansion) + " " +
          std::to_string(width_expansion / 2) + " " + std::to_string(-height_channel_half) +
          ")",  // Vertex 10
      "(" + std::to_string(length_inlet + length_expansion) + " " +
          std::to_string(width_expansion / 2) + " 0" + ")",  // Vertex 11
      "(" + std::to_string(length_inlet + length_expansion + length_extern) + " 0 " +
          std::to_string(-height_channel_half) + ")",  // Vertex 12
      "(" + std::to_string(length_inlet + length_expansion + length_extern) + " 0 " + "0" +
          ")",  // Vertex 13
      "(" + std::to_string(length_inlet + length_expansion + length_extern) + " " +
          std::to_string(width_expansion / 2) + " " + std::to_string(-height_channel_half) +
          ")",  // Vertex 14
      "(" + std::to_string(length_inlet + length_expansion + length_extern) + " " +
          std::to_string(width_expansion / 2) + " 0" + ")",  // Vertex 15
      "(" + std::to_string(length_inlet + length_expansion) + " " +
          std::to_string(width_expansion * 0.5 + channel_thickness + width_extern) + " " +
          std::to_string(-height_channel_half) + ")",  // Vertex 16
      "(" + std::to_string(length_inlet + length_expansion) + " " +
          std::to_string(width_expansion * 0.5 + channel_thickness + width_extern) + " 0" +
          ")",  // Vertex 17
      "(" + std::to_string(length_inlet + length_expansion + length_extern) + " " +
          std::to_string(width_expansion * 0.5 + channel_thickness + width_extern) + " " +
          std::to_string(-height_channel_half) + ")",  // Vertex 18
      "(" + std::to_string(length_inlet + length_expansion + length_extern) + " " +
          std::to_string(width_expansion * 0.5 + channel_thickness + width_extern) + " 0" +
          ")",  // Vertex 19
      "(" + std::to_string(length_inlet + length_expansion) + " 0 " +
          std::to_string(height_channel_half) + ")",  // Vertex 20
      "(" + std::to_string(length_inlet + length_expansion) + " " +
          std::to_string(width_expansion / 2) + " " + std::to_string(height_channel_half) +
          ")",  // Vertex 21
      "(" + std::to_string(length_inlet + length_expansion + length_extern) + " 0 " +
          std::to_string(height_channel_half) + ")",  // Vertex 22
      "(" + std::to_string(length_inlet + length_expansion + length_extern) + " " +
          std::to_string(width_expansion / 2) + " " + std::to_string(height_channel_half) +
          ")",  // Vertex 23
      "(" + std::to_string(length_inlet + length_expansion) + " " +
          std::to_string(width_expansion * 0.5 + channel_thickness + width_extern) + " " +
          std::to_string(height_channel_half) + ")",  // Vertex 24
      "(" + std::to_string(length_inlet + length_expansion + length_extern) + " " +
          std::to_string(width_expansion * 0.5 + channel_thickness + width_extern) + " " +
          std::to_string(height_channel_half) + ")",  // Vertex 25
      // Vértices para a espessura externa do canal na região do extern
      // Bloco 4: espessura em Y (parte inferior z)
      "(" + std::to_string(length_inlet + length_expansion) + " " +
          std::to_string(width_expansion / 2 + channel_thickness) + " " +
          std::to_string(-height_channel_half) + ")",  // Vertex 26
      "(" + std::to_string(length_inlet + length_expansion) + " " +
          std::to_string(width_expansion / 2 + channel_thickness) + " 0" + ")",  // Vertex 27
      "(" + std::to_string(length_inlet + length_expansion + length_extern) + " " +
          std::to_string(width_expansion / 2 + channel_thickness) + " " +
          std::to_string(-height_channel_half) + ")",  // Vertex 28
      "(" + std::to_string(length_inlet + length_expansion + length_extern) + " " +
          std::to_string(width_expansion / 2 + channel_thickness) + " 0" + ")",  // Vertex 29
      // Bloco 5: espessura em Z (topo, z positivo)
      "(" + std::to_string(length_inlet + length_expansion) + " " +
          std::to_string(width_expansion / 2 + channel_thickness) + " " +
          std::to_string(height_channel_half) + ")",  // Vertex 30
      "(" + std::to_string(length_inlet + length_expansion + length_extern) + " " +
          std::to_string(width_expansion / 2 + channel_thickness) + " " +
          std::to_string(height_channel_half) + ")",  // Vertex 31
      // Bloco 7: externo em Z (topo externo)
      "(" + std::to_string(length_inlet + length_expansion) + " 0 " +
          std::to_string(height_channel_half + height_extern) + ")",  // Vertex 32
      "(" + std::to_string(length_inlet + length_expansion + length_extern) + " 0 " +
          std::to_string(height_channel_half + height_extern) + ")",  // Vertex 33
      "(" + std::to_string(length_inlet + length_expansion) + " " +
          std::to_string(width_expansion * 0.5 + channel_thickness + width_extern) + " " +
          std::to_string(height_channel_half + height_extern) + ")",  // Vertex 34
      "(" + std::to_string(length_inlet + length_expansion + length_extern) + " " +
          std::to_string(width_expansion * 0.5 + channel_thickness + width_extern) + " " +
          std::to_string(height_channel_half + height_extern) + ")",  // Vertex 35
      "(" + std::to_string(length_inlet + length_expansion) + " " +
          std::to_string(width_expansion / 2) + " " +
          std::to_string(height_channel_half + height_extern) + ")",  // Vertex 36
      "(" + std::to_string(length_inlet + length_expansion + length_extern) + " " +
          std::to_string(width_expansion / 2) + " " +
          std::to_string(height_channel_half + height_extern) + ")",  // Vertex 37
      "(" + std::to_string(length_inlet + length_expansion) + " " +
          std::to_string(width_expansion / 2 + channel_thickness) + " " +
          std::to_string(height_channel_half + height_extern) + ")",  // Vertex 38
      "(" + std::to_string(length_inlet + length_expansion + length_extern) + " " +
          std::to_string(width_expansion / 2 + channel_thickness) + " " +
          std::to_string(height_channel_half + height_extern) + ")",  // Vertex 39
  };

  std::string vertices_string;
  for (const auto& vertex : vertices) {
    vertices_string += "    " + vertex + "\n";
  }

  auto n_cells_y_external = n_cells_y + n_cells_y_thickness + n_cells_y_extern;

  // Reorganização dos blocos com nomenclatura clara
  std::string mesh_sizes_canal_inlet = std::to_string(n_cells_x_inlet) + " " +
                                       std::to_string(n_cells_y) + " " + std::to_string(n_cells_z);
  std::string mesh_sizes_canal_divergente = std::to_string(n_cells_x_expansion) + " " +
                                            std::to_string(n_cells_y) + " " +
                                            std::to_string(n_cells_z);
  std::string mesh_sizes_outlet_center = std::to_string(n_cells_x_extern) + " " +
                                         std::to_string(n_cells_y) + " " +
                                         std::to_string(n_cells_z);
  std::string mesh_sizes_outlet_espessura_y = std::to_string(n_cells_x_extern) + " " +
                                              std::to_string(n_cells_y_thickness) + " " +
                                              std::to_string(n_cells_z);
  std::string mesh_sizes_outlet_espessura_z = std::to_string(n_cells_x_extern) + " " +
                                              std::to_string(n_cells_y_thickness) + " " +
                                              std::to_string(n_cells_z);
  std::string mesh_sizes_outlet_externo_y = std::to_string(n_cells_x_extern) + " " +
                                            std::to_string(n_cells_y_extern) + " " +
                                            std::to_string(n_cells_z);
  std::string mesh_sizes_outlet_externo_y_full = std::to_string(n_cells_x_extern) + " " +
                                                 std::to_string(n_cells_y_extern) + " " +
                                                 std::to_string(2 * n_cells_z);
  std::string mesh_sizes_outlet_externo_z_lower = std::to_string(n_cells_x_extern) + " " +
                                                  std::to_string(n_cells_y) + " " +
                                                  std::to_string(n_cells_z);
  std::string mesh_sizes_outlet_externo_z_upper_center = std::to_string(n_cells_x_extern) + " " +
                                                         std::to_string(n_cells_y) + " " +
                                                         std::to_string(n_cells_z_extern);
  std::string mesh_sizes_outlet_externo_z_upper_thickness = std::to_string(n_cells_x_extern) + " " +
                                                            std::to_string(n_cells_y_thickness) +
                                                            " " + std::to_string(n_cells_z_extern);
  std::string mesh_sizes_outlet_externo_z_upper_extern = std::to_string(n_cells_x_extern) + " " +
                                                         std::to_string(n_cells_y_extern) + " " +
                                                         std::to_string(n_cells_z_extern);

  // Formatação BlockMesh
  const double inv_grading_ratio_y = 1.0 / grading_ratio_y;
  const double inv_grading_ratio_z = 1.0 / grading_ratio_z;

  // A malha vai do plano de simetria (inicio do bloco) até a parede (fim do bloco), começando
  // grande e terminando pequena. Expansion ratio = last / first = min / max = inv_grading
  std::string simple_grading_y           = std::to_string(inv_grading_ratio_y);
  std::string simple_grading_y_thickness = std::to_string(grading_ratio_y_thickness);

  std::string simple_grading_z_lower     = std::to_string(inv_grading_ratio_z);
  std::string simple_grading_z_upper =
      std::to_string(grading_ratio_z);  // z=0 até z=h/2 cresce do pequeno (parede) pro grande

  // Formatados para blockMeshDict
  std::string grading_canal_inlet = "(1 " + simple_grading_y + " " + simple_grading_z_lower + ")";
  std::string grading_canal_divergente =
      "(1 " + simple_grading_y + " " + simple_grading_z_lower + ")";

  std::string grading_outlet_center = grading_canal_divergente;
  std::string grading_outlet_center_upper =
      "(1 " + simple_grading_y + " " + simple_grading_z_upper + ")";

  // Nas paredes e regiões externas, usa uniforme ou propaga o grading dos vizinhos
  std::string uniform_grading = "1";

  // Bloco 4 (espessura Y): Compartilha Y com espessura (min -> max), e Z inferior
  std::string grading_outlet_espessura_y =
      "(1 " + simple_grading_y_thickness + " " + simple_grading_z_lower + ")";

  // Bloco 5b (espessura Z): Compartilha Y com espessura, e Z superior
  std::string grading_outlet_espessura_z =
      "(1 " + simple_grading_y_thickness + " " + simple_grading_z_upper + ")";

  // Bloco 7a (Z externo superior central): Compartilha Y com canal, Z externo
  std::string grading_outlet_externo_z_upper_center =
      "(1 " + simple_grading_y + " " + uniform_grading + ")";

  // Blocos 6a/6b puros externos (sem compartilhamento em Y)
  std::string grading_outlet_externo_y =
      "(1 " + uniform_grading + " " + simple_grading_z_lower + ")";
  std::string grading_outlet_externo_y_upper =
      "(1 " + uniform_grading + " " + simple_grading_z_upper + ")";

  std::string grading_outlet_externo_z_upper_thickness =
      "(1 " + simple_grading_y_thickness + " " + uniform_grading + ")";
  std::string grading_outlet_externo_z_upper_extern = "(1 1 1)";

  // TODO: Ajustar as malhas
  // TODO: Ajuster os arquivos de condição de contorno

  const char* foamRunEnv = std::getenv("FOAM_RUN");
  if (foamRunEnv == nullptr) {
    std::cerr << "Erro: A variável de ambiente FOAM_RUN não está definida." << std::endl;
    return 1;  // Retorna um código de erro
  }
  //  Copy canal_base to canal (resolve absolute path to template)
  std::string dirPath = std::string(foamRunEnv) + "/canal";

  // Template simples: espera encontrar ./canal_base no diretório atual
  std::filesystem::path templateDir;
  if (const char* env_p = std::getenv("BUILD_WORKSPACE_DIRECTORY")) {
    templateDir = std::filesystem::path(env_p) / "canal_base";
  } else {
    templateDir = std::filesystem::path("/home/Shinmen/Workspace Cloud/flame-speed/canal_base");
  }

  if (!std::filesystem::exists(templateDir)) {
    std::cerr << "Erro: Diretório template 'canal_base' não encontrado em: " << templateDir
              << std::endl;
    return 1;
  }

  // Ensure destination exists before copying
  if (std::filesystem::exists(dirPath)) {
    for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
      std::filesystem::remove_all(entry.path());
    }
  }
  std::filesystem::create_directories(dirPath);
  std::filesystem::copy(
      templateDir,
      dirPath,
      std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);

  // Tenta só marcar buildrun.sh como executável; ignora erros
  auto buildrun = std::filesystem::path(dirPath) / "buildrun.sh";
  if (std::filesystem::exists(buildrun)) {
    std::filesystem::permissions(buildrun,
                                 std::filesystem::perms::owner_exec |
                                     std::filesystem::perms::group_exec |
                                     std::filesystem::perms::others_exec,
                                 std::filesystem::perm_options::add);
  }

  std::string blockMeshfilePath                            = dirPath + "/system/blockMeshDict";
  std::map<std::string, std::string> blockMeshReplacements = {
      {"${vertex_list}", vertices_string},
      {"${mesh_sizes_canal_inlet}", mesh_sizes_canal_inlet},
      {"${mesh_sizes_canal_divergente}", mesh_sizes_canal_divergente},
      {"${mesh_sizes_outlet_center}", mesh_sizes_outlet_center},
      {"${mesh_sizes_outlet_espessura_y}", mesh_sizes_outlet_espessura_y},
      {"${mesh_sizes_outlet_espessura_z}", mesh_sizes_outlet_espessura_z},
      {"${mesh_sizes_outlet_externo_y}", mesh_sizes_outlet_externo_y},
      {"${mesh_sizes_outlet_externo_y_full}", mesh_sizes_outlet_externo_y_full},
      {"${mesh_sizes_outlet_externo_z_lower}", mesh_sizes_outlet_externo_z_lower},
      {"${mesh_sizes_outlet_externo_z_upper_center}", mesh_sizes_outlet_externo_z_upper_center},
      {"${mesh_sizes_outlet_externo_z_upper_thickness}",
       mesh_sizes_outlet_externo_z_upper_thickness},
      {"${mesh_sizes_outlet_externo_z_upper_extern}", mesh_sizes_outlet_externo_z_upper_extern},
      {"${grading_canal_inlet}", grading_canal_inlet},
      {"${grading_canal_divergente}", grading_canal_divergente},
      {"${grading_outlet_center}", grading_outlet_center},
      {"${grading_outlet_center_upper}", grading_outlet_center_upper},
      {"${grading_outlet_espessura_y}", grading_outlet_espessura_y},
      {"${grading_outlet_espessura_z}", grading_outlet_espessura_z},
      {"${grading_outlet_externo_z_upper_center}", grading_outlet_externo_z_upper_center},
      {"${grading_outlet_externo_y}", grading_outlet_externo_y},
      {"${grading_outlet_externo_y_upper}", grading_outlet_externo_y_upper},
      {"${grading_outlet_externo_z_upper_thickness}", grading_outlet_externo_z_upper_thickness},
      {"${grading_outlet_externo_z_upper_extern}", grading_outlet_externo_z_upper_extern},
  };
  replaceInFile(blockMeshfilePath, blockMeshReplacements);

  std::map<std::string, double> initial_concentrations = {
      {"CH4", 0.055},
      {"N2", 0.724},
      {"O2", 0.22},
  };

  std::map<std::string, double> external_concentrations = {
      {"CH4", 0.0},
      {"N2", 0.767},
      {"O2", 0.233},
  };

  for (const auto& [molecule, concentration] : initial_concentrations) {
    std::string Y_temp_template_path = (templateDir / "0/Y_temp").string();
    std::string Y_temp_output_path   = dirPath + "/0/" + molecule;

    std::ifstream Y_temp_originalFile(Y_temp_template_path);
    std::ofstream Y_temp_output_file(Y_temp_output_path);

    if (!Y_temp_originalFile.is_open()) {
      std::cerr << "Erro: Não foi possível abrir o arquivo base Y_temp." << std::endl;
      return 1;  // Retorna um código de erro
    }
    if (!Y_temp_output_file.is_open()) {
      std::cerr << "Erro: Não foi possível criar o arquivo Y_temp." << std::endl;
      return 1;  // Retorna um código de erro
    }

    std::string line;
    while (std::getline(Y_temp_originalFile, line)) {
      size_t pos = line.find("${MOLECULE}");
      if (pos != std::string::npos) {
        line.replace(pos, 11, molecule);
      }
      pos = line.find("${initial_concentration}");
      if (pos != std::string::npos) {
        line.replace(pos, 24, std::to_string(concentration));
      }
      pos = line.find("${concentration_extern}");
      if (pos != std::string::npos) {
        line.replace(pos, 23, std::to_string(external_concentrations[molecule]));
      }
      Y_temp_output_file << line << std::endl;
    }
    Y_temp_originalFile.close();
    Y_temp_output_file.close();
  }

  // Delete temporary Y_temp file
  std::string Y_temp_filePath = dirPath + "/0/Y_temp";
  std::filesystem::remove(Y_temp_filePath);

  // ===== Substituições no controlDict e U =====
  // Preparar formato de tempo e velocidade para substituição
  std::string time_simulation_str = std::to_string(time_simulation);
  // Remove trailing zeros after decimal point
  time_simulation_str.erase(time_simulation_str.find_last_not_of('0') + 1, std::string::npos);
  if (time_simulation_str.back() == '.') {
    time_simulation_str.pop_back();
  }

  std::string time_cold_flow_str = std::to_string(time_cold_flow);
  time_cold_flow_str.erase(time_cold_flow_str.find_last_not_of('0') + 1, std::string::npos);
  if (time_cold_flow_str.back() == '.') {
    time_cold_flow_str.pop_back();
  }

  std::string velocity_inlet_str = std::to_string(velocity_inlet);
  velocity_inlet_str.erase(velocity_inlet_str.find_last_not_of('0') + 1, std::string::npos);
  if (velocity_inlet_str.back() == '.') {
    velocity_inlet_str.pop_back();
  }
  velocity_inlet_str = "(" + velocity_inlet_str + " 0 0)";  // Formato para arquivo U

  // Substituir no controlDict
  std::string controlDictPath                                = dirPath + "/system/controlDict";
  std::map<std::string, std::string> controlDictReplacements = {
      {"${time_simulation}", time_simulation_str}};
  replaceInFile(controlDictPath, controlDictReplacements);

  // Substituir no arquivo U (velocidade inicial)
  std::string U_filePath                            = dirPath + "/0/U";
  std::map<std::string, std::string> U_replacements = {{"${velocity_inlet}", velocity_inlet_str}};
  replaceInFile(U_filePath, U_replacements);

  // Substituir no fvModels (tempo de inicio da ignicao)
  std::string fvModelsPath                                = dirPath + "/constant/fvModels";
  std::map<std::string, std::string> fvModelsReplacements = {
      {"${time_start_ignition}", time_cold_flow_str}};
  replaceInFile(fvModelsPath, fvModelsReplacements);

  std::string position_ignition_flame_str = std::to_string(position_ignition_flame_val);
  // Remove trailing zeros
  position_ignition_flame_str.erase(position_ignition_flame_str.find_last_not_of('0') + 1,
                                    std::string::npos);
  if (position_ignition_flame_str.back() == '.') {
    position_ignition_flame_str.pop_back();
  }
  // Formato vetorial (X 0 0)
  std::string position_ignition_flame_vec = "(" + position_ignition_flame_str + " 0 0)";

  // Substituir no createZonesDict
  std::string createZonesPath                                = dirPath + "/system/createZonesDict";
  std::map<std::string, std::string> createZonesReplacements = {
      {"${position_ignition_flame}", position_ignition_flame_vec}};
  replaceInFile(createZonesPath, createZonesReplacements);

  // TODO: implementar uma forma de executar o script do OpenFOAM

  return 0;
}