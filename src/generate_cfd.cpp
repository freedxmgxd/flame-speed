#include <cmath>
#include <cstdlib>  // Necessário para std::getenv
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>

int main(int argc, char** argv) {
  auto height_channel   = 2.0;    // mm (Altura do canal)
  auto length_inlet     = 37.0;   // mm (Comprimento entrada)
  auto length_expansion = 110.0;  // mm (Comprimento expansão)
  auto length_outlet    = 50.0;   // mm (Comprimento saída)
  auto width_inlet      = 25.0;   // mm (Largura entrada)
  auto width_outlet     = 63.2;   // mm (Largura saída)
  auto height_channel_half = height_channel * 0.5;

  auto min_cell_size = 0.37 * 2.0;  // mm (menor comprimento de célula)

  // Keep dx continuous across blocks by scaling cell counts with length.
  const auto dx_ref        = min_cell_size;
  auto n_cells_x_inlet  = std::max(1, static_cast<int>(std::lround(length_inlet / dx_ref)));
  auto n_cells_x_expansion = std::max(1, static_cast<int>(std::lround(length_expansion / dx_ref)));
  auto n_cells_x_outlet    = std::max(1, static_cast<int>(std::lround(length_outlet / dx_ref)));

  // // Keep dy/dz close to dx at the inlet half-width and full height.
  auto n_cells_y = std::max(1, static_cast<int>(std::lround((width_inlet * 0.5) / dx_ref)));
  auto n_cells_z = std::max(1, static_cast<int>(std::lround(height_channel_half / dx_ref)));


  std::string vertices[26] = {
            "(0 0 " + std::to_string(-height_channel_half
            ) + ")",  // Vertex 0
            "(0 0 0)",  // Vertex 1
            "(0 " + std::to_string(width_inlet / 2) + " " + std::to_string(-height_channel_half) + ")",  // Vertex 2
            "(0 " + std::to_string(width_inlet / 2) + " 0" +
          ")",                                       // Vertex 3
            "(" + std::to_string(length_inlet) + " 0 " + std::to_string(-height_channel_half) + ")",  // Vertex 4
            "(" + std::to_string(length_inlet) + " 0 0" +
          ")",  // Vertex 5
      "(" + std::to_string(length_inlet) + " " + std::to_string(width_inlet / 2) +
              " " + std::to_string(-height_channel_half) + ")",  // Vertex 6
      "(" + std::to_string(length_inlet) + " " + std::to_string(width_inlet / 2) + " " +
              "0" + ")",                         // Vertex 7
            "(" + std::to_string(length_inlet + length_expansion) + " 0 " + std::to_string(-height_channel_half) + ")",  // Vertex 8
      "(" + std::to_string(length_inlet + length_expansion) + " 0 " +
              "0" + ")",  // Vertex 9
      "(" + std::to_string(length_inlet + length_expansion) + " " +
              std::to_string(width_outlet / 2) + " " + std::to_string(-height_channel_half) + ")",  // Vertex 10
            "(" + std::to_string(length_inlet + length_expansion) + " " +
              std::to_string(width_outlet / 2) + " 0" +
          ")",  // Vertex 11
            "(" + std::to_string(length_inlet + length_expansion + length_outlet) + " 0 " + std::to_string(-height_channel_half) + ")",  // Vertex 12
            "(" + std::to_string(length_inlet + length_expansion + length_outlet) + " 0 " +
              "0" + ")",  // Vertex 13
            "(" + std::to_string(length_inlet + length_expansion + length_outlet) + " " +
              std::to_string(width_outlet / 2) + " " + std::to_string(-height_channel_half) + ")",  // Vertex 14
            "(" + std::to_string(length_inlet + length_expansion + length_outlet) + " " +
              std::to_string(width_outlet / 2) + " 0" +
          ")",  // Vertex 15
            "(" + std::to_string(length_inlet + length_expansion) + " " +
              std::to_string(width_outlet / 2 + width_inlet / 2) + " " + std::to_string(-height_channel_half) + ")",  // Vertex 16
            "(" + std::to_string(length_inlet + length_expansion) + " " +
              std::to_string(width_outlet / 2 + width_inlet / 2) + " 0" +
          ")",  // Vertex 17
            "(" + std::to_string(length_inlet + length_expansion + length_outlet) + " " +
              std::to_string(width_outlet / 2 + width_inlet / 2) + " " + std::to_string(-height_channel_half) + ")",  // Vertex 18
            "(" + std::to_string(length_inlet + length_expansion + length_outlet) + " " +
              std::to_string(width_outlet / 2 + width_inlet / 2) + " 0" +
          ")",  // Vertex 19
            "(" + std::to_string(length_inlet + length_expansion) + " 0 " +
              std::to_string(height_channel_half) + ")",  // Vertex 20
            "(" + std::to_string(length_inlet + length_expansion) + " " +
              std::to_string(width_outlet / 2) + " " + std::to_string(height_channel_half) + ")",  // Vertex 21
            "(" + std::to_string(length_inlet + length_expansion + length_outlet) + " 0 " +
              std::to_string(height_channel_half) + ")",  // Vertex 22
            "(" + std::to_string(length_inlet + length_expansion + length_outlet) + " " +
              std::to_string(width_outlet / 2) + " " + std::to_string(height_channel_half) + ")",  // Vertex 23
            "(" + std::to_string(length_inlet + length_expansion) + " " +
              std::to_string(width_outlet / 2 + width_inlet / 2) + " " + std::to_string(height_channel_half) + ")",  // Vertex 24
            "(" + std::to_string(length_inlet + length_expansion + length_outlet) + " " +
              std::to_string(width_outlet / 2 + width_inlet / 2) + " " + std::to_string(height_channel_half) + ")",  // Vertex 25
  };

  std::string vertices_string;
  for (const auto& vertex : vertices) {
    vertices_string += "    " + vertex + "\n";
  }

  std::string mesh_sizes_block1 = std::to_string(n_cells_x_inlet) + " " +
                                  std::to_string(n_cells_y) + " " + std::to_string(n_cells_z);
  std::string mesh_sizes_block2 = std::to_string(n_cells_x_expansion) + " " +
                                  std::to_string(n_cells_y) + " " + std::to_string(n_cells_z);
  std::string mesh_sizes_block3 = std::to_string(n_cells_x_outlet) + " " +
                                  std::to_string(n_cells_y) + " " + std::to_string(n_cells_z);
  std::string mesh_sizes_block4 = std::to_string(n_cells_x_outlet) + " " +
                                  std::to_string(n_cells_y) + " " + std::to_string(n_cells_z);
  std::string mesh_sizes_block5 = std::to_string(n_cells_x_outlet) + " " +
                                  std::to_string(n_cells_y) + " " + std::to_string(n_cells_z);
  std::string mesh_sizes_block6 = std::to_string(n_cells_x_outlet) + " " +
                                  std::to_string(n_cells_y) + " " + std::to_string(n_cells_z);

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

  std::ifstream originalFile((templateDir / "system/blockMeshDict").string());
  // write to file

  std::string blockMeshfilePath = dirPath + "/system/blockMeshDict";

  std::filesystem::create_directories(dirPath);

  auto blockMeshFile = std::ofstream(blockMeshfilePath);

  if (!originalFile.is_open()) {
    std::cerr << "Erro: Não foi possível abrir o arquivo base blockMeshDict." << std::endl;
    return 1;  // Retorna um código de erro
  }
  if (!blockMeshFile.is_open()) {
    std::cerr << "Erro: Não foi possível criar o arquivo blockMeshDict." << std::endl;
    return 1;  // Retorna um código de erro
  }

  std::string line;
  while (std::getline(originalFile, line)) {
    // Perform modifications on 'line'
    // For example, replace "old" with "new"
    size_t pos = line.find("${vertex_list}");
    if (pos != std::string::npos) {
      line.replace(pos, 15, vertices_string);
    }
    pos = line.find("${mesh_sizes_block1}");
    if (pos != std::string::npos) {
      line.replace(pos, 20, mesh_sizes_block1);
    }
    pos = line.find("${mesh_sizes_block2}");
    if (pos != std::string::npos) {
      line.replace(pos, 20, mesh_sizes_block2);
    }
    pos = line.find("${mesh_sizes_block3}");
    if (pos != std::string::npos) {
      line.replace(pos, 20, mesh_sizes_block3);
    }
    pos = line.find("${mesh_sizes_block4}");
    if (pos != std::string::npos) {
      line.replace(pos, 20, mesh_sizes_block4);
    }
    pos = line.find("${mesh_sizes_block5}");
    if (pos != std::string::npos) {
      line.replace(pos, 20, mesh_sizes_block5);
    }
    pos = line.find("${mesh_sizes_block6}");
    if (pos != std::string::npos) {
      line.replace(pos, 20, mesh_sizes_block6);
    }
    blockMeshFile << line << std::endl;
  }
  originalFile.close();
  blockMeshFile.close();

  std::map<std::string, double> initial_concentrations = {
      {"CH4", 0.055},
      {"N2", 0.724},
      {"O2", 0.22},
  };

  for (const auto& [molecule, concentration] : initial_concentrations) {
    std::string Y_temp_template_path = (templateDir / "0/Y_temp").string();
    std::string Y_temp_output_path = dirPath + "/0/" + molecule;
    
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
      pos = line.find("${INITIAL_CONCENTRATION}");
      if (pos != std::string::npos) {
        line.replace(pos, 24, std::to_string(concentration));
      }
      pos = line.find("${OUTPUT_CONCENTRATION}");
      if (pos != std::string::npos) {
        line.replace(pos, 23, std::to_string(concentration));
      }
      Y_temp_output_file << line << std::endl;
    }
    Y_temp_originalFile.close();
    Y_temp_output_file.close();
  }

  // Delete temporary Y_temp file
  std::string Y_temp_filePath = dirPath + "/0/Y_temp";
  std::filesystem::remove(Y_temp_filePath);

  // TODO: implementar uma forma de executar o script do OpenFOAM

  return 0;
}