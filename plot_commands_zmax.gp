set terminal pngcairo size 800,600
set output 'zmax_comparison.png'
set title 'Z_max Comparison'
set xlabel 'Mixture Fraction'
set ylabel 'Z_max (m)'
set grid
set key outside
set datafile separator ","
set datafile columnheaders
plot 'flame_speed_data.csv' using 1:5 with lines title 'Reduced Mechanism', \
     '' using 1:9 with lines title 'Complete Mechanism'
