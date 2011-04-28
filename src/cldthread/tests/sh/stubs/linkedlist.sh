# LINKED LIST

for n in $(eval echo "{0..20}"); do
	
	export LIST_SIZE=$n
	$BENCHMARK stubs/linkedlist.sw "$n Items" $COUNT
	
done

