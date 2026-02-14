# state-based script generated for ParaView
import os
import csv
import numpy as np
import paraview
paraview.compatibility.major = 6
paraview.compatibility.minor = 0

from paraview.simple import *
from paraview import servermanager
from vtk.util import numpy_support

paraview.simple._DisableFirstRenderCameraReset()

# -----------------------------------------------------------------------------
# Load OpenFOAM case
# -----------------------------------------------------------------------------
reader = OpenFOAMReader(registrationName='simulation.foam', FileName='simulation.foam')
reader.MeshRegions = ['internalMesh']
reader.CellArrays = ['CH4', 'CO2', 'H2O', 'N2', 'O2', 'Qdot', 'T', 'U', 'p', 'rho']

# Ensure current time is used (do not force last time)
animation_scene = GetAnimationScene()
animation_scene.UpdateAnimationUsingDataTimeSteps()
time_keeper = GetTimeKeeper()
current_time = time_keeper.Time
reader.UpdatePipeline(current_time)

# -----------------------------------------------------------------------------
# Sample cell centers to compute profiles along x
# -----------------------------------------------------------------------------
cell_centers = CellCenters(Input=reader)
cell_centers.UpdatePipeline(current_time)

data = servermanager.Fetch(cell_centers)

def get_first_block(dataset):
    if dataset is None:
        return None
    if dataset.IsA('vtkMultiBlockDataSet'):
        it = dataset.NewIterator()
        it.UnRegister(None)
        it.InitTraversal()
        while not it.IsDoneWithTraversal():
            block = it.GetCurrentDataObject()
            if block is not None and block.GetNumberOfPoints() > 0:
                return block
            it.GoToNextItem()
        return None
    return dataset

mesh = get_first_block(data)
if mesh is None:
    raise RuntimeError('No valid mesh block found in the dataset.')

points = numpy_support.vtk_to_numpy(mesh.GetPoints().GetData())
point_data = mesh.GetPointData()
cell_data = mesh.GetCellData()


def get_array(name):
    array = point_data.GetArray(name)
    if array is None:
        array = cell_data.GetArray(name)
    if array is None:
        return None
    return numpy_support.vtk_to_numpy(array)


u = get_array('U')
if u is None:
    raise RuntimeError('Array U not found in the dataset.')

speed = np.linalg.norm(u, axis=1)

x = points[:, 0]

# Group by x positions (rounded to avoid floating point noise)
x_round = np.round(x, 9)
unique_x = np.unique(x_round)
unique_x.sort()

species = ['CH4', 'CO2', 'H2O', 'N2', 'O2']
species_arrays = {name: get_array(name) for name in species}

rows = []
for x_val in unique_x:
    mask = x_round == x_val
    if not np.any(mask):
        continue
    row = {
        'x': float(x_val),
        'U_mean': float(np.mean(speed[mask])),
        'U_max': float(np.max(speed[mask])),
    }
    for name, arr in species_arrays.items():
        if arr is not None:
            row[f'{name}_mean'] = float(np.mean(arr[mask]))
    rows.append(row)

# -----------------------------------------------------------------------------
# Write CSV output
# -----------------------------------------------------------------------------
output_dir = os.path.join(os.getcwd(), 'postProcessing')
os.makedirs(output_dir, exist_ok=True)
output_csv = os.path.join(output_dir, 'x_profiles.csv')

fieldnames = ['x', 'U_mean', 'U_max'] + [f'{name}_mean' for name in species]
with open(output_csv, 'w', newline='') as csvfile:
    writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
    writer.writeheader()
    for row in rows:
        writer.writerow(row)

# -----------------------------------------------------------------------------
# Visualize: simulation render + velocity chart + mass fraction chart
# -----------------------------------------------------------------------------
def safe_set(proxy, name, value):
    try:
        setattr(proxy, name, value)
    except Exception:
        pass

render_view = CreateView('RenderView')
render_view.Set(ViewSize=[1200, 600])

transform = Transform(Input=reader)
transform.Transform.Scale = [1.0, 1.0, -1.0]

render_display = Show(transform, render_view)
render_display.Representation = 'Surface'
u_lut = GetColorTransferFunction('U')
render_display.ColorArrayName = ['CELLS', 'U']
render_display.LookupTable = u_lut
render_display.SetScalarBarVisibility(render_view, True)
safe_set(transform, 'AddInputConnection', False)
try:
    transform_geometry = GetRepresentationProxy(render_view, transform)
    if transform_geometry and hasattr(transform_geometry, 'ShowBox'):
        transform_geometry.ShowBox = 0
except Exception:
    pass
if hasattr(render_view, 'ResetCameraToNearest'):
    render_view.ResetCameraToNearest()
else:
    render_view.ResetCamera()
    camera = render_view.GetActiveCamera()
    camera.Dolly(1.5)

csv_reader = CSVReader(registrationName='x_profiles.csv', FileName=[output_csv])

chart_view_vel = CreateView('XYChartView')
chart_view_vel.Set(ViewSize=[1200, 400])

plot_vel = Show(csv_reader, chart_view_vel)
plot_vel.UseIndexForXAxis = 0
plot_vel.XArrayName = 'x'
plot_vel.SeriesVisibility = ['U_mean', '1', 'U_max', '1']
chart_view_vel.BottomAxisTitle = 'x (m)'
chart_view_vel.LeftAxisTitle = 'Velocity (m/s)'

chart_view_frac = CreateView('XYChartView')
chart_view_frac.Set(ViewSize=[1200, 400])

plot_frac = Show(csv_reader, chart_view_frac)
plot_frac.UseIndexForXAxis = 0
plot_frac.XArrayName = 'x'
series_visibility = []
for name in species:
    series_visibility += [f'{name}_mean', '1']
plot_frac.SeriesVisibility = series_visibility
chart_view_frac.BottomAxisTitle = 'x (m)'
chart_view_frac.LeftAxisTitle = 'Mass fraction'

layout1 = CreateLayout(name='Velocity Layout')
layout1.AssignView(0, render_view)
layout1.SplitVertical(0, 0.6)
layout1.AssignView(2, chart_view_vel)

layout2 = CreateLayout(name='Mass Fractions')
layout2.AssignView(0, chart_view_frac)

# ----
# Nova aba: Highlights de velocidade 30-40 cm/s (0.3-0.4 m/s)
# ----
render_view_highlight = CreateView('RenderView')
render_view_highlight.Set(ViewSize=[1200, 600])

transform_highlight = Transform(Input=reader)
transform_highlight.Transform.Scale = [1.0, 1.0, -1.0]

# Criar filtro de threshold para destacar velocidades entre 0.3 e 0.4 m/s
threshold_filter = Threshold(Input=transform_highlight)
threshold_filter.Scalars = ['CELLS', 'U']
threshold_filter.ThresholdMethod = 'Between'
threshold_filter.LowerThreshold = 0.3
threshold_filter.UpperThreshold = 0.4

highlight_display = Show(threshold_filter, render_view_highlight)
highlight_display.Representation = 'Surface'
highlight_lut = GetColorTransferFunction('U')
highlight_display.ColorArrayName = ['CELLS', 'U']
highlight_display.LookupTable = highlight_lut
highlight_display.SetScalarBarVisibility(render_view_highlight, True)

# Adicionar geometria completa do canal como referência (wireframe)
complete_geometry = Show(transform_highlight, render_view_highlight)
complete_geometry.Representation = 'Wireframe'
complete_geometry.LineWidth = 0.5
complete_geometry.Opacity = 0.3

if hasattr(render_view_highlight, 'ResetCameraToNearest'):
    render_view_highlight.ResetCameraToNearest()
else:
    render_view_highlight.ResetCamera()
    camera = render_view_highlight.GetActiveCamera()
    camera.Dolly(1.5)

layout3 = CreateLayout(name='Velocity Highlights 30-40cm/s')
layout3.AssignView(0, render_view_highlight)

SetActiveView(chart_view_vel)
RenderAllViews()

# Optional: save a screenshot and CSV
# SaveScreenshot(os.path.join(output_dir, 'x_profiles.png'), chart_view)
# SaveData(output_csv, proxy=csv_reader)
