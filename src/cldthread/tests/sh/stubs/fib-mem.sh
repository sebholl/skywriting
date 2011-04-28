
# FIB MEMOISED

for n in $(eval echo "{0..9}") 10 20 30 40 50 60 70 80; do
	
	export FIB_N=$n
	export FIB_MEM=0
	$BENCHMARK stubs/fib.sw "Fib($n) Memoised" $COUNT
	
done

