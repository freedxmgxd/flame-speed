set terminal pngcairo size 800,600
set output 'tmax_comparison.png'
set title 'Maximum Temperature Comparison'
set xlabel 'Mixture Fraction'
set ylabel 'Temperature (K)'
set grid
set key outside
set datafile separator ","
set datafile columnheaders
plot 'flame_speed_data.csv' using 1:4 with lines title 'Reduced Mechanism', \
     '' using 1:8 with lines title 'Complete Mechanism'
