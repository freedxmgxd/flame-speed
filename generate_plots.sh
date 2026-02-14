#!/usr/bin/env bash
set -euo pipefail

# Run from script directory
cd "$(dirname "$0")"

OUTPUT_DIR="output"

if [ ! -d "$OUTPUT_DIR" ]; then
  echo "Error: Output directory '$OUTPUT_DIR' does not exist."
  exit 1
fi

# Check gnuplot
if ! command -v gnuplot >/dev/null 2>&1; then
  echo "Error: gnuplot not found in PATH" >&2
  exit 1
fi

# Inform
echo "Generating plots (PNG + SVG) with gnuplot in $OUTPUT_DIR..."

# Create gnuplot scripts in output directory
cat <<'GP_EOF' > "$OUTPUT_DIR/plot_commands_speed.gp"
set terminal pngcairo size 800,600
set output 'flame_speed_comparison.png'
set title 'Flame Speed Comparison'
set xlabel 'Equivalence Ratio (φ)'
set x2label 'Mixture Fraction'
set x2tics
set ylabel 'Flame Speed (m/s)'
set grid
set key top right
set key opaque
set key font ",14"
set key spacing 1.2
set key samplen 2.0
set key width 1

# Vertical reference line at phi = 1.0
set arrow 1 from 1.0, graph 0 to 1.0, graph 1 nohead lc rgb '#555555' lw 2 dt 2 front

# Consistent styles across plots
set style line 1 lc rgb '#1f77b4' lt 1 lw 3 pt 7 ps 1.0  # Reduced
set style line 2 lc rgb '#d62728' lt 1 lw 3 pt 7 ps 1.0  # Complete
set datafile separator ","
set datafile columnheaders
set datafile missing ""
set offsets graph 0, 0, 0.05, 0.05
plot 'flame_speed_data.csv' using 2:3 with linespoints ls 1 title 'Reduced Mechanism' axes x1y1, \
     '' using 2:8 with linespoints ls 2 title 'Complete Mechanism' axes x1y1, \
     '' using 1:(0/0) notitle axes x2y1

# Also export as SVG
set output
set terminal svg size 800,600
set output 'flame_speed_comparison.svg'
replot
set output
GP_EOF

cat <<'GP_EOF' > "$OUTPUT_DIR/plot_commands_temp.gp"
set terminal pngcairo size 800,600
set output 'temperature_comparison.png'
set title 'Temperature Comparison'
set xlabel 'Equivalence Ratio (φ)'
set x2label 'Mixture Fraction'
set x2tics
set ylabel 'Temperature (K)'
set grid
set key top right
set key opaque
set key font ",14"
set key spacing 1.2
set key samplen 2.0
set key width 1

# Vertical reference line at phi = 1.0
set arrow 1 from 1.0, graph 0 to 1.0, graph 1 nohead lc rgb '#555555' lw 2 dt 2 front

# Consistent styles across plots
set style line 1 lc rgb '#1f77b4' lt 1 lw 3 pt 7 ps 1.0  # Adiabatic Reduced
set style line 2 lc rgb '#d62728' lt 1 lw 3 pt 7 ps 1.0  # Adiabatic Complete
set style line 3 lc rgb '#1f77b4' lt 2 lw 3 pt 2 ps 1.0  # Max Reduced (cross)
set style line 4 lc rgb '#d62728' lt 2 lw 3 pt 2 ps 1.0  # Max Complete (cross)
set datafile separator ","
set datafile columnheaders
set datafile missing ""
set offsets graph 0, 0, 0.05, 0.05
plot 'flame_speed_data.csv' using 2:4 with linespoints ls 1 title 'Adiabatic (Reduced)' axes x1y1, \
     '' using 2:9 with linespoints ls 2 title 'Adiabatic (Complete)' axes x1y1, \
     '' using 2:5 with linespoints ls 3 title 'Maximum (Reduced)' axes x1y1, \
     '' using 2:10 with linespoints ls 4 title 'Maximum (Complete)' axes x1y1, \
     '' using 1:(0/0) notitle axes x2y1

# Also export as SVG
set output
set terminal svg size 800,600
set output 'temperature_comparison.svg'
replot
set output
GP_EOF

cat <<'GP_EOF' > "$OUTPUT_DIR/plot_commands_tmax.gp"
set terminal pngcairo size 800,600
set output 'tmax_comparison.png'
set title 'Maximum Temperature Comparison'
set xlabel 'Equivalence Ratio (φ)'
set x2label 'Mixture Fraction'
set x2tics
set ylabel 'Temperature (K)'
set grid
set key top right
set key opaque
set key font ",14"
set key spacing 1.2
set key samplen 2.0
set key width 1

# Vertical reference line at phi = 1.0
set arrow 1 from 1.0, graph 0 to 1.0, graph 1 nohead lc rgb '#555555' lw 2 dt 2 front

# Consistent styles across plots
set style line 1 lc rgb '#1f77b4' lt 1 lw 3 pt 7 ps 1.0  # Reduced
set style line 2 lc rgb '#d62728' lt 1 lw 3 pt 7 ps 1.0  # Complete
set datafile separator ","
set datafile columnheaders
set datafile missing ""
set offsets graph 0, 0, 0.05, 0.05
plot 'flame_speed_data.csv' using 2:5 with linespoints ls 1 title 'Reduced Mechanism' axes x1y1, \
     '' using 2:9 with linespoints ls 2 title 'Complete Mechanism' axes x1y1, \
     '' using 1:(0/0) notitle axes x2y1

# Also export as SVG
set output
set terminal svg size 800,600
set output 'tmax_comparison.svg'
replot
set output
GP_EOF

cat <<'GP_EOF' > "$OUTPUT_DIR/plot_commands_zmax.gp"
set terminal pngcairo size 800,600
set output 'zmax_comparison.png'
set title 'Z_max Comparison'
set xlabel 'Equivalence Ratio (φ)'
set x2label 'Mixture Fraction'
set x2tics
set ylabel 'Z_max (m)'
set grid
set key top right
set key opaque
set key font ",14"
set key spacing 1.2
set key samplen 2.0
set key width 1

# Vertical reference line at phi = 1.0
set arrow 1 from 1.0, graph 0 to 1.0, graph 1 nohead lc rgb '#555555' lw 2 dt 2 front

# Consistent styles across plots
set style line 1 lc rgb '#1f77b4' lt 1 lw 3 pt 7 ps 1.0  # Reduced
set style line 2 lc rgb '#d62728' lt 1 lw 3 pt 7 ps 1.0  # Complete
set datafile separator ","
set datafile columnheaders
set datafile missing ""
set offsets graph 0, 0, 0.05, 0.05
plot 'flame_speed_data.csv' using 2:6 with linespoints ls 1 title 'Reduced Mechanism' axes x1y1, \
     '' using 2:11 with linespoints ls 2 title 'Complete Mechanism' axes x1y1, \
     '' using 1:(0/0) notitle axes x2y1

# Also export as SVG
set output
set terminal svg size 800,600
set output 'zmax_comparison.svg'
replot
set output
GP_EOF

cat <<'GP_EOF' > "$OUTPUT_DIR/plot_reduction_ratio.gp"
set terminal pngcairo size 800,600
set output 'reaction_reduction_ratio.png'
set title 'Reaction Reduction Ratio'
set xlabel 'Reduction Step'
set ylabel 'Relative Difference in Value'
set grid
set key top right
set key opaque
set key font ",14"
set key spacing 1.2
set key samplen 2.0
set key width 1

# Consistent styles
set style line 1 lc rgb '#1f77b4' lt 1 lw 3 pt 7 ps 1.0
set datafile separator ","
set datafile columnheaders
set datafile missing ""
set offsets graph 0, 0, 0.05, 0.05
plot 'reaction_reduction.csv' using 0:4 with linespoints ls 1 title 'Reduction Ratio' axes x1y1

# Also export as SVG
set output
set terminal svg size 800,600
set output 'reaction_reduction_ratio.svg'
replot
set output
GP_EOF

# Run gnuplot in output directory
cd "$OUTPUT_DIR"
gnuplot plot_commands_speed.gp
gnuplot plot_commands_temp.gp
gnuplot plot_commands_tmax.gp
gnuplot plot_commands_zmax.gp
gnuplot plot_reduction_ratio.gp

# Summary
printf "\nDone. Generated files in $OUTPUT_DIR:\n"
ls -1 flame_speed_comparison.* temperature_comparison.* tmax_comparison.* zmax_comparison.* reaction_reduction_ratio.* 2>/dev/null | sed 's/^/  - /'
