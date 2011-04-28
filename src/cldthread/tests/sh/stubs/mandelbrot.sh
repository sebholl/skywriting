# MANDELBROT

for n in $(eval echo "{1..15}"); do
	
	export MAN_SPLITX=$n
	export MAN_SPLITY=1
	$BENCHMARK stubs/mandelbrot.sw "${MAN_SPLITX} x ${MAN_SPLITY} tiles" $COUNT
	
done

