#!/usr/bin/env bash
# Uso: ./buildrun.sh [--cleanBuild] [--verbose]

rm -rf logs
mkdir -p logs
VERBOSE=false
CLEAN_BUILD=false

for arg in "$@"; do
  [ "$arg" = "--verbose" ] && VERBOSE=true
  [ "$arg" = "--cleanBuild" ] && CLEAN_BUILD=true
done

# Run clean if requested
if [ "$CLEAN_BUILD" = true ]; then
  foamCleanCase 2>&1 | tee logs/foamCleanCase.log
  mkdir -p logs
fi

if [ "$VERBOSE" = true ]; then
  blockMesh 2>&1 | tee logs/blockMesh.log
  createZones 2>&1 | tee logs/createZones.log
  checkMesh 2>&1 | tee logs/checkMesh.log
  decomposePar -force 2>&1 | tee logs/decomposePar.log
  mpirun -np 16 --oversubscribe foamRun -parallel 2>&1 | tee logs/foamRun.log || true
  reconstructPar 2>&1 | tee logs/reconstructPar.log
else
  echo "Running blockMesh..."
  blockMesh 2>&1 >> logs/blockMesh.log
  echo "Running createZones..."
  createZones 2>&1 >> logs/createZones.log
  echo "Running checkMesh..."
  checkMesh 2>&1 >> logs/checkMesh.log
  echo "Running decomposePar..."
  decomposePar -force 2>&1 >> logs/decomposePar.log
  echo "Running foamRun (mpirun)..."
  mpirun -np 16 --oversubscribe foamRun -parallel 2>&1 >> logs/foamRun.log || true
  echo "Running reconstructPar..."
  reconstructPar 2>&1 >> logs/reconstructPar.log
fi

if command -v paraview.exe &> /dev/null || [ -f "paraview.exe" ]; then
  if [ "$VERBOSE" = true ]; then
    paraview.exe paraview_x_profiles.py 2>&1 | tee logs/paraview.log &
  else
    paraview.exe paraview_x_profiles.py 2>&1 >> logs/paraview.log &
  fi
fi

echo "OK"
