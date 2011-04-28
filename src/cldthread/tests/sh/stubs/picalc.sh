# PI CALC

for n in $(eval echo "{1..15}"); do
	
	export PI_COUNT=$n
	let x=3000000000/$PI_COUNT
	export PI_JUMP=$x
	
	$BENCHMARK stubs/picalc.sw "$PI_COUNT threads" $COUNT
	
done


