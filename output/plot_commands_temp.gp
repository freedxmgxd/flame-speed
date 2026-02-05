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
