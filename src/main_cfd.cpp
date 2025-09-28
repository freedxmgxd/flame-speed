#include <cstdlib> // Necessário para std::getenv
#include <fstream>
#include <iostream>
#include <set>
#include <string>

#include <filesystem>

int main(int argc, char **argv) {

  auto height_channel = 0.1;    // Altura do canal
  auto length_inlet = 10.0;     // Comprimento entrada
  auto length_expansion = 50.0; // Comprimento expansão
  auto length_outlet = 50.0;    // Comprimento saída
  auto width_inlet = 1.0;       // Largura entrada
  auto width_outlet = 27.8;     // Largura saída

  std::string vertices[16] = {
      "(0 " + std::to_string(-width_inlet / 2) + " 0)", // Vertex 0
      "(0 " + std::to_string(-width_inlet / 2) + " " +
          std::to_string(height_channel) + ")",        // Vertex 1
      "(0 " + std::to_string(width_inlet / 2) + " 0)", // Vertex 2
      "(0 " + std::to_string(width_inlet / 2) + " " +
          std::to_string(height_channel) + ")", // Vertex 3
      "(" + std::to_string(length_inlet) + " " +
          std::to_string(-width_inlet / 2) + " 0)", // Vertex 4
      "(" + std::to_string(length_inlet) + " " +
          std::to_string(-width_inlet / 2) + " " +
          std::to_string(height_channel) + ")", // Vertex 5
      "(" + std::to_string(length_inlet) + " " +
          std::to_string(width_inlet / 2) + " 0)", // Vertex 6
      "(" + std::to_string(length_inlet) + " " +
          std::to_string(width_inlet / 2) + " " +
          std::to_string(height_channel) + ")", // Vertex 7
      "(" + std::to_string(length_inlet + length_expansion) + " " +
          std::to_string(-width_outlet / 2) + " 0)", // Vertex 8
      "(" + std::to_string(length_inlet + length_expansion) + " " +
          std::to_string(-width_outlet / 2) + " " +
          std::to_string(height_channel) + ")", // Vertex 9
      "(" + std::to_string(length_inlet + length_expansion) + " " +
          std::to_string(width_outlet / 2) + " 0)", // Vertex 10
      "(" + std::to_string(length_inlet + length_expansion) + " " +
          std::to_string(width_outlet / 2) + " " +
          std::to_string(height_channel) + ")", // Vertex 11
      "(" + std::to_string(length_inlet + length_expansion + length_outlet) +
          " " + std::to_string(-width_outlet / 2) + " 0)", // Vertex 12
      "(" + std::to_string(length_inlet + length_expansion + length_outlet) +
          " " + std::to_string(-width_outlet / 2) + " " +
          std::to_string(height_channel) + ")", // Vertex 13
      "(" + std::to_string(length_inlet + length_expansion + length_outlet) +
          " " + std::to_string(width_outlet / 2) + " 0)", // Vertex 14
      "(" + std::to_string(length_inlet + length_expansion + length_outlet) +
          " " + std::to_string(width_outlet / 2) + " " +
          std::to_string(height_channel) + ")", // Vertex 15
  };

  std::string vertices_string;
  for (const auto &vertex : vertices) {
    vertices_string += "    " + vertex + "\n";
  }

  const char *foamRunEnv = std::getenv("FOAM_RUN");
  if (foamRunEnv == nullptr) {
    std::cerr << "Erro: A variável de ambiente FOAM_RUN não está definida."
              << std::endl;
    return 1; // Retorna um código de erro
  }
  //  Copy canal_base to canal (resolve absolute path to template)
  std::string dirPath = std::string(foamRunEnv) + "/canal";

  // Template simples: espera encontrar ./canal_base no diretório atual
  std::filesystem::path templateDir = std::filesystem::path("/home/Shinmen/Workspace Cloud/flame-speed/canal_base"); //TODO: ajusta path depending on execution context
  if (!std::filesystem::exists(templateDir)) {
    std::cerr << "Erro: Diretório template 'canal_base' não encontrado no diretório atual." << std::endl;
    return 1;
  }

  // Ensure destination exists before copying
  std::filesystem::create_directories(dirPath);
  std::filesystem::copy(templateDir, dirPath,
                        std::filesystem::copy_options::recursive |
                            std::filesystem::copy_options::overwrite_existing);

  std::ifstream originalFile((templateDir / "system/blockMeshDict").string());
  // write to file


  std::string blockMeshfilePath = dirPath + "/system/blockMeshDict";

  std::filesystem::create_directories(dirPath);

  auto blockMeshFile = std::ofstream(blockMeshfilePath);

  if (!originalFile.is_open()) {
    std::cerr << "Erro: Não foi possível abrir o arquivo base blockMeshDict."
              << std::endl;
    return 1; // Retorna um código de erro
  }
  if (!blockMeshFile.is_open()) {
    std::cerr << "Erro: Não foi possível criar o arquivo blockMeshDict."
              << std::endl;
    return 1; // Retorna um código de erro
  }

  std::string line;
  while (std::getline(originalFile, line)) {
    // Perform modifications on 'line'
    // For example, replace "old" with "new"
    size_t pos = line.find("${vertex_list}");
    if (pos != std::string::npos) {
      line.replace(pos, 15, vertices_string);
    }
    blockMeshFile << line << std::endl;
  }
  originalFile.close();
  blockMeshFile.close();

  // TODO: implementar uma forma de executar o script do OpenFOAM

  return 0;
}