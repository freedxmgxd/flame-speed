set terminal pngcairo size 800,600
set output 'temperature_comparison.png'
set title 'Adiabatic Flame Temperature Comparison'
set xlabel 'Mixture Fraction'
set ylabel 'Temperature (K)'
set grid
set key outside
set datafile separator ","
set datafile columnheaders
plot 'flame_speed_data.csv' using 1:3 with lines title 'Reduced Mechanism', \
     '' using 1:7 with lines title 'Complete Mechanism'
