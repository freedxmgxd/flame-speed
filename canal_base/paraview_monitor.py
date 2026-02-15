#!/usr/bin/env python3
"""
Monitor ParaView - Visualização decomposed do caso OpenFOAM
Abre a malha com filtro para inverter o eixo Z
"""

import os
import paraview
paraview.compatibility.major = 6
paraview.compatibility.minor = 0

from paraview.simple import *

paraview.simple._DisableFirstRenderCameraReset()

# Load OpenFOAM case in decomposed mode
reader = OpenFOAMReader(registrationName='simulation.foam', FileName='simulation.foam')
reader.CaseType = 'Decomposed Case'
reader.MeshRegions = ['internalMesh']
reader.CellArrays = ['T', 'U', 'p', 'CH4', 'O2', 'N2', 'Qdot']

# Get animation scene and update
animation_scene = GetAnimationScene()
animation_scene.UpdateAnimationUsingDataTimeSteps()
time_keeper = GetTimeKeeper()
current_time = time_keeper.Time
reader.UpdatePipeline(current_time)

# Transform to invert Z axis for visualization
transform = Transform(Input=reader)
transform.Transform.Scale = [1.0, 1.0, -1.0]

# Create mirrored copy in Y (reflect across Y symmetry plane)
transform_mirror = Transform(Input=reader, registrationName='transform_mirror')
transform_mirror.Transform.Scale = [1.0, -1.0, -1.0]

# Combine original and mirrored geometries
append = AppendDatasets(Input=[transform, transform_mirror])
append.UpdatePipeline()
merged_data = append

# Create render view
render_view = CreateView('RenderView')
render_view.Set(ViewSize=[1400, 800])

# Display merged geometry with U field
display = Show(merged_data, render_view)
display.Representation = 'Surface'
display.ColorArrayName = ['CELLS', 'U']
u_lut = GetColorTransferFunction('U')
display.LookupTable = u_lut
display.SetScalarBarVisibility(render_view, True)

# Setup camera
if hasattr(render_view, 'ResetCameraToNearest'):
    render_view.ResetCameraToNearest()
else:
    render_view.ResetCamera()
    camera = render_view.GetActiveCamera()
    camera.Dolly(1.5)

# Create layout
layout = CreateLayout(name='Monitor')
layout.AssignView(0, render_view)

# Set active view and render
SetActiveView(render_view)
RenderAllViews()
