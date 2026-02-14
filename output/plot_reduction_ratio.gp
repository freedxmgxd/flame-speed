set terminal pngcairo size 800,600
set output 'reaction_reduction_ratio.png'

set datafile separator ","
set datafile columnheaders
set datafile missing ""

set xlabel 'Reduction Step'
set ylabel 'Relative Difference in Value'
set title 'Mechanism Reduction: Error Ratio vs Reduction Step'
set grid
set key top right

set style line 1 lc rgb '#1f77b4' lt 1 lw 3 pt 7 ps 1.0

plot 'reaction_reduction.csv' using 0:4 with linespoints ls 1 title 'Reduction Ratio'

# Also export as SVG
set output
set terminal svg size 800,600
set output 'reaction_reduction_ratio.svg'
replot
set output
