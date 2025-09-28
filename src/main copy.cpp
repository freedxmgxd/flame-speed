#include <cstdlib> // Necessário para std::getenv
#include <fstream>
#include <iostream>
#include <set>
#include <string>

#include <filesystem> // C++17 para manipulação de diretórios

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
  auto blockMeshString_0 =
      R"(/*--------------------------------*- C++ -*----------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Version:  13
     \\/     M anipulation  |
\*---------------------------------------------------------------------------*/
FoamFile
{
    format      ascii;
    class       dictionary;
    object      blockMeshDict;
}
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

// Note: this file is a Copy of $FOAM_TUTORIALS/resources/blockMesh/pitzDaily

convertToMeters 0.001;

vertices
(
)";

  auto blockMeshString_1 =
      R"();

blocks
(
    hex (0 4 6 2 1 5 7 3)
    (10 10 10) 
    simpleGrading (1 1 1)

    hex (4 8 10 6 5 9 11 7)
    (20 10 10)
    simpleGrading (1 1 1)

    hex (8 12 14 10 9 13 15 11)
    (30 10 10)
    simpleGrading (1 1 1)
);

boundary
(
    inlet
    {
        type patch;
        faces
        (
            (0 1 2 3)
        );
    }
    outlet
    {
        type patch;
        faces
        (
            (12 13 14 15)
        );
    }
    walls
    {
        type    wall;
        faces
        (
            (0 1 4 5)
            (0 2 4 6)
            (1 3 5 7)
            (2 3 6 7)
            (4 5 8 9)
            (4 6 8 10)
            (5 7 9 11)
            (6 7 10 11)
            (8 9 12 13)
            (8 10 12 14)
            (9 11 13 15)
            (10 11 14 15)
        );
    }
);

// ************************************************************************* //)";

  auto blockMeshFile_string =
      blockMeshString_0 + vertices_string + blockMeshString_1;

  // write to file
  const char *foamRunEnv = std::getenv("FOAM_RUN");
  if (foamRunEnv == nullptr) {
    std::cerr << "Erro: A variável de ambiente FOAM_RUN não está definida."
              << std::endl;
    return 1; // Retorna um código de erro
  }

  std::string dirPath = std::string(foamRunEnv) + "/canal/system";
  std::string filePath = dirPath + "/blockMeshDict";

  std::filesystem::create_directories(dirPath);

  auto blockMeshFile = std::ofstream(filePath);

  if (!blockMeshFile.is_open()) {
    std::cerr << "Erro: Não foi possível criar ou abrir o arquivo em: "
              << filePath << std::endl;
    return 1;
  }

  blockMeshFile << blockMeshFile_string;
  blockMeshFile.close();

// TODO: implementar um copy and paste da pagina canal_base, e tambem tentar fazer a parte do blockMesh sendo modificando o arquivo blockMeshDict ao inves de usar raw strings, para limpar melhor o codigo

  return 0;
}