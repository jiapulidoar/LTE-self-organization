set terminal png
set output "nodes4.png"
set title "2-D Plot"
set xlabel "X"
set ylabel "Y"

set xrange [-1.5:+1.5]
set yrange [-1.2:+1.2]
set pointsize 3
set terminal png size 700, 700 font ',9'
set label 'UE1' at -0.725689,-0.572644
set label '0.106 mbps' at -0.725689,-0.622644
set label 'UE2' at 0.115738,-0.982095
set label '1.065 mbps' at 0.115738,-1.032095
set label 'UE3' at 0.115447,-1.035914
set label '2.007 mbps' at 0.115447,-1.085914
set label 'UE4' at -0.242149,0.869024
set label '1.245 mbps' at -0.242149,0.819024
set label 'UE5' at -0.863493,-0.283581
set label '2.654 mbps' at -0.863493,-0.333581
set label 'UE6' at 0.360823,0.831060
set label '0.385 mbps' at 0.360823,0.781060
set label 'avg = 0.667648' at  -1,1
plot "-"  title "eNodeB" with points pointtype 3, "-"  title "Conexión" with vectors, "-"  title "UE" with points pointtype 26
-1 0
1 0
e
-0.725689 -0.522644 1.72569 0.522644
0.115738 -0.932095 0.884262 0.932095
0.115447 -0.985914 0.884553 0.985914
-0.242149 0.919024 -0.757851 -0.919024
-0.863493 -0.233581 -0.136507 0.233581
0.360823 0.88106 -1.36082 -0.88106
e
-0.725689 -0.522644
0.115738 -0.932095
0.115447 -0.985914
-0.242149 0.919024
-0.863493 -0.233581
0.360823 0.88106
e
