#!/usr/bin/env bash
# Uso: ./buildrun.sh (executar de dentro de $FOAM_RUN/canal)

set +e

# Source OpenFOAM
if [ -f /opt/openfoam13/etc/bashrc ]; then
  source /opt/openfoam13/etc/bashrc
elif [ -n "${WM_PROJECT_DIR:-}" ] && [ -f "$WM_PROJECT_DIR/etc/bashrc" ]; then
  source "$WM_PROJECT_DIR/etc/bashrc"
fi

set -e

# Gera malha e checa
blockMesh
checkMesh
createZones

decomposePar -force
mpirun -np 16 --oversubscribe foamRun -parallel || true
reconstructPar

# Abre no ParaView
paraview.exe paraview_x_profiles.py &
echo "OK"
