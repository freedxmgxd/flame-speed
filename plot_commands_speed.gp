set terminal pngcairo size 800,600
set output 'flame_speed_comparison.png'
set title 'Flame Speed Comparison'
set xlabel 'Mixture Fraction'
set ylabel 'Flame Speed (m/s)'
set grid
set key outside
set datafile separator ","
set datafile columnheaders
plot 'flame_speed_data.csv' using 1:2 with lines title 'Reduced Mechanism', \
     '' using 1:6 with lines title 'Complete Mechanism'
