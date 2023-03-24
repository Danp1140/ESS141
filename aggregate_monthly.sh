for month in {1..12}
do
	echo Aggregating Month $month
	./wod_to_shiptrack filelist.txt "APB2007${month}.txt" 2007 $month
done
