
# FIB NON-MEMOISED

for n in $(eval echo "{0..10}"); do
	
	export FIB_N=$n
	export FIB_MEM=1
	$BENCHMARK stubs/fib.sw "Fib($n) Non-Memoised" $COUNT
	
done

