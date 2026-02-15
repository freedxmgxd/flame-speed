#!/usr/bin/env bash
# Uso: ./buildrun.sh [--cleanBuild] [--verbose] [--singlethread]

rm -rf logs
mkdir -p logs
VERBOSE=false
CLEAN_BUILD=false
SINGLETHREAD=false

for arg in "$@"; do
  [ "$arg" = "--verbose" ] && VERBOSE=true
  [ "$arg" = "--cleanBuild" ] && CLEAN_BUILD=true
  [ "$arg" = "--singlethread" ] && SINGLETHREAD=true
done

# Run clean if requested
if [ "$CLEAN_BUILD" = true ]; then
  foamCleanCase 2>&1 | tee logs/foamCleanCase.log
  mkdir -p logs
fi

if [ "$VERBOSE" = true ]; then
  blockMesh 2>&1 | tee logs/blockMesh.log
  createZones 2>&1 | tee logs/createZones.log
  refineMesh -all 2>&1 | tee logs/refineMesh.log
  checkMesh 2>&1 | tee logs/checkMesh.log
  
  # Open ParaView monitor if checkMesh succeeded
  if [ $? -eq 0 ]; then
    echo "checkMesh passed. Opening ParaView monitor..."
    paraview.exe paraview_monitor.py 2>&1 | tee logs/paraview.log &
    PARAVIEW_PID=$!
  fi
  
  if [ "$SINGLETHREAD" = true ]; then
    foamRun 2>&1 | tee logs/foamRun.log
  else
    decomposePar -force 2>&1 | tee logs/decomposePar.log
    mpirun -np 16 --oversubscribe foamRun -parallel 2>&1 | tee logs/foamRun.log || true
    reconstructPar 2>&1 | tee logs/reconstructPar.log
  fi
else
  echo "Running blockMesh..."
  blockMesh 2>&1 >> logs/blockMesh.log
  echo "Running createZones..."
  createZones 2>&1 >> logs/createZones.log
  echo "Running refineMesh..."
  refineMesh -all 2>&1 >> logs/refineMesh.log
  echo "Running checkMesh..."
  checkMesh 2>&1 >> logs/checkMesh.log
  
  # Open ParaView monitor if checkMesh succeeded
  if [ $? -eq 0 ]; then
    echo "checkMesh passed. Opening ParaView monitor..."
    paraview.exe paraview_monitor.py 2>&1 >> logs/paraview.log &
    PARAVIEW_PID=$!
  fi
  
  if [ "$SINGLETHREAD" = true ]; then
    echo "Running foamRun (singlethread)..."
    foamRun 2>&1 >> logs/foamRun.log
  else
    echo "Running decomposePar..."
    decomposePar -force 2>&1 >> logs/decomposePar.log
    echo "Running foamRun (mpirun)..."
    mpirun -np 16 --oversubscribe foamRun -parallel 2>&1 >> logs/foamRun.log || true
    echo "Running reconstructPar..."
    reconstructPar 2>&1 >> logs/reconstructPar.log
  fi
fi

echo "OK"
